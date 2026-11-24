/*
 * Copyright (C) 2026 ArtInChip Technology Co., Ltd.
 *
 * 动画原片(animImg / lv_animimg)只有切帧 API，没有“覆盖过渡”参数。
 * SCREEN1_COVER_SCROLL=1 时：停用其内置播放，读取其图片列表，双图层实现：
 * 后一张从上往下 2s 滑入覆盖，再展示 1s。16 × (2+1) = 48000ms。
 * SCREEN1_COVER_SCROLL=0 时：显示仍用 animImg_1，但不用其 0..N-1 插值
 * （末帧几乎无时长），改为按 duration/N 均分切帧。
 */

#include "custom.h"

/* 0=直接切帧(animImg 显示)，1=上往下覆盖滚动 */
#define SCREEN1_COVER_SCROLL  0

/* 下次 screen_1 加载时从这张开始。s_boot_last 优先，用完即清。 */
static uint8_t s_boot_idx = 0;
static uint8_t s_boot_last = 0;

static uint8_t slideshow_boot_index(uint8_t count)
{
    uint8_t idx = 0;

    if (count == 0) {
        s_boot_last = 0;
        s_boot_idx = 0;
        return 0;
    }
    if (s_boot_last) {
        idx = (uint8_t)(count - 1);
    } else if (s_boot_idx < count) {
        idx = s_boot_idx;
    }
    s_boot_last = 0;
    s_boot_idx = 0;
    return idx;
}

#if SCREEN1_COVER_SCROLL == 0

static lv_obj_t *s_animimg = NULL;
static lv_timer_t *s_frame_timer = NULL;
static const void **s_srcs = NULL;
static uint8_t s_count = 0;
static uint8_t s_idx = 0;
static uint32_t s_frame_ms = 0;

static void frame_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (!s_animimg || !s_srcs || s_count == 0) {
        return;
    }

    if ((uint16_t)s_idx + 1 >= s_count) {
        if (s_frame_timer) {
            lv_timer_del(s_frame_timer);
            s_frame_timer = NULL;
        }
        return;
    }

    s_idx++;
    lv_img_set_src(s_animimg, s_srcs[s_idx]);
}

#elif SCREEN1_COVER_SCROLL != 0

#define COVER_MS        2000
#define HOLD_MS         1000
#define SLIDE_W         480
#define SLIDE_H         272

static lv_obj_t *s_img_base = NULL;
static lv_obj_t *s_img_cover = NULL;
static lv_timer_t *s_hold_timer = NULL;
static const void **s_srcs = NULL;
static uint8_t s_count = 0;
static uint8_t s_idx = 0;
static bool s_running = false;

static void start_cover(uint8_t idx);
static void cover_y_exec(void *obj, int32_t v);
static void cover_ready(lv_anim_t *a);
static void hold_timer_cb(lv_timer_t *t);

static void cover_y_exec(void *obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)obj, (lv_coord_t)v);
}

static void hold_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_hold_timer = NULL;

    if (!s_running) {
        return;
    }

    if ((uint16_t)s_idx + 1 >= s_count) {
        return;
    }

    start_cover((uint8_t)(s_idx + 1));
}

static void cover_ready(lv_anim_t *a)
{
    (void)a;
    if (!s_running || !s_img_base || !s_img_cover || !s_srcs) {
        return;
    }

    lv_img_set_src(s_img_base, s_srcs[s_idx]);
    lv_obj_set_y(s_img_cover, -SLIDE_H);

    if (s_hold_timer) {
        lv_timer_del(s_hold_timer);
        s_hold_timer = NULL;
    }
    s_hold_timer = lv_timer_create(hold_timer_cb, HOLD_MS, NULL);
    lv_timer_set_repeat_count(s_hold_timer, 1);
}

static void start_cover(uint8_t idx)
{
    if (!s_running || !s_img_cover || !s_srcs || idx >= s_count) {
        return;
    }

    s_idx = idx;
    lv_img_set_src(s_img_cover, s_srcs[idx]);
    lv_obj_set_y(s_img_cover, -SLIDE_H);
    lv_obj_clear_flag(s_img_cover, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_img_cover);

    lv_anim_del(s_img_cover, cover_y_exec);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, s_img_cover);
    lv_anim_set_values(&anim, -SLIDE_H, 0);
    lv_anim_set_time(&anim, COVER_MS);
    lv_anim_set_exec_cb(&anim, cover_y_exec);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_ready_cb(&anim, cover_ready);
    lv_anim_start(&anim);
}

#endif /* SCREEN1_COVER_SCROLL */

