/*
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "lvgl.h"
#include "displayResources.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/



/*******************************************************************************
 *  STATIC VARIABLES
 ******************************************************************************/



/*******************************************************************************
 *  variables
 ******************************************************************************/
lv_style_t gTabStyle;
lv_style_t gInvisibleContainerStyle;
lv_style_t gCardWidgetStyle;
lv_style_t gCardInfoStyle;
lv_style_t gArcTempSelectorMainStyle;
lv_style_t gArcTempSelectorIndicatorStyle;
lv_style_t gTempGraphStyle;
lv_style_t gSmallTextStyle;
lv_style_t gMediumTextStyle;
lv_style_t gLargeTextStyle;
lv_style_t gBigTextStyle;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/



/*******************************************************************************
 * Code
 ******************************************************************************/
void lv_initResources(void)
{
	/* Tab Style */
    lv_style_init(&gTabStyle);
	lv_style_set_bg_color(&gTabStyle, lv_palette_lighten(LV_PALETTE_GREY, 4));
	lv_style_set_bg_opa(&gTabStyle, 255);
    lv_style_set_pad_row(&gTabStyle, 0);
    lv_style_set_pad_column(&gTabStyle, 0);
    lv_style_set_pad_left(&gTabStyle, 5);
    lv_style_set_pad_right(&gTabStyle, 5);
    lv_style_set_pad_top(&gTabStyle, 5);
    lv_style_set_pad_bottom(&gTabStyle, 5);
    lv_style_set_border_width(&gTabStyle, 0);
    lv_style_set_shadow_opa(&gTabStyle, 0);

	/* Invisible Style */
    lv_style_init(&gInvisibleContainerStyle);
	lv_style_set_bg_color(&gInvisibleContainerStyle, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_bg_opa(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_row(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_column(&gInvisibleContainerStyle, 0);
    lv_style_set_align(&gInvisibleContainerStyle, LV_ALIGN_BOTTOM_LEFT);
    lv_style_set_border_width(&gInvisibleContainerStyle, 0);
    lv_style_set_shadow_opa(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_left(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_right(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_top(&gInvisibleContainerStyle, 0);
    lv_style_set_pad_bottom(&gInvisibleContainerStyle, 0);

    /* Card Widget Style */
    lv_style_init(&gCardWidgetStyle);
	lv_style_set_bg_color(&gCardWidgetStyle, lv_palette_main(LV_PALETTE_GREY));
	lv_style_set_bg_opa(&gCardWidgetStyle, 100);
    lv_style_set_pad_row(&gCardWidgetStyle, 0);
    lv_style_set_pad_column(&gCardWidgetStyle, 0);
	lv_style_set_shadow_color(&gCardWidgetStyle, lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_shadow_opa(&gCardWidgetStyle, 150);
	lv_style_set_shadow_width(&gCardWidgetStyle, 10);
	lv_style_set_shadow_spread(&gCardWidgetStyle, 1);
    lv_style_set_align(&gCardWidgetStyle, LV_ALIGN_BOTTOM_LEFT);
    lv_style_set_border_width(&gCardWidgetStyle, 0);
    lv_style_set_pad_left(&gCardWidgetStyle, 0);
    lv_style_set_pad_right(&gCardWidgetStyle, 0);
    lv_style_set_pad_top(&gCardWidgetStyle, 0);
    lv_style_set_pad_bottom(&gCardWidgetStyle, 0);
    
    /* Card Info Style */
    lv_style_init(&gCardInfoStyle);
	lv_style_set_bg_color(&gCardInfoStyle, lv_palette_main(LV_PALETTE_GREY));
	lv_style_set_bg_opa(&gCardInfoStyle, 0);
    lv_style_set_pad_row(&gCardInfoStyle, 0);
    lv_style_set_pad_column(&gCardInfoStyle, 0);
	lv_style_set_shadow_color(&gCardInfoStyle, lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_shadow_opa(&gCardInfoStyle, 0);
	lv_style_set_shadow_width(&gCardInfoStyle, 0);
	lv_style_set_shadow_spread(&gCardInfoStyle, 0);
    lv_style_set_align(&gCardInfoStyle, LV_ALIGN_BOTTOM_LEFT);
    lv_style_set_border_width(&gCardInfoStyle, 0);
    lv_style_set_pad_left(&gCardInfoStyle, 0);
    lv_style_set_pad_right(&gCardInfoStyle, 0);
    lv_style_set_pad_top(&gCardInfoStyle, 0);
    lv_style_set_pad_bottom(&gCardInfoStyle, 0);

    /* Arc Temperature Selector Style */
    lv_style_init(&gArcTempSelectorMainStyle);
    lv_style_set_arc_color(&gArcTempSelectorMainStyle, lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_arc_opa(&gArcTempSelectorMainStyle, 80);
	lv_style_set_arc_width(&gArcTempSelectorMainStyle, 4);
	lv_style_set_shadow_color(&gArcTempSelectorMainStyle, lv_palette_darken(LV_PALETTE_GREY, 3));
	lv_style_set_shadow_opa(&gArcTempSelectorMainStyle, 150);
	lv_style_set_shadow_width(&gArcTempSelectorMainStyle, 10);
	lv_style_set_shadow_spread(&gArcTempSelectorMainStyle, 1);
	lv_style_set_bg_color(&gArcTempSelectorMainStyle, lv_palette_darken(LV_PALETTE_NONE, 1));
	lv_style_set_bg_opa(&gArcTempSelectorMainStyle, 0);
	lv_style_set_arc_rounded(&gArcTempSelectorMainStyle, false);
    lv_style_init(&gArcTempSelectorIndicatorStyle);
    lv_style_set_arc_color(&gArcTempSelectorIndicatorStyle, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
	lv_style_set_arc_opa(&gArcTempSelectorIndicatorStyle, 255);
	lv_style_set_arc_width(&gArcTempSelectorIndicatorStyle, 4);
	lv_style_set_shadow_opa(&gArcTempSelectorIndicatorStyle, 0);
	lv_style_set_arc_rounded(&gArcTempSelectorIndicatorStyle, false);

	/* Temperature Graph Style */
	 lv_style_init(&gTempGraphStyle);
	 lv_style_set_bg_color(&gTempGraphStyle, lv_color_white());
	 lv_style_set_bg_opa(&gTempGraphStyle, 100);

    /* Small Text Style */
    lv_style_init(&gSmallTextStyle);
    lv_style_set_text_font(&gSmallTextStyle, &lv_font_montserrat_12);
    lv_style_set_border_width(&gSmallTextStyle, 0);

    /* Medium Text Style */
    lv_style_init(&gMediumTextStyle);
    lv_style_set_text_font(&gMediumTextStyle, &lv_font_montserrat_20);
    lv_style_set_border_width(&gMediumTextStyle, 0);

    /* Bid Text Style */
    lv_style_init(&gBigTextStyle);
    lv_style_set_text_font(&gBigTextStyle, &lv_font_montserrat_40);
    lv_style_set_border_width(&gBigTextStyle, 0);

    lv_style_init(&gLargeTextStyle);
    lv_style_set_text_font(&gLargeTextStyle, &lv_font_montserrat_30);
    lv_style_set_border_width(&gLargeTextStyle, 0);
}
