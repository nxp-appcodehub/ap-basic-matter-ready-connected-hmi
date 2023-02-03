/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    Copyright 2023 NXP
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

#include "AppMatterCli.h"
#include <cstring>
#include <platform/CHIPDeviceLayer.h>
#include "AppTask.h"
#include <app/server/Server.h>

#ifdef ENABLE_CHIP_SHELL
#include <lib/shell/Engine.h>
#include <ChipShellCollection.h>
#include "task.h"

#if WIFI_CONNECT
#include "WifiConnect.h"
#endif

#define MATTER_CLI_TASK_SIZE ((configSTACK_DEPTH_TYPE)2048 / sizeof(portSTACK_TYPE))
#define MATTER_CLI_LOG(message) (streamer_printf(streamer_get(), message))

extern bool isLoggingEnabled;

using namespace chip::Shell;
TaskHandle_t AppMatterCliTaskHandle;
static bool isShellInitialized = false;
#else
#define MATTER_CLI_LOG(...)
#endif /* ENABLE_CHIP_SHELL */

using namespace ::chip::DeviceLayer;

void AppMatterCliTask(void * args)
{
#ifdef ENABLE_CHIP_SHELL
    Engine::Root().RunMainLoop();
#endif /* ENABLE_CHIP_SHELL */
}

/* Application Matter CLI commands */
#ifdef ENABLE_CHIP_SHELL
CHIP_ERROR commissioningManager(int argc, char * argv[])
{
    CHIP_ERROR error = CHIP_NO_ERROR;
    if (strncmp(argv[0], "on", 2) == 0)
    {
        GetAppTask().StartCommissioningHandler();
    }
    else if (strncmp(argv[0], "off", 3) == 0)
    {
        GetAppTask().StopCommissioningHandler();
    }
    else
    {
        MATTER_CLI_LOG("wrong args should be either \"mattercommissioning on\" or \"mattercommissioning off\"" );
        error = CHIP_ERROR_INVALID_ARGUMENT;
    }
    return error;
}

CHIP_ERROR cliFactoryReset(int argc, char * argv[])
{
    GetAppTask().FactoryResetHandler();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliReset(int argc, char * argv[])
{
    /* 
       Shutdown device before reboot,
       this emits the ShutDown event, handles the server shutting down,
       and stores in flash the total-operational-hours value.
    */
    chip::DeviceLayer::PlatformMgr().Shutdown();
    chip::DeviceLayer::PlatformMgrImpl().ScheduleResetInIdle();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliLogs(int argc, char * argv[])
{
    char text[27] = "Matter logs are ";
    size_t offset = strlen(text);

    /* Use isLoggingEnabled to toggle on/off the Matter logs */
    isLoggingEnabled = !isLoggingEnabled;

    snprintf(text + offset, sizeof(text) - offset, "%s\r\n", isLoggingEnabled ? "enabled" : "disabled");
    MATTER_CLI_LOG(text);

    return CHIP_NO_ERROR;
}

#if WIFI_CONNECT
CHIP_ERROR cliWifiScan(int argc, char * argv[])
{
    WifiStartScan();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliWifiAdd(int argc, char * argv[])
{
    if ((argc != 2) || ((strlen(argv[0]) == 0) || (strlen(argv[1]) == 0)))
    {
        ChipLogError(Shell, "Usage: wifiadd <ssid> <password>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    WifiAddNetwork(argv[0], argv[1]);

    return CHIP_NO_ERROR;
}

CHIP_ERROR cliWifiConnect(int argc, char * argv[])
{
    WifiConnectNetwork();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliWifiStatus(int argc, char * argv[])
{
    WifiStatus();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliWifiRemove(int argc, char * argv[])
{
    WifiRemoveNetwork();
    return CHIP_NO_ERROR;
}

CHIP_ERROR cliWifiDisconnect(int argc, char * argv[])
{
    WifiDisconnect();
    return CHIP_NO_ERROR;
}
#endif /* WIFI_CONNECT */
#endif /* ENABLE_CHIP_SHELL */

CHIP_ERROR AppMatterCli_RegisterCommands(void)
{
#ifdef ENABLE_CHIP_SHELL
    if (!isShellInitialized)
    {
        int error = Engine::Root().Init();
        if (error != 0)
        {
            ChipLogError(Shell, "Streamer initialization failed: %d", error);
            return CHIP_ERROR_INTERNAL;
        }

        /* Register common shell commands */
        cmd_misc_init();
        cmd_otcli_init();
    #if CHIP_SHELL_ENABLE_CMD_SERVER
        cmd_app_server_init();
    #endif /* CHIP_SHELL_ENABLE_CMD_SERVER */

        /* Register application commands */
        static const shell_command_t kCommands[] = {
            {
                .cmd_func = commissioningManager,
                .cmd_name = "mattercommissioning",
                .cmd_help = "Open/close the commissioning window. Usage : mattercommissioning [on|off]",
            },
            {
                .cmd_func = cliFactoryReset,
                .cmd_name = "matterfactoryreset",
                .cmd_help = "Perform a factory reset on the device",
            },
            {
                .cmd_func = cliReset,
                .cmd_name = "matterreset",
                .cmd_help = "Reset the device",
            },
            {
                .cmd_func = cliLogs,
                .cmd_name = "matterlogs",
                .cmd_help = "Enable or disable Matter logs",
            },
#if WIFI_CONNECT
            {
                .cmd_func = cliWifiScan,
                .cmd_name = "wifiscan",
                .cmd_help = "Scan nearby wifis. Usage : wifiscan",
            },
            {
                .cmd_func = cliWifiAdd,
                .cmd_name = "wifiadd",
                .cmd_help = "Add wifi network. Usage : wifiadd <ssid> <password>",
            },
            {
                .cmd_func = cliWifiConnect,
                .cmd_name = "wificonnect",
                .cmd_help = "Connect to wifi. If no network was added using the wifiadd command, the station connects to the default network set at build time. Usage: wificonnect ",
            },
            {
                .cmd_func = cliWifiStatus,
                .cmd_name = "wifistatus",
                .cmd_help = "Show wifi state. Usage : wifistatus",
            },
            {
                .cmd_func = cliWifiRemove,
                .cmd_name = "wifiremove",
                .cmd_help = "Remove wifi network. The station needs to be disconnected from the network before removal. Usage : wifiremove",
            },
            {
                .cmd_func = cliWifiDisconnect,
                .cmd_name = "wifidisconnect",
                .cmd_help = "Disconnect from the current wireless network. Usage : wifidisconnect",
            },
#endif /* WIFI_CONNECT */
        };

        Engine::Root().RegisterCommands(kCommands, sizeof(kCommands) / sizeof(kCommands[0]));

        if (xTaskCreate(&AppMatterCliTask, "AppMatterCli_task", MATTER_CLI_TASK_SIZE, NULL, 1, &AppMatterCliTaskHandle) != pdPASS)
        {
            ChipLogError(Shell, "Failed to start Matter CLI task");
             return CHIP_ERROR_INTERNAL;
        }
        isShellInitialized = true;
    }
#endif /* ENABLE_CHIP_SHELL */
    
    return CHIP_NO_ERROR;
}


