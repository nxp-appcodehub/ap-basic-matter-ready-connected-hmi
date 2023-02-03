/*
 *  Copyright 2023 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "DeviceCallbacks.h"
#include <lib/support/Span.h>
#include <platform/nxp/common/NetworkCommissioningDriver.h>
#include <platform/ConnectivityManager.h>
#include <lwip/api.h>
#include "AppMatterCli.h"
#include <platform/nxp/common/ram_storage.h>

extern "C" {
#include "wlan.h"
#include "wifi.h"
#include "wm_net.h"
#include <wm_os.h>
}

using namespace ::chip::DeviceLayer;

/* Wifi Connect task configuration */
#define WIFI_CONNECT_TASK_NAME "wifi_connect"
#define WIFI_CONNECT_TASK_SIZE 1024
#define WIFI_CONNECT_TASK_PRIO (configMAX_PRIORITIES - 10)

/* Set to 1 in order to connect to a dedicated Wi-Fi network. */
#ifndef WIFI_CONNECT
#define WIFI_CONNECT 1
#endif

#if WIFI_CONNECT

#define WIFI_SSID_FILENAME "ssid_file"
#define WIFI_PASS_FILENAME "pass_file"

#endif /* WIFI_CONNECT */

char *ipv6_addr_type_to_desc(struct ipv6_config *ipv6_conf);
bool networkAdded = false;

static const char *print_role(enum wlan_bss_role role)
{
    if (role == WLAN_BSS_ROLE_STA)
    {
        return "Infra";
    }
    else if (role == WLAN_BSS_ROLE_UAP)
    {
        return "uAP";
    }
    else if (role == WLAN_BSS_ROLE_ANY)
    {
        return "any";
    }
    else
    {
        return "unknown";
    }
}

#if ENABLE_CHIP_SHELL
static int __scan_cb(unsigned int count)
{
    struct wlan_scan_result res;
    unsigned int i;
    int err;

    if (count == 0U)
    {
        (void)PRINTF("[WIFI_SCAN] No networks found\r\n");
        return 0;
    }

    (void)PRINTF("[WIFI_SCAN] %d network%s found:\r\n", count, count == 1U ? "" : "s");

    for (i = 0; i < count; i++)
    {
        err = wlan_get_scan_result(i, &res);
        if (err != 0)
        {
            (void)PRINTF("[WIFI_SCAN] Error: can't get scan res %d\r\n", i);
            continue;
        }

        print_mac(res.bssid);

        if (res.ssid[0] != '\0')
        {
            (void)PRINTF(" \"%s\" %s\r\n", res.ssid, print_role(res.role));
        }
        else
        {
            (void)PRINTF(" (hidden) %s\r\n", print_role(res.role));
        }

        (void)PRINTF("\tchannel: %d\r\n", res.channel);
        (void)PRINTF("\trssi: -%d dBm\r\n", res.rssi);
        (void)PRINTF("\tsecurity: ");

        if (res.wep != 0U)
        {
            (void)PRINTF("WEP ");
        }

        if ((res.wpa != 0U) && (res.wpa2 != 0U))
        {
            (void)PRINTF("WPA/WPA2 Mixed ");
        }
        else
        {
            if (res.wpa != 0U)
            {
                (void)PRINTF("WPA ");
            }

            if (res.wpa2 != 0U)
            {
                (void)PRINTF("WPA2 ");
            }

            if (res.wpa3_sae != 0U)
            {
                (void)PRINTF("WPA3 SAE ");
            }

            if (res.wpa2_entp != 0U)
            {
                (void)PRINTF("WPA2 Enterprise ");
            }
        }

        if (!((res.wep != 0U) || (res.wpa != 0U) || (res.wpa2 != 0U) || (res.wpa3_sae != 0U) || (res.wpa2_entp != 0U)))
        {
            (void)PRINTF("OPEN ");
        }

        (void)PRINTF("\r\n");
        (void)PRINTF("\tWMM: %s\r\n", (res.wmm != 0U) ? "YES" : "NO");
    }

    return 0;
}
#endif /* ENABLE_CHIP_SHELL */

