/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include <platform/CHIPDeviceLayer.h>
#include "AppTask.h"
#include "AppMatterButton.h"
#include "fsl_component_timer_manager.h"
#include "board_comp.h"
#include "fwk_platform.h"

static BUTTON_HANDLE_DEFINE(sdkButtonHandle);

static button_status_t AppMatterButton_ButtonCallback(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventShortPress:
            GetAppTask().SwitchCommissioningStateHandler();
            break;
        case kBUTTON_EventLongPress:
            GetAppTask().FactoryResetHandler();
            break;
        default:
            break;
    }
    return kStatus_BUTTON_Success;
}

CHIP_ERROR AppMatterButton_registerButtons(void)
{
    button_config_t buttonConfig;
    button_status_t bStatus;
    timer_status_t tStatus;
    CHIP_ERROR err = CHIP_NO_ERROR;

    do
    {
        /* Init the Platform Timer Manager */
        tStatus = PLATFORM_InitTimerManager();
        if (tStatus != kStatus_TimerSuccess)
        {
            err = CHIP_ERROR_UNEXPECTED_EVENT;
            ChipLogError(DeviceLayer, "tmr init error");
            break;
        }

        /* Init board buttons */
        bStatus = BOARD_InitButton((button_handle_t)sdkButtonHandle);
        if (bStatus != kStatus_BUTTON_Success)
        {
            err = CHIP_ERROR_UNEXPECTED_EVENT;
            ChipLogError(DeviceLayer, "button init error");
            break;
        }
        bStatus = BUTTON_InstallCallback((button_handle_t)sdkButtonHandle, AppMatterButton_ButtonCallback, NULL);

        if (bStatus != kStatus_BUTTON_Success)
        {
            err = CHIP_ERROR_UNEXPECTED_EVENT;
            ChipLogError(DeviceLayer, "button init error");
            break;
        }
    }
    while (0);

    return err;
}