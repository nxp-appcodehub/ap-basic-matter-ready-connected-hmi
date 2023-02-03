/*
 *  Copyright 2023 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __WIFICONNECT_H__
#define __WIFICONNECT_H__

/* Creates a dedicated Task responsible for connecting to a WiFi network */
void WifiConnectTask(void);

/* Starts the WiFi Scan procedure */
void WifiStartScan(void);

/* Adds SSID and Password for WiFi network */
void WifiAddNetwork(char *ssid, char *password);

/* Start connection to WiFi network after adding credentials */
void WifiConnectNetwork(void);

/* Check WiFi status */
void WifiStatus(void);

/* Removes WiFi network credentials */
void WifiRemoveNetwork(void);

/* Disconnects from WiFi network */
void WifiDisconnect(void);

#endif /* __WIFICONNECT_H__ */