static void printWiFiStatus(void)
{
    CHIP_ERROR err;
    int result;
    char ip[16];
    uint8_t mac[MLAN_MAC_ADDR_LENGTH];
    struct wlan_ip_config addr;
    struct wlan_network network;
    short int rssi;

    const char security_types[9][20] = {"None", "WEP Open", "WEP Shared", "WPA", "WPA2", "WPA WPA2 Mixed", "Wildcard", "WPA3 SAE", "WPA2 WPA3 SAE Mixed"};

    if (wlan_get_mac_address(mac) == 0)
    {
        ChipLogProgress(DeviceLayer, "[WIFI_STATUS] MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else
    {
        ChipLogError(DeviceLayer,"[WIFI_STATUS] Error: unable to retrieve MAC address");
    }

    result = wlan_get_address(&addr);

    if (result == WM_SUCCESS)
    {
        net_inet_ntoa(addr.ipv4.address, ip);
        result = wlan_get_current_network(&network);

        if (result == WM_SUCCESS)
        {
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] State: Connected");
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Network: \"%s\" ", network.ssid);
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Channel: %d", network.channel);
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Role: %s", print_role(network.role));
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Security: %s", security_types[network.security.type]);

            result = wlan_get_current_rssi(&rssi);

            if (result == WM_SUCCESS)
            {
                ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Rssi: %d dBm", rssi);
            }

            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] IPv4 Address: [%s]", ip);

#if CONFIG_IPV6 && LWIP_IPV6
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] IPv6 Addresses:");

            for (int i = 0; i < CONFIG_MAX_IPV6_ADDRESSES; i++)
            {
                if (network.ip.ipv6[i].addr_state != IP6_ADDR_INVALID)
                {
                    ChipLogProgress(DeviceLayer, "\t%-13s:\t%s (%s)", ipv6_addr_type_to_desc(&network.ip.ipv6[i]),
                        inet6_ntoa(network.ip.ipv6[i].address), ipv6_addr_state_to_desc(network.ip.ipv6[i].addr_state));
                }
            }
#endif /* LWIP_IPV6 */
        }
        else if (result == WLAN_ERROR_STATE)
        {
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] State: Not connected");
        }
        else
        {
            ChipLogProgress(DeviceLayer, "[WIFI_STATUS] Network name is invalid");
        }
    }
    else
    {
        ChipLogProgress(DeviceLayer, "[WIFI_STATUS] State: Not connected");
    }
}

/* Check WiFi status */
void WifiStatus(void)
{
    /* Print network information. */
    printWiFiStatus();
}

#if ENABLE_CHIP_SHELL
/* Starts the WiFi Scan procedure */
void WifiStartScan(void)
{
    /* In the future, switch to the Matter implementation of ScanNetworks():
    * NetworkCommissioning::NXPWiFiDriver::ScanNetworks(ssid, mpScanCallback);
    * For now, use the SDK version. */
    if (wlan_scan(__scan_cb) != 0)
    {
        ChipLogError(DeviceLayer, "[WIFI_SCAN] Error: Scan request failed");
    }
    else
    {
        ChipLogProgress(DeviceLayer,"[WIFI_SCAN] Progress: Scan scheduled...");
    }
}

/* Adds SSID and Password for WiFi network */
void WifiAddNetwork(char *user_ssid, char *user_password)
{
    uint8_t networkIndex;
    NetworkCommissioning::Status status;
    chip::MutableCharSpan debugText;
    chip::ByteSpan ssid;
    chip::ByteSpan password;
    
    ssid = chip::ByteSpan(reinterpret_cast<const uint8_t *>(user_ssid), strlen(user_ssid));
    password = chip::ByteSpan(reinterpret_cast<const uint8_t *>(user_password), strlen(user_password));

    if (IsSpanUsable(ssid) && IsSpanUsable(password))
    {
        ChipLogProgress(DeviceLayer, "[WIFI_ADD] Progress: Adding/Updating network %s", ssid.data());

        /* AddOrUpdateNetwork() checks ssid length and password length. The network info is not persistent. */
        status = NetworkCommissioning::NXPWiFiDriver::GetInstance().AddOrUpdateNetwork(ssid, password, debugText, networkIndex);

        if (status != NetworkCommissioning::Status::kSuccess)
        {
            ChipLogError(DeviceLayer, "[WIFI_ADD] Error: AddOrUpdateNetwork: %u", (uint8_t)status);
        }
        else
        {
            /* The implementation in NetworkCommissioning::NXPWiFiDriver::CommitConfiguration() saves configuration to RAM.
            * Commit network configuration to flash, instead. */
            int err = ramStorageSavetoFlash(WIFI_SSID_FILENAME, (uint8_t *)ssid.data(), ssid.size());

            if (err < 0)
            {
                ChipLogError(DeviceLayer, "[WIFI_ADD] Error: ramStorageSavetoFlash: %d", err);
            }

            err = ramStorageSavetoFlash(WIFI_PASS_FILENAME, (uint8_t *)password.data(), password.size());

            if (err < 0)
            {
                ChipLogError(DeviceLayer, "[WIFI_ADD] Error: ramStorageSavetoFlash: %d", err);
            }

            networkAdded = true;
        }
    }
    else
    {
        ChipLogError(DeviceLayer, "[WIFI_ADD] Error adding SSID %s and password %s", ssid.data(), password.data());
    }
}

