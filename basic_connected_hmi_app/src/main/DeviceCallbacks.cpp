/*
 *
 *    Copyright (c) 2020-2023 Project CHIP Authors
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

/**
 * @file DeviceCallbacks.cpp
 *
 * Implements all the callbacks to the application from the CHIP Stack
 *
 **/
#include "DeviceCallbacks.h"

#include <app/clusters/identify-server/identify-server.h>
#include <app/util/attribute-storage.h>
#include <app/util/attribute-table.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/att-storage.h>
#include <app/util/af.h>
#include <app/clusters/bindings/bindings.h>
#include "app/clusters/bindings/BindingManager.h"

#include <lib/support/CodeUtils.h>
#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
#include "openthread-system.h"
#include "ot_platform_common.h"
#endif /* CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED */

#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#include "display_app.h"
#endif

#if WIFI_CONNECT
#include "WifiConnect.h"
#endif

Identify gIdentify0 = {
    chip::EndpointId{ 1 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED,
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyTriggerEffect"); },
};

Identify gIdentify1 = {
    chip::EndpointId{ 1 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED,
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyTriggerEffect"); },
};

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer;

void DeviceCallbacks::DeviceEventCallback(const ChipDeviceEvent * event, intptr_t arg)
{
    ChipLogDetail(DeviceLayer, "DeviceEventCallback: 0x%04x", event->Type);
    uint8_t count = 0;

    switch (event->Type)
    {
    case DeviceEventType::kWiFiConnectivityChange:
        OnWiFiConnectivityChange(event);
        break;
    
    case DeviceEventType::kInternetConnectivityChange:
        OnInternetConnectivityChange(event);
        break;

    case DeviceEventType::kInterfaceIpAddressChanged:
        OnInterfaceIpAddressChanged(event);
        break;
        
    case DeviceEventType::kCHIPoBLEConnectionEstablished:
        ChipLogProgress(DeviceLayer, "CHIPoBLE Connection Established");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateBluetoothState(bt_connected);
#endif
        break;

    case DeviceEventType::kBindingsChangedViaCluster:
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        count = chip::BindingTable::GetInstance().Size();
        updateButtons(count);
        updateTable(count);
#endif    
        break;
    
    case DeviceEventType::kCHIPoBLEConnectionClosed:
        ChipLogProgress(DeviceLayer, "CHIPoBLE Connection Closed");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateBluetoothState(bt_disconnected);
#endif
        break;
    
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
        ChipLogProgress(DeviceLayer, "CHIPoBLE advertising has changed");
        OnBLEAdvertisingStateChange(event);
        break;

#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
    case DeviceEventType::kThreadStateChange:
        OnThreadStateChange(event);
        break;

    case DeviceEventType::kThreadConnectivityChange:
        ChipLogProgress(DeviceLayer, "kThreadConnectivityChange result = %d", event->ThreadConnectivityChange.Result);
        break;

    case DeviceEventType::kCommissioningComplete:
        DeviceCallbacks::OnComissioningComplete(event);
        break;
#endif
    }
}

void DeviceCallbacks::PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId,
                                                  uint8_t type, uint16_t size, uint8_t * value)
{
    ChipLogProgress(DeviceLayer, "endpointId " ChipLogFormatMEI " clusterId " ChipLogFormatMEI " attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u",
                        ChipLogValueMEI(endpointId), ChipLogValueMEI(clusterId), ChipLogValueMEI(attributeId), type, *value, size);
    /* Update GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
    if(clusterId == ZCL_THREAD_NETWORK_DIAGNOSTICS_CLUSTER_ID){
        switch(attributeId){
        case ZCL_DIAG_PAN_ID_ATTRIBUTE_ID:
            updateMatterPanID(*(uint16_t *)value);
            break;
        case ZCL_CHANNEL_ATTRIBUTE_ID:
            updateMatterChannel(*(uint16_t *)value);
            break;
        case ZCL_NETWORK_NAME_ATTRIBUTE_ID:
            updateMatterNetworkName((char *)value);
            break;
        }
    }
    /*else if(clusterId == ZCL_TEMPERATURE_MEASUREMENT_CLUSTER_ID){
        if(attributeId == ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID){
        }
    }*/
    else if(clusterId == ZCL_THERMOSTAT_CLUSTER_ID){
        //TODO
    }
    else if(clusterId == ZCL_ON_OFF_CLUSTER_ID){
        if(attributeId == ZCL_ON_OFF_ATTRIBUTE_ID){
        }
    }
#ifdef DISPLAY_MATTER_LOGS
    /* Add logs on GUI */
    char logs [100] = {0};
    sprintf(logs, "Attribute Changed : Endpoint = %d, cluster = %ld, attribute = %ld", endpointId, clusterId, attributeId);
    addMatterLogs(logs, strlen(logs), false);
#endif
#endif
}

void DeviceCallbacks::OnWiFiConnectivityChange(const ChipDeviceEvent * event)
{
    if (event->WiFiConnectivityChange.Result == kConnectivity_Established)
    {
        ChipLogProgress(DeviceLayer, "WiFi connection established");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateNetworkState(connected);
#endif
#if WIFI_CONNECT
        WifiStatus();
#endif
    }
    else if (event->WiFiConnectivityChange.Result == kConnectivity_Lost)
    {
        ChipLogProgress(DeviceLayer, "WiFi connection lost");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateNetworkState(disconnected);
#endif
    }
}

void DeviceCallbacks::OnBLEAdvertisingStateChange(const ChipDeviceEvent * event)
{
    if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started)
    {
        ChipLogProgress(DeviceLayer, "BLE started advertising");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateBluetoothState(bt_start_adv);
#endif
    }
    else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped)
    {
        ChipLogProgress(DeviceLayer, "BLE stopped advertising");
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        updateBluetoothState(bt_stop_adv);
#endif
    }
}

