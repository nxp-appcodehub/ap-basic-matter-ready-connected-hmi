/*
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "lvgl.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "display_app.h"
extern "C"{
#include "displayResources.h"
}
#include "math.h"
#include "binding-handler.h"
#include <string> 

#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/internal/DeviceNetworkInfo.h>
#include <app/clusters/bindings/bindings.h>
#include <app/clusters/bindings/BindingManager.h>

using namespace chip;
using namespace chip::TLV;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app::Clusters;
/*********************
 *      DEFINES
 *********************/
#define MAX_LOG_LENGTH        500
#ifndef light_count
#define light_count        EMBER_BINDING_TABLE_SIZE
#endif 
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_create_homeTab(lv_obj_t * parent);
static void lv_create_devicesTab(lv_obj_t * parent);
static void lv_create_infoTab(lv_obj_t * parent);
static void lv_create_matterTab(lv_obj_t * parent);
static void lv_create_infoCardWidgets(lv_obj_t * parent, const lv_img_dsc_t *icon, lv_obj_t **image, char * name, lv_obj_t ** infoLabel, char *infoText);
static void lv_create_infoLabel(lv_obj_t * parent, char * infoName, lv_obj_t ** infoLabel, char *defaultValue);
static void onoff_event_handler(lv_event_t * e);
static void draw_part_event_cb(lv_event_t * e);

extern lv_style_t gTabStyle;
extern lv_style_t gInvisibleContainerStyle;
extern lv_style_t gCardWidgetStyle;
extern lv_style_t gCardInfoStyle;
extern lv_style_t gSmallTextStyle;
extern lv_style_t gMediumTextStyle;
extern lv_style_t gLargeTextStyle;
extern lv_style_t gBigTextStyle;

/* Icon/Image declaration */
LV_IMG_DECLARE(networkIcon);
LV_IMG_DECLARE(threadIcon);
LV_IMG_DECLARE(bluetoothIcon);
LV_IMG_DECLARE(onoffIcon);
LV_IMG_DECLARE(qrcodeIcon);
LV_IMG_DECLARE(infoqrIcon);
LV_IMG_DECLARE(nxpIcon);
LV_IMG_DECLARE(thermometerIcon);
LV_IMG_DECLARE(matterIcon);

/**********************
 *  STATIC VARIABLES
 **********************/
/* Home Tab objects */
static lv_obj_t * gTabview;

#ifdef SHOW_DATE_TIME
static lv_obj_t * gDateLabel;
static lv_obj_t * gHourLabel;
static lv_obj_t * gAM_PM_Label;
#endif

static lv_obj_t * NetworkStatusCard;
static lv_obj_t * NetworkStatusLabel;
static lv_obj_t * NetworkImage;
static lv_obj_t * ThreadStatusCard;
static lv_obj_t * ThreadStatusLabel;
static lv_obj_t * ThreadImage;
static lv_obj_t * BluetoothStatusCard;
static lv_obj_t * BluetoothStatusLabel;
static lv_obj_t * BluetoothImage;

static lv_obj_t * OnOffStatusCard[light_count];
static lv_obj_t * OnOffImage[light_count];
static lv_obj_t * OnOffStatusLabel[light_count];
char OnOffText[light_count];

static lv_obj_t * devices_table;
static lv_obj_t * infoChannelLabel;
static lv_obj_t * infoPanIdLabel;
static lv_obj_t * infoNetworkNameLabel;
static lv_obj_t * infoIPV6AddrLabel;

#ifdef DISPLAY_MATTER_LOGS
static lv_obj_t * logLabel;
static lv_obj_t * TextLogsContainer;
static char logs[MAX_LOG_LENGTH] = {0};
#endif

static SemaphoreHandle_t lvgl_mutex;
bool s_lvgl_initialized = false;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void display_task(void *pvParameters)
{
    lv_port_pre_init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    lvgl_mutex = xSemaphoreCreateMutex();
    if(lvgl_mutex == NULL){
        while(1);
    }

    s_lvgl_initialized = true;

    lv_start_display();

    for (;;)
    {
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        lv_task_handler();
        xSemaphoreGive( lvgl_mutex );
        vTaskDelay(5);
    }

    vTaskDelete(NULL);
}

