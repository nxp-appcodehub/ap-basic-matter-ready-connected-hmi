/*
 *
 *    Copyright (c) 2021-2023 Project CHIP Authors
 *    Copyright (c) 2021 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "AppTask.h"
#include "AppEvent.h"
#include "CHIPDeviceManager.h"
#include "DeviceCallbacks.h"
#include "binding-handler.h"
#include "lib/support/ErrorStr.h"
#include <app/server/Server.h>
#include <app/server/Dnssd.h>
#include <lib/dnssd/Advertiser.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
#include <platform/nxp/common/NetworkCommissioningDriver.h>
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/internal/DeviceNetworkInfo.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <lib/support/ThreadOperationalDataset.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/util/attribute-storage.h>

#include "application_config.h"

#include "AppMatterCli.h"
#include "AppMatterButton.h"

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR
#include "OTARequestorInitiator.h"
#endif

#if ENABLE_OTA_PROVIDER
#include <OTAProvider.h>
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <inet/EndPointStateOpenThread.h>
#endif

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
#include "FactoryDataProvider.h"
#include "DataReaderEncryptedDCP.h"
/*
* Test key used to encrypt factory data before storing it to the flash.
* The software key should be used only during development stage.
* For production usage, it is recommended to use the OTP key which needs to be fused in the RT1060 SW_GP2.
*/
static const uint8_t aes128TestKey[] __attribute__((aligned)) = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                                                                0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
#endif

#if WIFI_CONNECT
#include "WifiConnect.h"
#include <wm_os.h>
#endif

#if defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U)
#include "lvgl.h"
#endif

#ifndef APP_TASK_STACK_SIZE
#define APP_TASK_STACK_SIZE ((configSTACK_DEPTH_TYPE)6144 / sizeof(portSTACK_TYPE))
#endif
#ifndef APP_TASK_PRIORITY
#define APP_TASK_PRIORITY (configMAX_PRIORITIES - 8)
#endif
#define APP_EVENT_QUEUE_SIZE 10

static QueueHandle_t sAppEventQueue;

extern bool s_lvgl_initialized;

using namespace chip;
using namespace chip::TLV;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceManager;
using namespace ::chip::app::Clusters;

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR
OTARequestorInitiator gOTARequestorInitiator;
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
namespace {
chip::app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(0 /* Endpoint Id */, &(::chip::DeviceLayer::NetworkCommissioning::NXPWiFiDriver::GetInstance()));
} // namespace

void NetWorkCommissioningInstInit()
{
    sWiFiNetworkCommissioningInstance.Init();
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

/* If only display is enabled, without WiFi */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U) && (WIFI_CONNECT == 0U))
void vApplicationTickHook(void)
{
    if (s_lvgl_initialized)
    {
        lv_tick_inc(1);
    }
}
/* If both display and WiFi are enabled */
#elif (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U) && (WIFI_CONNECT == 1U))
void vApplDisplayTickHook(void)
{
    if (s_lvgl_initialized)
    {
        lv_tick_inc(1);
    }
}
#endif

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::StartAppTask()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    TaskHandle_t taskHandle;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL)
    {
        err = CHIP_ERROR_NO_MEMORY;
        ChipLogError(DeviceLayer, "Failed to allocate app event queue");
        assert(err == CHIP_NO_ERROR);
    }

    if (xTaskCreate(&AppTask::AppTaskMain, "AppTaskMain", APP_TASK_STACK_SIZE,
                    &sAppTask, APP_TASK_PRIORITY, &taskHandle) != pdPASS)
    {
        err = CHIP_ERROR_NO_MEMORY;
        ChipLogError(DeviceLayer, "Failed to start app task");
        assert(err == CHIP_NO_ERROR);
    }

    return err;
}

#if CHIP_ENABLE_OPENTHREAD
void LockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}
#endif

void AppTask::InitServer(intptr_t arg)
{
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();

#if CHIP_ENABLE_OPENTHREAD
    // Init ZCL Data Model and start server
    chip::Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
    nativeParams.lockCb                = LockOpenThreadTask;
    nativeParams.unlockCb              = UnlockOpenThreadTask;
    nativeParams.openThreadInstancePtr = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
    initParams.endpointNativeParams    = static_cast<void *>(&nativeParams);
#endif
    VerifyOrDie((chip::Server::GetInstance().Init(initParams)) == CHIP_NO_ERROR);

#ifdef DEVICE_TYPE_ALL_CLUSTERS
    // Disable last fixed endpoint, which is used as a placeholder for all of the
    // supported clusters so that ZAP will generated the requisite code.
    emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)), false);
#endif /* DEVICE_TYPE_ALL_CLUSTERS */

#if ENABLE_OTA_PROVIDER
    InitOTAServer();
#endif

}