void DeviceCallbacks::OnInternetConnectivityChange(const ChipDeviceEvent * event)
{
    if (event->InternetConnectivityChange.IPv4 == kConnectivity_Established)
    {
        char ip_addr[Inet::IPAddress::kMaxStringLength];
        event->InternetConnectivityChange.ipAddress.ToString(ip_addr);
        ChipLogProgress(DeviceLayer, "Server ready at: %s:%d", ip_addr, CHIP_PORT);
    }
    else if (event->InternetConnectivityChange.IPv4 == kConnectivity_Lost)
    {
        ChipLogProgress(DeviceLayer, "Lost IPv4 connectivity...");
    }
    if (event->InternetConnectivityChange.IPv6 == kConnectivity_Established)
    {
        char ip_addr[Inet::IPAddress::kMaxStringLength];
        event->InternetConnectivityChange.ipAddress.ToString(ip_addr);
        ChipLogProgress(DeviceLayer, "IPv6 Server ready at: [%s]:%d", ip_addr, CHIP_PORT);
    }
    else if (event->InternetConnectivityChange.IPv6 == kConnectivity_Lost)
    {
        ChipLogProgress(DeviceLayer, "Lost IPv6 connectivity...");
    }
}

void DeviceCallbacks::OnInterfaceIpAddressChanged(const ChipDeviceEvent * event)
{
    switch (event->InterfaceIpAddressChanged.Type)
    {
    case InterfaceIpChangeType::kIpV4_Assigned:
        ChipLogProgress(DeviceLayer, "Interface IPv4 address assigned");
        break;
    case InterfaceIpChangeType::kIpV4_Lost:
        ChipLogProgress(DeviceLayer, "Interface IPv4 address lost");
        break;
    case InterfaceIpChangeType::kIpV6_Assigned:
        ChipLogProgress(DeviceLayer, "Interface IPv6 address assigned");
        break;
    case InterfaceIpChangeType::kIpV6_Lost:
        ChipLogProgress(DeviceLayer, "Interface IPv6 address lost");
        break;
    }
}