void lv_start_display(void)
{
    /* Init ressources */
    lv_initResources();

    /* Create TabView object */
    gTabview = lv_tabview_create(lv_scr_act(), LV_DIR_BOTTOM, LCD_HEIGHT/7);

    /* Add 2 Tabs to the tabview */
    lv_obj_t * t1 = lv_tabview_add_tab(gTabview, "Home");
    lv_obj_t * t2 = lv_tabview_add_tab(gTabview, "Devices");
    lv_obj_t * t3 = lv_tabview_add_tab(gTabview, "Info");
#ifdef DISPLAY_MATTER_LOGS
    lv_obj_t * t4 = lv_tabview_add_tab(gTabview, "Connectivity");
#endif
    /* Tabview bg style */
    lv_obj_set_style_bg_color(gTabview, lv_palette_lighten(LV_PALETTE_GREY, 2), LV_STATE_DEFAULT);

    /* Button style */
    lv_obj_t * tab_btns = lv_tabview_get_tab_btns(gTabview);
    lv_obj_set_style_bg_color(tab_btns, lv_palette_darken(LV_PALETTE_GREY, 2), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tab_btns, lv_palette_lighten(LV_PALETTE_GREY, 5), LV_STATE_DEFAULT);

    /* Create Home Tab */
    lv_create_homeTab(t1);

    /* Create Devices Tab */
    lv_create_devicesTab(t2);

    /* Create Info Tab */
    lv_create_infoTab(t3);

#ifdef DISPLAY_MATTER_LOGS
    /* Create Connectivity param Tab */
    lv_create_matterTab(t4);
#endif
}

