/*
 * Copyright (C) 2026 ArtInChip Technology Co., Ltd.
 *
 * TFT042B008 0.42" ST7735P3：96x54，仓鼠 / 六角恐龙 / 猫，每张 5 秒。
 */

#include "ui_objects.h"
#include "aic_ui.h"
#include "ui_util.h"
#ifdef LV_SPI_ST7735S
#include "slide_img.h"
#include "lv_aic_spi.h"
#endif

void __attribute__((weak)) screen_1_slideshow_start(void)
{
}

static void screen_1_loaded(lv_event_t *e)
{
    LV_UNUSED(e);
#ifndef LV_SPI_ST7735S
    screen_1_slideshow_start();
#endif
}

#ifdef LV_SPI_ST7735S
#define CAT_SLIDE_MS 5000

static const lv_image_dsc_t * const s_cat_imgs[] = {
    &img_hamster,
    &img_axolotl,
    &img_cat,
};

static lv_obj_t *s_cat_img_obj;
static uint8_t s_cat_idx;
static lv_timer_t *s_cat_timer;
static uint8_t s_cat_bl_ready;

static void cat_slide_show(uint8_t idx)
{
    if (!s_cat_img_obj)
        return;
    s_cat_idx = idx % (sizeof(s_cat_imgs) / sizeof(s_cat_imgs[0]));
    lv_img_set_src(s_cat_img_obj, s_cat_imgs[s_cat_idx]);
}

static void cat_slide_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    /* 背光未开时不切图，避免首帧时间被吃掉 */
    if (!s_cat_bl_ready) {
        if (!lv_spi_is_backlight_on(0))
            return;
        s_cat_bl_ready = 1;
        lv_timer_set_period(s_cat_timer, CAT_SLIDE_MS);
        lv_timer_reset(s_cat_timer);
        return;
    }

    cat_slide_show(s_cat_idx + 1);
}
#endif

void screen_1_create(ui_manager_t *ui)
{
    screen_1_t *scr = screen_1_get(ui);

    if (!ui->auto_del && scr->obj)
        return;

    scr->obj = lv_obj_create(NULL);
    lv_obj_set_scrollbar_mode(scr->obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr->obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr->obj, screen_1_loaded, LV_EVENT_SCREEN_LOADED, NULL);

#ifdef LV_SPI_ST7735S
    /* 小屏原生 96x54：不要走 ui_fit_to_panel（那是 360 圆屏缩放） */
    lv_obj_remove_style_all(scr->obj);
    lv_obj_set_style_bg_color(scr->obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr->obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(scr->obj, 0, LV_PART_MAIN);
    lv_obj_set_size(scr->obj, 96, 54);

    scr->animImg_1 = lv_img_create(scr->obj);
    lv_obj_remove_style_all(scr->animImg_1);
    lv_img_set_src(scr->animImg_1, &img_hamster);
    lv_obj_set_pos(scr->animImg_1, 0, 0);
    lv_obj_set_size(scr->animImg_1, 96, 54);
    lv_image_set_inner_align(scr->animImg_1, LV_IMAGE_ALIGN_DEFAULT);

    s_cat_img_obj = scr->animImg_1;
    s_cat_idx = 0;
    s_cat_bl_ready = 0;
    if (s_cat_timer) {
        lv_timer_del(s_cat_timer);
        s_cat_timer = NULL;
    }
    /* 先短周期轮询背光；背光就绪后再按 5s 切图 */
    s_cat_timer = lv_timer_create(cat_slide_timer_cb, 100, NULL);
#else
    lv_obj_set_style_bg_color(scr->obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    scr->animImg_1 = lv_animimg_create(scr->obj);
    static const char *animImg_1_imgs[16] = {
        LVGL_IMAGE_PATH(screen1/red.png),
        LVGL_IMAGE_PATH(screen1/green.png),
        LVGL_IMAGE_PATH(screen1/blue.png),
        LVGL_IMAGE_PATH(screen1/white.png),
        LVGL_IMAGE_PATH(screen1/black.png),
        LVGL_IMAGE_PATH(screen1/18-gray_scale_crosswise.png),
        LVGL_IMAGE_PATH(screen1/10-gray_scale_vertical.png),
        LVGL_IMAGE_PATH(screen1/color_bar.png),
        LVGL_IMAGE_PATH(screen1/gradient.png),
        LVGL_IMAGE_PATH(screen1/picture_1.png),
        LVGL_IMAGE_PATH(screen1/picture_2.png),
        LVGL_IMAGE_PATH(screen1/picture_3.png),
        LVGL_IMAGE_PATH(screen1/picture_4.png),
        LVGL_IMAGE_PATH(screen1/picture_5.png),
        LVGL_IMAGE_PATH(screen1/picture_6.png),
        LVGL_IMAGE_PATH(screen1/picture_7.png),
    };
    lv_animimg_set_src(scr->animImg_1, (const void **)animImg_1_imgs, 16);
    lv_animimg_set_duration(scr->animImg_1, 51000);
    lv_animimg_set_repeat_count(scr->animImg_1, 1);
    lv_animimg_start(scr->animImg_1);
    lv_obj_set_pos(scr->animImg_1, 0, 0);
    lv_obj_set_size(scr->animImg_1, 480, 272);

    ui_fit_to_panel(scr->obj);
#endif
}