/* Start connection to WiFi network after adding credentials */
void WifiConnectNetwork(void)
{
    chip::ByteSpan ssidSpan;
    char ssid[Internal::kMaxWiFiSSIDLength] = {0};

    NetworkCommissioning::NXPWiFiDriver::GetInstance().GetNetworkSSID((char *)&ssid);
    ssidSpan = chip::ByteSpan(reinterpret_cast<const uint8_t *>(ssid), strlen(ssid));

    /* If the desired network was added, then connect, otherwise add network first. */
    if (!networkAdded)
    {
        ChipLogProgress(DeviceLayer, "[WIFI_CONNECT] No network credentials added!");
    }
    else
    {
        ChipLogProgress(DeviceLayer, "[WIFI_CONNECT] Progress: Connecting to network");
        /* Connection event will be returned in OnWiFiConnectivityChange from DeviceCallbacks.cpp */
        NetworkCommissioning::NXPWiFiDriver::GetInstance().ConnectNetwork(ssidSpan, nullptr);
    }
}

/* Removes WiFi network credentials */
void WifiRemoveNetwork(void)
{
    chip::MutableCharSpan debugText;
    uint8_t networkIndex;
    NetworkCommissioning::Status status;

    chip::ByteSpan ssidSpan;
    chip::ByteSpan passwordSpan;
    char ssid[Internal::kMaxWiFiSSIDLength] = {0};
    char credentials[Internal::kMaxWiFiKeyLength] = {0};

    NetworkCommissioning::NXPWiFiDriver::GetInstance().GetNetworkSSID((char *)&ssid);
    ssidSpan = chip::ByteSpan(reinterpret_cast<const uint8_t *>(ssid), strlen(ssid));

    NetworkCommissioning::NXPWiFiDriver::GetInstance().GetNetworkPassword((char *)&credentials);
    passwordSpan = chip::ByteSpan(reinterpret_cast<const uint8_t *>(credentials), strlen(credentials));

    if (IsSpanUsable(ssidSpan) && IsSpanUsable(passwordSpan))
    {
        int err = 0;
        int err_code = 0;

        ChipLogProgress(DeviceLayer, "[WIFI_REMOVE] Trying to remove network %s", ssidSpan.data());

        err_code = wlan_remove_network((char *)ssidSpan.data());

        switch (err_code)
        {
            case -WM_E_INVAL:
                if (!networkAdded)
                {
                    ChipLogError(DeviceLayer, "[WIFI_REMOVE] Error: Network not found");
                    break;
                }
                /* If the network was added, but it didn't reach low level driver structures,
                 * remove network only from high level structures. */
            case WM_SUCCESS:
                status = NetworkCommissioning::NXPWiFiDriver::GetInstance().RemoveNetwork(ssidSpan, debugText, networkIndex);

                if (status != NetworkCommissioning::Status::kSuccess)
                {
                    ChipLogError(DeviceLayer, "[WIFI_REMOVE] Error: RemoveNetwork: %u", (uint8_t)status);
                }
                else
                {
                    ssidSpan = chip::ByteSpan(NULL, 0);
                    passwordSpan = chip::ByteSpan(NULL, 0);

                    /* The implementation in NetworkCommissioning::NXPWiFiDriver::CommitConfiguration() saves configuration to RAM.
                    * Commit network configuration to flash, instead. */
                    err = ramStorageSavetoFlash(WIFI_SSID_FILENAME, (uint8_t *)ssidSpan.data(), ssidSpan.size());

                    if (err < 0)
                    {
                        ChipLogError(DeviceLayer, "[WIFI_ADD] Error: ramStorageSavetoFlash: %d", err);
                    }

                    err = ramStorageSavetoFlash(WIFI_PASS_FILENAME, (uint8_t *)passwordSpan.data(), passwordSpan.size());

                    if (err < 0)
                    {
                        ChipLogError(DeviceLayer, "[WIFI_ADD] Error: ramStorageSavetoFlash: %d", err);
                    }

                    ChipLogProgress(DeviceLayer, "[WIFI_REMOVE] Successfully removed network");
                    networkAdded = false;
                }
                break;
            case WLAN_ERROR_STATE:
                ChipLogError(DeviceLayer, "[WIFI_REMOVE] Error: Can't remove network in this state");
                break;
            default:
                ChipLogError(DeviceLayer, "[WIFI_REMOVE] Error: Unable to remove network");
                break;
        }
    }
    else
    {
        ChipLogError(DeviceLayer, "[WIFI_REMOVE] Error: No saved network credentials!");
    }
}