CHIP_ERROR AppTask::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    /* Init Chip memory management before the stack */
    chip::Platform::MemoryInit();

#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U) && defined(WIFI_CONNECT) && (WIFI_CONNECT == 1U))
    os_setup_tick_function(vApplDisplayTickHook);
#endif

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
    /*
    * Load factory data from the flash to the RAM.
    * Needs to be done before starting other Matter modules to avoid concurrent access issues with DCP hardware module.
    */
    FactoryDataProvider *factoryDataProvider = &FactoryDataProvider::GetDefaultInstance();
    DataReaderEncryptedDCP *dataReaderEncryptedDCPInstance = &DataReaderEncryptedDCP::GetDefaultInstance();
    /*
    * This example demonstrates the usage of the ecb with a software key, to use other encryption mode,
    * or to use hardware keys, check available methodes from the DataReaderEncryptedDCP class.
    */
    dataReaderEncryptedDCPInstance->SetEncryptionMode(encrypt_ecb);
    dataReaderEncryptedDCPInstance->SetAes128Key(&aes128TestKey[0]);

    err = factoryDataProvider->Init(dataReaderEncryptedDCPInstance);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Factory Data Provider init failed");
        goto exit;
    }
#endif

    /*
    * Initialize the CHIP stack.
    * Would also initialize all required platform modules
    */
    err = PlatformMgr().InitChipStack();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "PlatformMgr().InitChipStack() failed: %s", ErrorStr(err));
        goto exit;
    }

    /*
    * Register all application callbacks allowing to be informed of stack events
    */
    err = CHIPDeviceManager::GetInstance().Init(&deviceCallbacks);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "CHIPDeviceManager.Init() failed: %s", ErrorStr(err));
        goto exit;
    }

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    ConnectivityMgrImpl().StartWiFiManagement();
#endif

#if CHIP_ENABLE_OPENTHREAD
    err = ThreadStackMgr().InitThreadStack();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Error during ThreadStackMgr().InitThreadStack()");
        goto exit;
    }

    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
    if (err != CHIP_NO_ERROR)
    {
        goto exit;
    }
#endif

    /*
    * Schedule an event to the Matter stack to initialize
    * the ZCL Data Model and start server
    */
    PlatformMgr().ScheduleWork(InitServer, 0);

    /* Init binding handlers */
    err = InitBindingHandlers();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "InitBindingHandlers failed: %s", ErrorStr(err));
        goto exit;
    }

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
    SetDeviceInstanceInfoProvider(factoryDataProvider);
    SetDeviceAttestationCredentialsProvider(factoryDataProvider);
    SetCommissionableDataProvider(factoryDataProvider);
#else
    // Initialize device attestation with example one (only for debug purpose)
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    NetWorkCommissioningInstInit();
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR 
    /* If an update is under test make it permanent */
    gOTARequestorInitiator.HandleSelfTest();
    /* Initialize OTA Requestor */
    PlatformMgr().ScheduleWork(gOTARequestorInitiator.InitOTA, reinterpret_cast<intptr_t>(&gOTARequestorInitiator));
#endif

    /* Register Matter CLI cmds */
    err = AppMatterCli_RegisterCommands();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Error during AppMatterCli_RegisterCommands");
        goto exit;
    }
    /* Register Matter buttons */
    err = AppMatterButton_registerButtons();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Error during AppMatterButton_registerButtons");
        goto exit;
    }

    err = DisplayDeviceInformation();
    if (err != CHIP_NO_ERROR)
        goto exit;

    /* Start a task to run the CHIP Device event loop. */
    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Error during PlatformMgr().StartEventLoopTask()");
        goto exit;
    }

#if CHIP_ENABLE_OPENTHREAD
    // Start OpenThread task
    err = ThreadStackMgrImpl().StartThreadTask();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Error during ThreadStackMgrImpl().StartThreadTask()");
        goto exit;
    }
#endif

#if WIFI_CONNECT
    WifiConnectTask();
#endif

exit:
    return err;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppTask *task = (AppTask *)pvParameter;
    CHIP_ERROR err;
    AppEvent event;

    ChipLogProgress(DeviceLayer, "Welcome to NXP All Clusters Demo App");

    err = task->Init();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "AppTask.Init() failed");
        assert(err == CHIP_NO_ERROR);
    }

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
    }
}