void DeviceCallbacks::OnThreadStateChange(const ChipDeviceEvent * event)
{
    /* Add logs on GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U) && defined(DISPLAY_MATTER_LOGS))
    char logs[100] = {0}; // = "Thread State changed :";
    sprintf(logs, "Thread State changed (Event 0x%08lX)", event->ThreadStateChange.OpenThread.Flags);
    addMatterLogs(logs, strlen(logs), false);
    memset(logs, 0, 100);
#endif

#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
    if(event->ThreadStateChange.OpenThread.Flags & OT_CHANGED_THREAD_PANID){
        uint16_t panID = otLinkGetPanId(ThreadStackMgrImpl().OTInstance());
        ChipLogProgress(DeviceLayer, "PanID = 0x%04X", panID);

        /* Add logs on GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#if defined (DISPLAY_MATTER_LOGS)
        sprintf(logs, "    - PanID = 0x%04X", panID);
        addMatterLogs(logs, strlen(logs), false);
        memset(logs, 0, 100);
#endif
        updateMatterPanID(panID);
#endif
        /* TEST PURPOSE ONLY - FORCE TO WRITE IN THE MATTER STACK */
        EmberAfStatus err = emberAfWriteAttribute(0, ZCL_THREAD_NETWORK_DIAGNOSTICS_CLUSTER_ID, ZCL_DIAG_PAN_ID_ATTRIBUTE_ID,
                                                         (uint8_t *) &panID, ZCL_INT16U_ATTRIBUTE_TYPE);
    }

    if(event->ThreadStateChange.OpenThread.Flags & OT_CHANGED_THREAD_CHANNEL){
        uint16_t channelID = (uint16_t) otLinkGetChannel(ThreadStackMgrImpl().OTInstance());
        ChipLogProgress(DeviceLayer, "ChannelID = 0x%04X", channelID);

        /* Add logs on GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#if defined (DISPLAY_MATTER_LOGS)
        sprintf(logs, "    - ChannelID = 0x%04X", channelID);
        addMatterLogs(logs, strlen(logs), false);
        memset(logs, 0, 100);
#endif
        updateMatterChannel(channelID);
#endif
        /* TEST PURPOSE ONLY - FORCE TO WRITE IN THE MATTER STACK */
        EmberAfStatus err = emberAfWriteAttribute(0, ZCL_THREAD_NETWORK_DIAGNOSTICS_CLUSTER_ID, ZCL_CHANNEL_ATTRIBUTE_ID,
                                                         (uint8_t *) &channelID, ZCL_INT16U_ATTRIBUTE_TYPE);
    }

    if(event->ThreadStateChange.OpenThread.Flags & OT_CHANGED_THREAD_NETWORK_NAME){
        const char *ntwName = otThreadGetNetworkName(ThreadStackMgrImpl().OTInstance());
        ChipLogProgress(DeviceLayer, "Ntw Name = %s", ntwName);

        /* Add logs on GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#if defined(DISPLAY_MATTER_LOGS)
        sprintf(logs, "    - Ntw Name = %s", ntwName);
        addMatterLogs(logs, strlen(logs), false);
        memset(logs, 0, 100);
#endif
        updateMatterNetworkName((char *)ntwName);
#endif
        /* TEST PURPOSE ONLY - FORCE TO WRITE IN THE MATTER STACK */
        EmberAfStatus err = emberAfWriteAttribute(0, ZCL_THREAD_NETWORK_DIAGNOSTICS_CLUSTER_ID, ZCL_NETWORK_NAME_ATTRIBUTE_ID,
                                                         (uint8_t *) ntwName, ZCL_CHAR_STRING_ATTRIBUTE_TYPE);
    }

    /* TODO: Not triggered - Check if any Attribute Write possible to trigger ADDR IPV6 update */
    if((event->ThreadStateChange.OpenThread.Flags & OT_CHANGED_THREAD_LL_ADDR) || (event->ThreadStateChange.AddressChanged)){
        const otIp6Address *IPv6Addr = otThreadGetLinkLocalIp6Address(ThreadStackMgrImpl().OTInstance());
        uint16_t addrTable[8] = {0};
        /* Copy and Invert Bytes */
        for(uint8_t i=0; i<8; i++){
            addrTable[i] = ((IPv6Addr->mFields.m16[i] & 0x00FF) << 8) +((IPv6Addr->mFields.m16[i] & 0xFF00) >> 8);
        }
        ChipLogProgress(DeviceLayer, "Addr (IPv6) = %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                 addrTable[0],
                 addrTable[1],
                 addrTable[2],
                 addrTable[3],
                 addrTable[4],
                 addrTable[5],
                 addrTable[6],
                 addrTable[7]);

        /* Add logs on GUI */
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#if defined(DISPLAY_MATTER_LOGS)
        sprintf(logs, "    - Addr = %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                addrTable[0],
                addrTable[1],
                addrTable[2],
                addrTable[3],
                addrTable[4],
                addrTable[5],
                addrTable[6],
                addrTable[7]);
        addMatterLogs(logs, strlen(logs), false);
        memset(logs, 0, 100);
#endif
        updateMatterIPV6Addr(addrTable);
#endif
    }

    if(event->ThreadStateChange.OpenThread.Flags & OT_CHANGED_THREAD_ROLE){
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        otDeviceRole role = otThreadGetDeviceRole(ThreadStackMgrImpl().OTInstance());
        switch (role){
        case OT_DEVICE_ROLE_DISABLED:
            updateThreadState(disabled);
            break;
        case OT_DEVICE_ROLE_DETACHED:
            updateThreadState(detached);
            break;
        case OT_DEVICE_ROLE_CHILD:
            updateThreadState(child);
            break;
        case OT_DEVICE_ROLE_ROUTER:
            updateThreadState(router);
            break;
        case OT_DEVICE_ROLE_LEADER:
            updateThreadState(leader);
            break;
        }
#endif
    }

    /* Update Thread Network quality each time thread stack change event is received */
    otNeighborInfo info;
    otNeighborInfoIterator interator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    while (otThreadGetNextNeighborInfo(ThreadStackMgrImpl().OTInstance(), &interator, &info) == OT_ERROR_NONE){
        ChipLogProgress(DeviceLayer, "Quality link = %d", info.mLinkQualityIn);
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
        switch(info.mLinkQualityIn){
        case 0:
            /*update display element*/
            break;
        case 1:
            /*update display element*/
            break;
        case 2:
            /*update display element*/
            break;
        case 3:
            /*update display element*/
            break;
        }
#endif
    }
#endif /* CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED */
}

#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED
void DeviceCallbacks::OnComissioningComplete(const chip::DeviceLayer::ChipDeviceEvent * event)
{
    /*
     * If a transceiver supporting a multiprotocol scenario is used, a check of the provisioning state is required,
     * so that we can inform the transceiver to stop BLE to give the priority to another protocol.
     * For example it is the case when a K32W0 transceiver supporting OT+BLE+Zigbee is used. When the device is already provisioned,
     * BLE is no more required and the transceiver needs to be informed so that Zigbee can be switched on and BLE switched off.
     *
     * If a transceiver does not support such vendor property the cmd would be ignored.
     */
    if (ConfigurationMgr().IsFullyProvisioned())
    {
        ChipLogDetail(DeviceLayer, "Provisioning complete, stopping BLE\n");
        ThreadStackMgrImpl().LockThreadStack();
        PlatformMgrImpl().StopBLEConnectivity();
        ThreadStackMgrImpl().UnlockThreadStack();
    }
}
#endif