void screen_1_slideshow_start(void)
{
#if SCREEN1_COVER_SCROLL == 0
    /* 直接变：不隐藏 animImg_1，停掉其有 BUG 的内置插值，按均分时长切帧 */
    screen_1_t *scr = screen_1_get(&ui_manager);
    uint32_t dur;
    uint32_t per_ms;

    if (!scr || !scr->animImg_1) {
        return;
    }

    screen_1_anim_cleanup();

    s_srcs = lv_animimg_get_src(scr->animImg_1);
    s_count = lv_animimg_get_src_count(scr->animImg_1);
    if (!s_srcs || s_count == 0) {
        return;
    }

    dur = lv_animimg_get_duration(scr->animImg_1);
    if (dur == 0) {
        dur = 48000;
    }
    per_ms = dur / s_count;
    if (per_ms == 0) {
        per_ms = 1;
    }

    /* 停用内置 0..N-1 插值动画，控件本身继续显示 */
    lv_animimg_set_repeat_count(scr->animImg_1, 0);
    lv_anim_del(scr->animImg_1, NULL);
    lv_obj_clear_flag(scr->animImg_1, LV_OBJ_FLAG_HIDDEN);

    s_animimg = scr->animImg_1;
    s_frame_ms = per_ms;
    s_idx = slideshow_boot_index(s_count);
    lv_img_set_src(s_animimg, s_srcs[s_idx]);

    s_frame_timer = lv_timer_create(frame_timer_cb, per_ms, NULL);
#else
    screen_1_t *scr = screen_1_get(&ui_manager);
    if (!scr || !scr->obj || !scr->animImg_1) {
        return;
    }

    screen_1_anim_cleanup();

    /* 停用动画原片内置切帧 */
    lv_animimg_set_repeat_count(scr->animImg_1, 0);
    lv_anim_del(scr->animImg_1, NULL);
    lv_obj_add_flag(scr->animImg_1, LV_OBJ_FLAG_HIDDEN);

    s_srcs = lv_animimg_get_src(scr->animImg_1);
    s_count = lv_animimg_get_src_count(scr->animImg_1);
    if (!s_srcs || s_count == 0) {
        return;
    }

    s_running = true;
    s_idx = slideshow_boot_index(s_count);

    s_img_base = lv_img_create(ui_screen_content(scr->obj));
    lv_obj_set_pos(s_img_base, 0, 0);
    lv_obj_set_size(s_img_base, SLIDE_W, SLIDE_H);

    s_img_cover = lv_img_create(ui_screen_content(scr->obj));
    lv_obj_set_size(s_img_cover, SLIDE_W, SLIDE_H);
    lv_obj_set_pos(s_img_cover, 0, -SLIDE_H);

    start_cover(s_idx);
#endif
}

/* 推图时停住当前这一帧。不清图片，QSPI 停在正在显示的画面上。 */
void screen_1_slideshow_pause(void)
{
#if SCREEN1_COVER_SCROLL == 0
    if (s_frame_timer) {
        lv_timer_pause(s_frame_timer);
    }
#else
    s_running = false;
    if (s_hold_timer) {
        lv_timer_pause(s_hold_timer);
    }
    if (s_img_cover) {
        lv_anim_del(s_img_cover, cover_y_exec);
    }
#endif
}

/* 从暂停的这一张接着播，不回到第一张。重新计这一张的间隔，避免一恢复就切走。 */
void screen_1_slideshow_resume(void)
{
#if SCREEN1_COVER_SCROLL == 0
    if (s_frame_timer) {
        lv_timer_resume(s_frame_timer);
        lv_timer_reset(s_frame_timer);
    }
#else
    if (!s_srcs || s_count == 0) {
        return;
    }
    s_running = true;
    if (s_hold_timer) {
        lv_timer_resume(s_hold_timer);
        lv_timer_reset(s_hold_timer);
    }
#endif
}

void screen_1_anim_cleanup(void)
{
#if SCREEN1_COVER_SCROLL == 0
    if (s_frame_timer) {
        lv_timer_del(s_frame_timer);
        s_frame_timer = NULL;
    }
    if (s_animimg) {
        lv_anim_del(s_animimg, NULL);
    }
    s_animimg = NULL;
    s_srcs = NULL;
    s_count = 0;
    s_idx = 0;
#else
    s_running = false;

    if (s_img_cover) {
        lv_anim_del(s_img_cover, cover_y_exec);
    }

    if (s_hold_timer) {
        lv_timer_del(s_hold_timer);
        s_hold_timer = NULL;
    }

    s_img_base = NULL;
    s_img_cover = NULL;
    s_srcs = NULL;
    s_count = 0;
    s_idx = 0;
#endif
}

void screen_1_slideshow_set_start(uint8_t idx)
{
    s_boot_idx = idx;
    s_boot_last = 0;
}

void screen_1_slideshow_set_start_last(void)
{
    s_boot_last = 1;
}

uint8_t screen_1_slideshow_index(void)
{
    return s_idx;
}

uint8_t screen_1_slideshow_count(void)
{
    return s_count;
}

uint32_t screen_1_slideshow_frame_ms(void)
{
#if SCREEN1_COVER_SCROLL == 0
    return s_frame_ms;
#else
    return COVER_MS + HOLD_MS;
#endif
}

int screen_1_slideshow_show(uint8_t idx)
{
#if SCREEN1_COVER_SCROLL == 0
    if (!s_animimg || !s_srcs || idx >= s_count) {
        return 0;
    }

    s_idx = idx;
    lv_img_set_src(s_animimg, s_srcs[s_idx]);
    if (s_frame_timer) {
        lv_timer_resume(s_frame_timer);
        lv_timer_reset(s_frame_timer);
    } else if (s_frame_ms) {
        s_frame_timer = lv_timer_create(frame_timer_cb, s_frame_ms, NULL);
    }
    return 1;
#else
    if (!s_running || !s_srcs || idx >= s_count) {
        return 0;
    }
    start_cover(idx);
    return 1;
#endif
}