/* Disconnects from WiFi network */
void WifiDisconnect(void)
{
	if (ConnectivityMgr().IsWiFiStationConnected())
	{
	    ChipLogProgress(DeviceLayer, "[WIFI_DISCONNECT] Progress: Disconnecting");

	    int ret = wlan_disconnect();

	    if (ret != WM_SUCCESS)
	    {
	        ChipLogError(DeviceLayer, "[WIFI_DISCONNECT] Error: wlan_disconnect: %u", (uint8_t)ret);
	    }
	}
	else
	{
	    ChipLogError(DeviceLayer, "[WIFI_DISCONNECT] Error: WiFi not connected!");
	}
}
#endif /* ENABLE_CHIP_SHELL */

void _wifiConnect(void *pvParameters)
{
#if WIFI_CONNECT
    CHIP_ERROR chip_err;
    NetworkCommissioning::Status status;
    chip::MutableCharSpan debugText;
    uint8_t networkIndex;

    int len_ssid, len_pass;
    char ssidBuf[Internal::kMaxWiFiSSIDLength] = {0};
    char passBuf[Internal::kMaxWiFiKeyLength] = {0};

    chip::ByteSpan ssid;
    chip::ByteSpan password;

    /* The Init() method also checks previously saved configurations, but the saving is currently done in RAM. */
    chip_err = NetworkCommissioning::NXPWiFiDriver::GetInstance().Init(nullptr);
    if (chip_err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "[WIFI_CONNECT_TASK] Error: Init: %" CHIP_ERROR_FORMAT, chip_err.Format());
    }

    len_ssid = ramStorageReadFromFlash(WIFI_SSID_FILENAME, (uint8_t *)ssidBuf, Internal::kMaxWiFiSSIDLength);

    if (len_ssid <= 0)
    {
#ifdef WIFI_SSID
        /* If SSID and PASSWORD are entered at build time, they can be used */
        ssid = chip::ByteSpan(reinterpret_cast<const uint8_t *>(WIFI_SSID), strlen(WIFI_SSID));
        ChipLogProgress(DeviceLayer, "[WIFI_CONNECT_TASK] Info: Using the default SSID %s", ssid.data());
#endif
    }
    else
    {
        ssid = chip::ByteSpan(reinterpret_cast<const uint8_t *>(ssidBuf), len_ssid);
    }
    
    len_pass = ramStorageReadFromFlash(WIFI_PASS_FILENAME, (uint8_t *)passBuf, Internal::kMaxWiFiKeyLength);

    if (len_pass <= 0)
    {
#ifdef WIFI_PASSWORD
        /* If SSID and PASSWORD are entered at build time, they can be used */
        password = chip::ByteSpan(reinterpret_cast<const uint8_t *>(WIFI_PASSWORD), strlen(WIFI_PASSWORD));
        ChipLogProgress(DeviceLayer, "[WIFI_CONNECT_TASK] Info: Using the default password %s", password.data());
#endif
    }
    else
    {
        password = chip::ByteSpan(reinterpret_cast<const uint8_t *>(passBuf), len_pass);
    }

    /* If SSID and Password are filled, WiFi can be automatically started */
    if (IsSpanUsable(ssid) && IsSpanUsable(password))
    {
        ChipLogProgress(DeviceLayer, "[WIFI_CONNECT_TASK] Connecting to Wi-Fi network: SSID = %s and PASSWORD = %s", ssid.data(), password.data());

        status = NetworkCommissioning::NXPWiFiDriver::GetInstance().AddOrUpdateNetwork(ssid, password, debugText, networkIndex);
        networkAdded = true;

        if (status != NetworkCommissioning::Status::kSuccess)
        {
            ChipLogError(DeviceLayer, "[WIFI_CONNECT_TASK] Error: AddOrUpdateNetwork: %u", (uint8_t)status);
        }

        /* Connection event will be returned in OnWiFiConnectivityChange from DeviceCallbacks.cpp */
        NetworkCommissioning::NXPWiFiDriver::GetInstance().ConnectNetwork(ssid, nullptr);
    }
    else
    {
        ChipLogError(DeviceLayer, "[WIFI_CONNECT_TASK] No valid SSID and password found!");
    }

    /* Delete task after connection establishment */
    vTaskDelete(NULL);

#endif /* WIFI_CONNECT */
}

/* Creates a dedicated Task responsible for connecting to a WiFi network */
void WifiConnectTask(void)
{
    TaskHandle_t taskHandle;

    if (xTaskCreate(&_wifiConnect, WIFI_CONNECT_TASK_NAME, WIFI_CONNECT_TASK_SIZE, NULL, WIFI_CONNECT_TASK_PRIO, &taskHandle) != pdPASS)
    {
        ChipLogError(DeviceLayer, "Failed to start wifi_connect task");
        assert(false);
    }
}