CHIP_ERROR AppTask::DisplayDeviceInformation(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    uint16_t discriminator;
    uint32_t setupPasscode;
    uint16_t vendorId;
    uint16_t productId;
    char currentSoftwareVer[ConfigurationManager::kMaxSoftwareVersionStringLength + 1];

    err = GetCommissionableDataProvider()->GetSetupDiscriminator(discriminator);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Couldn't get discriminator: %s", ErrorStr(err));
        goto exit;
    }
    ChipLogProgress(DeviceLayer, "Setup discriminator: %u (0x%x)", discriminator, discriminator);


    err = GetCommissionableDataProvider()->GetSetupPasscode(setupPasscode);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Couldn't get setupPasscode: %s", ErrorStr(err));
        goto exit;
    }
    ChipLogProgress(DeviceLayer, "Setup passcode: %lu (0x%lx)", setupPasscode, setupPasscode);

    err = GetDeviceInstanceInfoProvider()->GetVendorId(vendorId);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Couldn't get vendorId: %s", ErrorStr(err));
        goto exit;
    }
    ChipLogProgress(DeviceLayer, "Vendor ID: %u (0x%x)", vendorId, vendorId);


    err = GetDeviceInstanceInfoProvider()->GetProductId(productId);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Couldn't get productId: %s", ErrorStr(err));
        goto exit;
    }
    ChipLogProgress(DeviceLayer, "nProduct ID: %u (0x%x)", productId, productId);

    // QR code will be used with CHIP Tool
#if CONFIG_NETWORK_LAYER_BLE
    PrintOnboardingCodes(chip::RendezvousInformationFlag(chip::RendezvousInformationFlag::kBLE));
#else
    PrintOnboardingCodes(chip::RendezvousInformationFlag(chip::RendezvousInformationFlag::kOnNetwork));
#endif /* CONFIG_NETWORK_LAYER_BLE */

    err = ConfigurationMgr().GetSoftwareVersionString(currentSoftwareVer, sizeof(currentSoftwareVer));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Get current software version error");
        goto exit;
    }

    ChipLogProgress(DeviceLayer, "Current Software Version: %s", currentSoftwareVer);

exit:
    return err;
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, aEvent, 0))
        {
            ChipLogError(DeviceLayer, "Failed to post event to app task event queue");
        }
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Event received with no handler. Dropping event.");
    }
}

void AppTask::UpdateClusterState(void)
{
    uint8_t newValue = 0;//!ClusterMgr().IsTurnedOff();
    int16_t newTemp = 2300; //23 deg

    // write the new on/off value
    EmberAfStatus status = emberAfWriteAttribute(1, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID,
                                                 (uint8_t *) &newValue, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogError(DeviceLayer, "ERR: updating on/off %x", status);
    }

    //write the new temperature value
    status = emberAfWriteAttribute(1, ZCL_TEMPERATURE_MEASUREMENT_CLUSTER_ID, ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID,
    											 (uint8_t *) &newTemp, ZCL_INT16S_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogError(NotSpecified, "ERR: updating Temp Measurement %x", status);
    }
}

void AppTask::StartCommissioning(intptr_t arg)
{
    /* Check the status of the commissioning */
    if (ConfigurationMgr().IsFullyProvisioned())
    {
        ChipLogProgress(DeviceLayer, "Device already commissioned");
    }
    else if (chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen())
    {
         ChipLogProgress(DeviceLayer, "Commissioning window already opened");
    }
    else
    {
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
    }
}

void AppTask::StopCommissioning(intptr_t arg)
{
    /* Check the status of the commissioning */
    if (ConfigurationMgr().IsFullyProvisioned())
    {
        ChipLogProgress(DeviceLayer, "Device already commissioned");
    }
    else if (!chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen())
    {
         ChipLogProgress(DeviceLayer, "Commissioning window not opened");
    }
    else
    {
        chip::Server::GetInstance().GetCommissioningWindowManager().CloseCommissioningWindow();
    }
}

void AppTask::SwitchCommissioningState(intptr_t arg)
{
     /* Check the status of the commissioning */
    if (ConfigurationMgr().IsFullyProvisioned())
    {
        ChipLogProgress(DeviceLayer, "Device already commissioned");
    }
    else if (!chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen())
    {
        chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
    }
    else
    {
        chip::Server::GetInstance().GetCommissioningWindowManager().CloseCommissioningWindow();
    }
}

void AppTask::StartCommissioningHandler(void)
{
    /* Publish an event to the Matter task to always set the commissioning state in the Matter task context */
    PlatformMgr().ScheduleWork(StartCommissioning, 0);
}

void AppTask::StopCommissioningHandler(void)
{
    /* Publish an event to the Matter task to always set the commissioning state in the Matter task context */
    PlatformMgr().ScheduleWork(StopCommissioning, 0);
}

void AppTask::SwitchCommissioningStateHandler(void)
{
    /* Publish an event to the Matter task to always set the commissioning state in the Matter task context */
    PlatformMgr().ScheduleWork(SwitchCommissioningState, 0);
}

void AppTask::FactoryResetHandler(void)
{
    ConfigurationMgr().InitiateFactoryReset();
}
