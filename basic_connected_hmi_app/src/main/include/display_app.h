
#ifndef DISPLAY_APP_H_
#define DISPLAY_APP_H_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

/*********************
 *      INCLUDES
 *********************/
#include "stdbool.h"
#include "stdint.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
	connected,
	disconnected,
	unknown,
} NetworkSate_t;

typedef enum {
	disabled,
	detached,
	child,
	router,
	leader,
} ThreadRole_t;

typedef enum {
	bt_connected,
	bt_disconnected,
	bt_start_adv,
	bt_stop_adv,
} BluetoothState_t;
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void display_task(void *pvParameters);
void lv_start_display(void);
void updateButtons(uint8_t count);
void updateTable(uint8_t count);
void updateNetworkType(uint8_t device, uint8_t state);
void updateConnectionStatus(uint8_t device, bool isConnected);
void updateDate(uint16_t year, uint8_t month, uint8_t day);
void updateTime(uint8_t hour, uint8_t minutes, uint8_t am_pm);
void updateNetworkState(NetworkSate_t state);
void updateThreadState(ThreadRole_t role);
void updateBluetoothState(BluetoothState_t state);
void updateOnOffState(bool state, uint8_t device);
void updateMatterChannel(uint16_t channel);
void updateMatterPanID(uint16_t panId);
void updateMatterNetworkName(char * name);
void updateMatterIPV6Addr(uint16_t * addr);
void addMatterLogs(char * textLogs, uint16_t length, bool clear);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DISPLAY_APP_H_ */