static void lv_create_homeTab(lv_obj_t * parent)
{
    static lv_coord_t col_dsc[] = {LCD_WIDTH/3-5, LCD_WIDTH/3-5, LCD_WIDTH/3-5, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {3*LCD_HEIGHT/7-5, 3*LCD_HEIGHT/7-5, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
    lv_obj_add_style(parent, &gTabStyle, LV_STATE_DEFAULT);

    /* Left side info container */
    lv_obj_t * LeftPanel = lv_obj_create(parent);
    lv_obj_set_size(LeftPanel, lv_pct(33), lv_pct(100));/*6*LCD_HEIGHT/7-5); LCD_WIDTH/3-5*/
    lv_obj_set_grid_cell(LeftPanel, LV_GRID_ALIGN_START, 0, 0, LV_GRID_ALIGN_START, 0, 1);
    lv_obj_add_style(LeftPanel, &gInvisibleContainerStyle, LV_STATE_DEFAULT);

#ifdef SHOW_DATE_TIME
    /* Use updateDate and updateTime APIs to update values on screen */
    /* Date */
    gDateLabel = lv_label_create(LeftPanel);
    lv_obj_set_size(gDateLabel, lv_pct(100)-10, LV_SIZE_CONTENT);
    lv_obj_align(gDateLabel, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_style(gDateLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(gDateLabel, "2023/05/03");
    /* Hour */
    lv_obj_t * HourContainer = lv_obj_create(LeftPanel);
    lv_obj_set_size(HourContainer, lv_pct(100)-10, LV_SIZE_CONTENT);
    lv_obj_align_to(HourContainer, gDateLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -5);
    lv_obj_add_style(HourContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    gHourLabel = lv_label_create(HourContainer);
    lv_obj_align(gHourLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_style(gHourLabel, &gMediumTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(gHourLabel, "09:43");
    gAM_PM_Label = lv_label_create(HourContainer);
    lv_obj_add_style(gAM_PM_Label, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_obj_align_to(gAM_PM_Label, gHourLabel, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, -5);
    lv_label_set_text(gAM_PM_Label, "PM");
#endif

    /*pairing qrcode*/
    lv_obj_t * qrcodeIconImage = lv_img_create(LeftPanel);
    lv_img_set_src(qrcodeIconImage, &qrcodeIcon);
    lv_img_set_zoom(qrcodeIconImage, 384);
    lv_obj_align(qrcodeIconImage, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t * qrlabel = lv_label_create(LeftPanel);
    lv_obj_set_size(qrlabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(qrlabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(qrlabel, "Scan for pairing    :");
    lv_obj_align_to(qrlabel, qrcodeIconImage, LV_ALIGN_OUT_TOP_MID, 0, -25);

        /* Right side states container */
    lv_obj_t * StatePanel = lv_obj_create(parent);
    lv_obj_set_size(StatePanel, lv_pct(67), lv_pct(50));
    lv_obj_set_grid_cell(StatePanel, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);
    lv_obj_add_style(StatePanel, &gTabStyle, LV_STATE_DEFAULT);
    /* Network diagnostics */
    NetworkStatusCard = lv_obj_create(StatePanel);
    lv_obj_set_size(NetworkStatusCard, lv_pct(33)-2, lv_pct(100));
    lv_obj_add_style(NetworkStatusCard, &gCardInfoStyle, LV_STATE_DEFAULT);
    lv_obj_align(NetworkStatusCard, LV_ALIGN_LEFT_MID, 0, 0);
    lv_create_infoCardWidgets(NetworkStatusCard, &networkIcon, &NetworkImage, (char *) "NETWORK", &NetworkStatusLabel, (char *) "UNKNOW");
    updateNetworkState(unknown);

    /* Thread info */
    ThreadStatusCard = lv_obj_create(StatePanel);
    lv_obj_set_size(ThreadStatusCard, lv_pct(33)-2, lv_pct(100));
    lv_obj_add_style(ThreadStatusCard, &gCardInfoStyle, LV_STATE_DEFAULT);
    lv_obj_align_to(ThreadStatusCard, NetworkStatusCard, LV_ALIGN_OUT_RIGHT_MID, 9, 0);
    lv_create_infoCardWidgets(ThreadStatusCard, &threadIcon, &ThreadImage, (char *) "ROLE", &ThreadStatusLabel, (char *) "DISABLED");
    updateThreadState(disabled);

    /* Bluetooth info */
    BluetoothStatusCard = lv_obj_create(StatePanel);
    lv_obj_set_size(BluetoothStatusCard, lv_pct(33)-2, lv_pct(100));
    lv_obj_add_style(BluetoothStatusCard, &gCardInfoStyle, LV_STATE_DEFAULT);
    lv_obj_align_to(BluetoothStatusCard, ThreadStatusCard, LV_ALIGN_OUT_RIGHT_MID, 9, 0);
    lv_create_infoCardWidgets(BluetoothStatusCard, &bluetoothIcon, &BluetoothImage, (char *) "STATE", &BluetoothStatusLabel, (char *) "DISCONNECTED");
    updateBluetoothState(bt_disconnected);

    /* Right side buttons container */
    lv_obj_t * ButtonPanel = lv_obj_create(parent);
    lv_obj_set_size(ButtonPanel, lv_pct(67), lv_pct(50));
    lv_obj_set_grid_cell(ButtonPanel, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 1, 1);
    lv_obj_add_style(ButtonPanel, &gTabStyle, LV_STATE_DEFAULT);

    for(int i = 0; i<light_count; i++)
    {
        OnOffStatusCard[i] = lv_btn_create(ButtonPanel);
        lv_obj_set_size(OnOffStatusCard[i], lv_pct(33)-2, lv_pct(100));
        lv_obj_add_style(OnOffStatusCard[i], &gCardWidgetStyle, LV_STATE_DEFAULT);
        lv_obj_add_style(OnOffStatusCard[i], &gCardWidgetStyle, LV_STATE_CHECKED);
        if(i == 0 )
        {
            lv_obj_align(OnOffStatusCard[i], LV_ALIGN_LEFT_MID, 0, 0);
        }
        else
        {
            lv_obj_align_to(OnOffStatusCard[i], OnOffStatusCard[i-1], LV_ALIGN_OUT_RIGHT_MID, 9, 0);
        }

        snprintf(OnOffText, sizeof(OnOffText), "Light %d",i+1);

        lv_create_infoCardWidgets(OnOffStatusCard[i], &onoffIcon, &OnOffImage[i], (char*) OnOffText, &OnOffStatusLabel[i], (char *) "ON");
        lv_obj_add_event_cb(OnOffStatusCard[i], onoff_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(OnOffStatusCard[i], LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_flag(OnOffStatusCard[i], LV_OBJ_FLAG_HIDDEN);
        updateOnOffState(false,i);
    }
}

static void lv_create_devicesTab(lv_obj_t * parent)
{
    devices_table = lv_table_create(parent);
    lv_obj_align(devices_table, LV_ALIGN_TOP_MID, 0, -10);

    lv_table_set_col_width(devices_table, 0, LCD_WIDTH  / 4);
    lv_table_set_col_width(devices_table, 1, LCD_WIDTH  / 4);
    lv_table_set_col_width(devices_table, 2, LCD_WIDTH  / 4);
    lv_table_set_col_width(devices_table, 3, LCD_WIDTH  / 4);

    /*Fill the first row*/
    lv_table_set_cell_value(devices_table, 0, 0, "Device");
    lv_table_set_cell_value(devices_table, 0, 1, "Network");
    lv_table_set_cell_value(devices_table, 0, 2, "Status");
    lv_table_set_cell_value(devices_table, 0, 3, "Cluster");

    lv_obj_add_event_cb(devices_table, draw_part_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
}

static void lv_create_infoTab(lv_obj_t * parent)
{
    /*info qrcode*/
    lv_obj_t * qrcodeIconImage = lv_img_create(parent);
    lv_img_set_src(qrcodeIconImage, &infoqrIcon);
    lv_img_set_zoom(qrcodeIconImage, 384);
    lv_obj_align(qrcodeIconImage, LV_ALIGN_LEFT_MID, 10, 40);

    lv_obj_t * qrlabel = lv_label_create(parent);
    lv_obj_set_size(qrlabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(qrlabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(qrlabel, "Scan QR code for more info:");
    lv_obj_align(qrlabel, LV_ALIGN_LEFT_MID, 0, -20);

    /*NXP logo*/
    lv_obj_t * nxpIconImage = lv_img_create(parent);
    lv_img_set_src(nxpIconImage, &nxpIcon);
    lv_img_set_zoom(nxpIconImage, 384);
    lv_obj_align(nxpIconImage, LV_ALIGN_TOP_LEFT, 10, 0);

#ifndef DISPLAY_MATTER_LOGS
    /* Matter Icon */
    lv_obj_t * MatterIconIimage = lv_img_create(parent);
    lv_img_set_src(MatterIconIimage, &matterIcon);
    lv_obj_set_size(MatterIconIimage, matterIcon.header.w, lv_pct(20));
    lv_obj_align(MatterIconIimage, LV_ALIGN_TOP_RIGHT, 0, -5);

    /* Info Container */
    lv_obj_t * InfoContainer = lv_obj_create(parent);
    lv_obj_set_size(InfoContainer, lv_pct(49), lv_pct(79));
    lv_obj_add_style(InfoContainer, &gCardWidgetStyle, LV_STATE_DEFAULT);
    lv_obj_align(InfoContainer, LV_ALIGN_BOTTOM_RIGHT, 10, 0);
    lv_obj_set_flex_flow(InfoContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "Channel: ", &infoChannelLabel, (char *) "0");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "PanID: ", &infoPanIdLabel, (char *) "0");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "Network Name: ", &infoNetworkNameLabel, (char *) "Unknown");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_obj_set_height(InnerContainer, 30);
    lv_create_infoLabel(InnerContainer, (char *) "IPV6 Adrr: ", &infoIPV6AddrLabel, (char *) "0:0:0:0:0:0:0:0");
#endif
}

static void onoff_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    uint8_t nodeid;

    for(int i=0; i<light_count; i++)
    {
        if(obj == OnOffStatusCard[i])
        {
            nodeid = i + 1;
            break;
        }
    }

    if(code == LV_EVENT_VALUE_CHANGED) {
        BindingCommandData * data = Platform::New<BindingCommandData>();
        data->commandId           = chip::app::Clusters::OnOff::Commands::Toggle::Id;
        data->clusterId           = chip::app::Clusters::OnOff::Id;
        data->NodeId              = nodeid;
        DeviceLayer::PlatformMgr().ScheduleWork(ControllerWorkerFunction, reinterpret_cast<intptr_t>(data));
    }
}

static void lv_create_infoCardWidgets(lv_obj_t * parent, const lv_img_dsc_t *icon, lv_obj_t **image, char * name, lv_obj_t ** infoLabel, char *infoText)
{
    /* Icon Object */
    *image = lv_img_create(parent);
    lv_img_set_src(*image, icon);
    lv_obj_set_size(*image, icon->header.w, icon->header.h);
    lv_obj_align(*image, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_img_recolor_opa(*image, 255, 0);
    lv_obj_set_style_img_recolor(*image, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);

    lv_obj_t * nameLabel = lv_label_create(parent);
    lv_obj_set_size(nameLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(nameLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(nameLabel, name);
    lv_obj_align_to(nameLabel, *image, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    *infoLabel = lv_label_create(parent);
    lv_obj_set_size(*infoLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_style(*infoLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_obj_set_size(*infoLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(*infoLabel, infoText);

    lv_obj_align_to(*infoLabel, parent, LV_ALIGN_BOTTOM_MID, 0, -10);
}

#ifdef DISPLAY_MATTER_LOGS
static void lv_create_matterTab(lv_obj_t * parent)
{
    lv_obj_add_style(parent, &gTabStyle, LV_STATE_DEFAULT);
    /* Matter Icon */
    lv_obj_t * MatterIconIimage = lv_img_create(parent);
    lv_img_set_src(MatterIconIimage, &matterIcon);
    lv_obj_set_size(MatterIconIimage, matterIcon.header.w, lv_pct(20));
    lv_obj_align(MatterIconIimage, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Title */
    lv_obj_t * titleLabel = lv_label_create(parent);
    lv_obj_set_size(titleLabel, LV_SIZE_CONTENT, lv_pct(20));
    lv_obj_add_style(titleLabel, &gLargeTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(titleLabel, "MATTER INFORMATION");
    lv_obj_align_to(titleLabel, MatterIconIimage, LV_ALIGN_OUT_RIGHT_MID, 0, 5);

    /* Info Container */
    lv_obj_t * InfoContainer = lv_obj_create(parent);
    lv_obj_set_size(InfoContainer, lv_pct(49), lv_pct(79));
    lv_obj_add_style(InfoContainer, &gCardWidgetStyle, LV_STATE_DEFAULT);
    lv_obj_align(InfoContainer, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_flex_flow(InfoContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "Channel: ", &infoChannelLabel, (char *) "0");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "PanID: ", &infoPanIdLabel, (char *) "0");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_create_infoLabel(InnerContainer, (char *) "Network Name: ", &infoNetworkNameLabel, (char *) "Unknown");
    InnerContainer = lv_obj_create(InfoContainer);
    lv_obj_set_size(InnerContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(InnerContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_obj_set_height(InnerContainer, 30);
    lv_create_infoLabel(InnerContainer, (char *) "IPV6 Adrr: ", &infoIPV6AddrLabel, (char *) "0:0:0:0:0:0:0:0");

    /* Logs Container */
    lv_obj_t * LogsContainer = lv_obj_create(parent);
    lv_obj_set_size(LogsContainer, lv_pct(49), lv_pct(79));
    lv_obj_add_style(LogsContainer, &gCardWidgetStyle, LV_STATE_DEFAULT);
    lv_obj_align(LogsContainer, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_clear_flag(LogsContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t * LogTitleLabel = lv_label_create(LogsContainer);
    lv_obj_set_size(LogTitleLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(LogTitleLabel, "Logs:");
    lv_obj_add_style(LogTitleLabel, &gMediumTextStyle, LV_STATE_DEFAULT);
    lv_obj_align(LogTitleLabel, LV_ALIGN_TOP_LEFT, 5, 0);
    TextLogsContainer = lv_obj_create(LogsContainer);
    lv_obj_add_style(TextLogsContainer, &gInvisibleContainerStyle, LV_STATE_DEFAULT);
    lv_obj_set_size(TextLogsContainer, lv_pct(100), lv_pct(80));
    lv_obj_align_to(TextLogsContainer, LogTitleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    logLabel = lv_label_create(TextLogsContainer);
    lv_obj_set_size(logLabel, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_add_style(logLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_label_set_text(logLabel, "No log to show...");
    lv_obj_align(logLabel, LV_ALIGN_TOP_LEFT, 0, 0);
}
#endif

static void lv_create_infoLabel(lv_obj_t * parent, char * infoName, lv_obj_t ** infoLabel, char *defaultValue)
{
    lv_obj_t * InfoNameLabel = lv_label_create(parent);
    lv_obj_set_size(InfoNameLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(InfoNameLabel, infoName);
    lv_obj_add_style(InfoNameLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_obj_align(InfoNameLabel, LV_ALIGN_TOP_LEFT, 5, 0);

    *infoLabel = lv_label_create(parent);
    lv_obj_set_size(*infoLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(*infoLabel, defaultValue);
    lv_obj_add_style(*infoLabel, &gSmallTextStyle, LV_STATE_DEFAULT);
    lv_obj_align_to(*infoLabel, InfoNameLabel, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
}

/****************************
 *   APP INTERFACE FUNCTIONS
 ****************************/
#ifdef SHOW_DATE_TIME
void updateDate(uint16_t year, uint8_t month, uint8_t day)
{
    char buf[11];
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_snprintf(buf, sizeof(buf), "%04d/%02d/%02d", year, month, day);
    lv_label_set_text(gDateLabel, buf);
    xSemaphoreGive( lvgl_mutex );
}

void updateTime(uint8_t hour, uint8_t minutes, uint8_t am_or_pm)
{
    char buf[6];
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", hour, minutes);
    lv_label_set_text(gHourLabel, buf);

    if(am_or_pm == 0){
        lv_label_set_text(gAM_PM_Label, "AM");
    }
    else{
        lv_label_set_text(gAM_PM_Label, "PM");
    }

    lv_obj_align_to(gAM_PM_Label, gHourLabel, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, -5);
    xSemaphoreGive( lvgl_mutex );
}
#endif

void updateNetworkState(NetworkSate_t state)
{
    lv_style_value_t value;
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);

    switch(state)
    {
        case connected:{
            lv_label_set_text(NetworkStatusLabel, "CONNECTED");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(NetworkImage, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
        } break;
        case disconnected:{
            lv_label_set_text(NetworkStatusLabel, "DISCONNECTED");
            value.ptr = &lv_font_montserrat_10;
            lv_obj_set_style_img_recolor(NetworkImage, lv_palette_main(LV_PALETTE_RED), 0);
        } break;
        case unknown:{
            lv_label_set_text(NetworkStatusLabel, "UNKNOWN");
            value.ptr = &lv_font_montserrat_10;
            lv_obj_set_style_img_recolor(NetworkImage, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);

        } break;
    }

    lv_obj_set_local_style_prop(NetworkStatusLabel, LV_STYLE_TEXT_FONT, value, LV_PART_MAIN);
    lv_obj_align_to(NetworkStatusLabel, NetworkStatusCard, LV_ALIGN_BOTTOM_MID, 0, -10);
    xSemaphoreGive( lvgl_mutex );
}

void updateThreadState(ThreadRole_t role)
{
    lv_style_value_t value;
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);

    switch(role){
        case disabled:
            lv_label_set_text(ThreadStatusLabel, "DISABLED");
            value.ptr = &lv_font_montserrat_10;
            lv_obj_set_style_img_recolor(ThreadImage,  lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
            break;
        case detached:
            lv_label_set_text(ThreadStatusLabel, "DETACHED");
            value.ptr = &lv_font_montserrat_10;
            lv_obj_set_style_img_recolor(ThreadImage, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
            break;
        case child:
            lv_label_set_text(ThreadStatusLabel, "CHILD");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(ThreadImage, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
            break;
        case router:
            lv_label_set_text(ThreadStatusLabel, "ROUTER");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(ThreadImage, lv_palette_main(LV_PALETTE_DEEP_ORANGE), 0);
            break;
        case leader:
            lv_label_set_text(ThreadStatusLabel, "LEADER");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(ThreadImage, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
            break;
    }

    lv_obj_set_local_style_prop(ThreadStatusLabel, LV_STYLE_TEXT_FONT, value, LV_PART_MAIN);
    lv_obj_align_to(ThreadStatusLabel, ThreadStatusCard, LV_ALIGN_BOTTOM_MID, 0, -10);
    xSemaphoreGive( lvgl_mutex );
}

void updateBluetoothState(BluetoothState_t state)
{
    lv_style_value_t value;
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);

    switch(state)
    {
        case bt_connected:{
            lv_label_set_text(BluetoothStatusLabel, "CONNECTED");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(BluetoothImage, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
        } break;
        case bt_disconnected:{
            lv_label_set_text(BluetoothStatusLabel, "DISCONNECTED");
            value.ptr = &lv_font_montserrat_10;
            lv_obj_set_style_img_recolor(BluetoothImage, lv_palette_main(LV_PALETTE_GREY), 0);
        } break;
        case bt_start_adv:{
            lv_label_set_text(BluetoothStatusLabel, "ADVERTISING");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(BluetoothImage, lv_palette_main(LV_PALETTE_DEEP_ORANGE), 0);
        } break;
        case bt_stop_adv:{
            lv_label_set_text(BluetoothStatusLabel, "STOPPED");
            value.ptr = &lv_font_montserrat_12;
            lv_obj_set_style_img_recolor(BluetoothImage, lv_palette_main(LV_PALETTE_GREY), 0);
        } break;
    }

    lv_obj_set_local_style_prop(BluetoothStatusLabel, LV_STYLE_TEXT_FONT, value, LV_PART_MAIN);
    lv_obj_align_to(BluetoothStatusLabel, BluetoothStatusCard, LV_ALIGN_BOTTOM_MID, 0, -10);
    xSemaphoreGive( lvgl_mutex );
}

void updateOnOffState(bool state, uint8_t device)
{
    lv_style_value_t value;

    if(state)
    {
        lv_label_set_text(OnOffStatusLabel[device], "ON");
        value.ptr = &lv_font_montserrat_16;
        lv_obj_set_style_img_recolor(OnOffImage[device],  lv_palette_lighten(LV_PALETTE_ORANGE, 1), 0);
    }
    else
    {
        lv_label_set_text(OnOffStatusLabel[device], "OFF");
        value.ptr = &lv_font_montserrat_16;
        lv_obj_set_style_img_recolor(OnOffImage[device], lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    }

    lv_obj_set_local_style_prop(OnOffStatusLabel[device], LV_STYLE_TEXT_FONT, value, LV_PART_MAIN);
    lv_obj_align_to(OnOffStatusLabel[device], OnOffStatusCard[device], LV_ALIGN_BOTTOM_MID, 0, -10);
}

void updateMatterChannel(uint16_t channel)
{
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    char buf[5];
    lv_snprintf(buf, sizeof(buf), "%d", channel);
    lv_label_set_text(infoChannelLabel, buf);
    xSemaphoreGive( lvgl_mutex );
}

void updateMatterPanID(uint16_t panId)
{
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    char buf[7];
    lv_snprintf(buf, sizeof(buf), "0x%X", panId);
    lv_label_set_text(infoPanIdLabel, buf);
    xSemaphoreGive( lvgl_mutex );
}

void updateMatterNetworkName(char * name)
{
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_label_set_text(infoNetworkNameLabel, name);
    xSemaphoreGive( lvgl_mutex );
}

void updateMatterIPV6Addr(uint16_t * addr)
{
    char buf[41];
    bool contiguous_zero = false;
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_snprintf(buf, sizeof(buf), "%04X", addr[0]);

    for(uint8_t i = 1; i<8; i++){
        if(addr[i] != 0){
            lv_snprintf(buf, sizeof(buf), "%s:%04X", buf, addr[i]);
            contiguous_zero = false;
        }
        else if(!contiguous_zero){
            lv_snprintf(buf, sizeof(buf), "%s:", buf);
            contiguous_zero = true;
        }
    }

    lv_label_set_text(infoIPV6AddrLabel, buf);
    xSemaphoreGive( lvgl_mutex );
}

#ifdef DISPLAY_MATTER_LOGS
void addMatterLogs(char * logs, uint16_t length, bool clear)
{
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);

    if(clear){
        lv_label_set_text(logLabel, textLogs);
    } else{
        if(strlen(lv_label_get_text(logLabel)) + length >= MAX_LOG_LENGTH){
            /* Need to remove first elem */
            lv_label_cut_text(logLabel, 0, length + 1);
        }
        lv_label_ins_text(logLabel, LV_LABEL_POS_LAST, "\r\n");
        lv_label_ins_text(logLabel, LV_LABEL_POS_LAST, textLogs);
    }
    int16_t scroll = lv_obj_get_scroll_bottom(TextLogsContainer);
    if(scroll > 0){
        lv_obj_scroll_by(TextLogsContainer, 0, -scroll , LV_ANIM_OFF);
    }

    xSemaphoreGive( lvgl_mutex );
}
#endif

void updateButtons(uint8_t count)
{
    for(int i = 0; i<count; i++)
    {
        lv_obj_clear_flag(OnOffStatusCard[i], LV_OBJ_FLAG_HIDDEN);
    }

    for(int i=count; i<light_count; i++)
    {
        lv_obj_add_flag(OnOffStatusCard[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void updateConnectionStatus(uint8_t device, bool isConnected)
{
    if(isConnected){
        lv_table_set_cell_value(devices_table, device+1, 2, "Connected");
    }
    else{
        lv_table_set_cell_value(devices_table, device+1, 2, "Connected");
    }
}

void updateNetworkType(uint8_t device, uint8_t state)
{
    switch(state){
        case EMBER_ZCL_INTERFACE_TYPE_UNSPECIFIED:{
            lv_table_set_cell_value(devices_table, device+1, 1, "Unspecified");
        } break;
        case EMBER_ZCL_INTERFACE_TYPE_WI_FI:{
            lv_table_set_cell_value(devices_table, device+1, 1, "Wi-Fi");
        } break;
        case EMBER_ZCL_INTERFACE_TYPE_ETHERNET:{
            lv_table_set_cell_value(devices_table, device+1, 1, "Ethernet");
        } break;
        case EMBER_ZCL_INTERFACE_TYPE_CELLULAR:{
            lv_table_set_cell_value(devices_table, device+1, 1, "LTE");
        } break;
        case EMBER_ZCL_INTERFACE_TYPE_THREAD:{
            lv_table_set_cell_value(devices_table, device+1, 1, "Thread");
        } break;
    }
}

void updateTable(uint8_t count)
{
    for(int i = 0; i<count; i++)
    {
        const EmberBindingTableEntry & entry = BindingTable::GetInstance().GetAt(i);
        snprintf(OnOffText, sizeof(OnOffText), "Light %d",i+1);

        lv_table_set_cell_value(devices_table, i+1, 0, (char*) OnOffText);
        lv_table_set_cell_value(devices_table, i+1, 1, "Unknown");
        lv_table_set_cell_value(devices_table, i+1, 2, "Unknown");

        if(entry.clusterId.Value() == chip::app::Clusters::OnOff::Id){
            lv_table_set_cell_value(devices_table, i+1, 3, "On-Off");
        }
        else{
            lv_table_set_cell_value(devices_table, i+1, 3, "Unknown");
        }
    }

    for(int i=count; i<light_count; i++)
    {
        lv_table_set_cell_value(devices_table, i+1, 0, "");
        lv_table_set_cell_value(devices_table, i+1, 1, "");
        lv_table_set_cell_value(devices_table, i+1, 2, "");
        lv_table_set_cell_value(devices_table, i+1, 3, "");
    }
}

static void draw_part_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    /*If the cells are drawn...*/
    if(dsc->part == LV_PART_ITEMS) {
        uint32_t row = dsc->id /  lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);

        /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_BLUE), dsc->rect_dsc->bg_color, LV_OPA_20);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
        /*In the first column align the texts to the right*/
        else if(col == 0) {
            dsc->label_dsc->flag = LV_TEXT_ALIGN_RIGHT;
        }

        /*Make every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), dsc->rect_dsc->bg_color, LV_OPA_10);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
    }
}
