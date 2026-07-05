/* Multi-touch LVGL input device for the GT911 touch controller.
 *
 * Replaces lvgl_port_add_touch(): esp_lvgl_port's read callback only
 * reports a single touch point, so LVGL's pinch recognizer never sees a
 * second finger and LV_EVENT_GESTURE is never emitted. This driver reads
 * all GT911 contacts and feeds them through
 * lv_indev_gesture_recognizers_update(), enabling pinch zoom while
 * preserving normal single-finger pointer behaviour. */
#pragma once

#include "esp_lcd_touch.h"
#include "lvgl.h"

/* Create and register the multi-touch LVGL indev. Returns NULL on error. */
lv_indev_t *touch_gesture_add_indev(lv_display_t *disp, esp_lcd_touch_handle_t tp);
