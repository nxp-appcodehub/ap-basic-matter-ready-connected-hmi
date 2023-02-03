/*
 *
 *    Copyright (c) 2021-2023 Google LLC.
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

// ================================================================================
// Main Code
// ================================================================================

#include <lib/core/CHIPError.h>
#include <lib/support/logging/CHIPLogging.h>
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#include "lvgl.h"
#include "lvgl_support.h"
#include "display_app.h"
#endif
#include <AppTask.h>
#include "FreeRTOS.h"

#if configAPPLICATION_ALLOCATED_HEAP
uint8_t __attribute__((section(".heap"))) ucHeap[configTOTAL_HEAP_SIZE];
#endif

using namespace ::chip::DeviceLayer;

#ifndef DISPLAY_TASK_SIZE
#define DISPLAY_TASK_SIZE (900)//(configSTACK_DEPTH_TYPE)4096 / sizeof(portSTACK_TYPE))
#endif

bool cluserInit = false;
#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/att-storage.h>
#include <app/util/af.h>

#ifdef SHOW_DATE_TIME
static void UpdateTask(void *param)
{
    uint8_t count = 0;
    /* Use dummy date and hour */
    uint16_t year = 2023;
    uint8_t month = 5;
    uint8_t day = 10;
    uint8_t hour = 00;
    uint8_t min = 00;
    uint8_t am_or_pm = 0;
    updateDate(year, month, day);
    updateTime(hour, min, am_or_pm);

    for (;;){
        vTaskDelay(1000/portTICK_PERIOD_MS);
        /*
         * Update Time and Date:
         * Dummy way just for demo - update minutes each second
         */
        min = (min+1)%60;
        if(min == 0){
            hour = (hour + 1)%12;
            if(hour == 0){
                am_or_pm = (am_or_pm + 1)%2;
                if(am_or_pm == 0){
                    day = (day+1)%31;
                    if(day == 0){
                        month = month%12 + 1;
                        if(month == 1){
                            year = year + 1;
                        }
                    }
                    updateDate(year, month, day);
                }
            }
        }
        updateTime(hour, min, am_or_pm);
    }
}
#endif

#endif

extern "C" int main(int argc, char * argv[])
{
    TaskHandle_t displayTaskHandle;

    PlatformMgrImpl().HardwareInit();
    GetAppTask().StartAppTask();

#if (defined(CHIP_DEVICE_CONFIG_ENABLE_DISPLAY) && (CHIP_DEVICE_CONFIG_ENABLE_DISPLAY > 0U))
    if (xTaskCreate(&display_task, "display_task", DISPLAY_TASK_SIZE, NULL, configMAX_PRIORITIES - 5, &displayTaskHandle) != pdPASS)
    {
        ChipLogError(DeviceLayer, "Failed to start display task");
        assert(false);
    }

#ifdef SHOW_DATE_TIME
    if (xTaskCreate(UpdateTask, "UpdateTask", configMINIMAL_STACK_SIZE + 800, NULL, configMAX_PRIORITIES - 6, NULL) != pdPASS)
    {
        ChipLogError(DeviceLayer, "Failed to start display task");
        assert(false);
    }
#endif
#endif

    ChipLogProgress(DeviceLayer, "Starting FreeRTOS scheduler");
    vTaskStartScheduler();
}

#if (defined(configCHECK_FOR_STACK_OVERFLOW) && (configCHECK_FOR_STACK_OVERFLOW > 0))
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    assert(0);
}
#endif
