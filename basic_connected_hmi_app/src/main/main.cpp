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
 *  Copyright 2023-2024 NXP
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
