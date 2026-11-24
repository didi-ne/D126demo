/*
 * Copyright (C) 2026 ArtInChip Technology Co., Ltd.
 *
 */

#include "custom.h"
#include "aic_log.h"
#include <string.h>
#include <time.h>

/* ========== Macro Definitions ========== */

#define NAV_IMAGE_COUNT     3
#define NAV_INFO_COUNT      3
#define DATA_TIMER_PERIOD   5000
#define TEST_TIMER_PERIOD   50
#define IMAGE_COUNT         5
#define SPEED_MAX           200
#define RPM_MAX             99
#define ARC_MAX             100

/* ========== Static Data ========== */

// Driving distance data
static const char *s_data_values[] = {"510", "420", "380", "350", "290"};
static const uint8_t s_data_value_count = 5;

// Navigation image paths
static const char *nav_image_paths[NAV_IMAGE_COUNT] = {
    LVGL_IMAGE_PATH(turn/straight2.png),
    LVGL_IMAGE_PATH(turn/left.png),
    LVGL_IMAGE_PATH(turn/right.png)
};

// Navigation hint text
static const char *nav_info_texts[NAV_INFO_COUNT] = {
    "直行进入珠海大桥",
    "左转进入香港路",
    "右转进入澳门大道"
};

// Gear images
static const char *image_array[IMAGE_COUNT] = {
    LVGL_IMAGE_PATH(gear/P.png),
    LVGL_IMAGE_PATH(gear/R.png),
    LVGL_IMAGE_PATH(gear/N.png),
    LVGL_IMAGE_PATH(gear/D.png),
    LVGL_IMAGE_PATH(gear/S.png)
};

/* ========== Static Variables ========== */

// Data management
static uint8_t s_data_index = 0;
static lv_timer_t *data_timer = NULL;

// Navigation info
static lv_obj_t *image_nav = NULL;
static lv_obj_t *label_nav_text = NULL;
static lv_obj_t *label_distance = NULL;

// Gear rotation
static lv_obj_t *image_obj = NULL;
static lv_timer_t *image_timer = NULL;
static uint8_t image_idx = 0;

// Test simulator
static lv_timer_t *test_timer = NULL;
static uint16_t test_speed = 0;
static uint8_t test_digit_low = 0;
static uint8_t test_digit_high = 0;

/* ========== Function Forward Declarations ========== */

// Utility functions
static lv_obj_t* get_valid_obj(lv_obj_t *obj);

// Data management
static const char* data_manager_get_value(void);
static void data_manager_next_value(void);
static void data_manager_init(void);
static void data_timer_cb(lv_timer_t *timer);

// Navigation info
static void get_nav_info_ui_objects(screen_2_t *scr);
static void nav_info_update_by_index(uint8_t idx);

// Gear rotation
static void image_rotate_cb(lv_timer_t *timer);
static void image1_slider_init(lv_obj_t *img_obj, uint32_t interval_ms);
static void image1_slider_stop(void);

// Test simulator
static void test_simulator_cb(lv_timer_t *timer);

// Display update
void screen_2_set_speed_real(uint16_t target);
void screen_2_set_rpm_real(uint16_t target);
void test_speed_simulator_init(void);
void screen_2_anim_cleanup(void);

/* ========== Utility Functions ========== */

static lv_obj_t* get_valid_obj(lv_obj_t *obj)
{
    return (obj && lv_obj_is_valid(obj)) ? obj : NULL;
}

/* ========== Data Management ========== */

static const char* data_manager_get_value(void)
{
    return s_data_values[s_data_index];
}

static void data_manager_next_value(void)
{
    s_data_index = (s_data_index + 1) % s_data_value_count;
}

static void data_manager_init(void)
{
    if (data_timer == NULL) {
        data_timer = lv_timer_create(data_timer_cb, DATA_TIMER_PERIOD, NULL);
    }
}

static void data_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    
    data_manager_next_value();
    const char *value = data_manager_get_value();
    uint8_t idx = s_data_index;
    
    screen_2_t *scr = screen_2_get(&ui_manager);
    if (!scr || !scr->obj || lv_scr_act() != scr->obj) {
        return;
    }
    
    aic_freeze_step("nav");
    FREEZE_DBG("nav %u\n", idx);
    if (label_distance && lv_obj_is_valid(label_distance)) {
        lv_label_set_text(label_distance, value);
    }
    
    nav_info_update_by_index(idx);
    aic_freeze_step("nav-done");
    FREEZE_DBG("nav done %u\n", idx);
}

/* ========== Navigation Info ========== */

static void get_nav_info_ui_objects(screen_2_t *scr)
{
    label_distance = get_valid_obj(scr->label_meter);
    label_nav_text = get_valid_obj(scr->label_address);
    image_nav = get_valid_obj(scr->image_straight);
}

static uint32_t nav_utf8_next(const char **ps)
{
    const uint8_t *p = (const uint8_t *)*ps;
    uint32_t cp;

    if (p[0] == 0) {
        return 0;
    }
    if (p[0] < 0x80) {
        *ps += 1;
        return p[0];
    }
    if ((p[0] & 0xE0) == 0xC0 && p[1]) {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *ps += 2;
        return cp;
    }
    if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *ps += 3;
        return cp;
    }
    *ps += 1;
    return 0;
}

/* 仪表盘出来约 5 秒时 data_timer_cb() 才第一次画「左转进入香港路」。
 * fs_droidsansfallback_10（导航用的 FreeType 字体）在那一帧现渲染新汉字，
 * 字模申请失败会把 LVGL 线程打崩。页面刚出来时内存还够，先把三句导航的字模渲进缓存。 */
static void preload_nav_font(void)
{
    const lv_font_t *font = fs_droidsansfallback_10;
    int i;

    if (!font || !font->get_glyph_dsc) {
        return;
    }

    for (i = 0; i < NAV_INFO_COUNT; i++) {
        const char *s = nav_info_texts[i];

        while (s && *s) {
            uint32_t cp = nav_utf8_next(&s);
            lv_font_glyph_dsc_t g;

            if (cp < 0x20) {
                continue;
            }
            lv_memzero(&g, sizeof(g));
            if (!lv_font_get_glyph_dsc(font, &g, cp, 0)) {
                continue;
            }
            if (lv_font_get_glyph_bitmap(&g, cp, NULL) && font->release_glyph) {
                font->release_glyph(font, &g);
            }
        }
    }
}

/* 转速 7.5 第一次用到 28px 的 '5'。字模如果留到那一帧再申请，刷新会在 rpm cb out 之后死掉。 */
static void preload_digit_font(const lv_font_t *font)
{
    uint32_t cp;

    if (!font || !font->get_glyph_dsc) {
        return;
    }
    for (cp = '0'; cp <= '9'; cp++) {
        lv_font_glyph_dsc_t g;

        lv_memzero(&g, sizeof(g));
        if (!lv_font_get_glyph_dsc(font, &g, cp, 0)) {
            continue;
        }
        if (lv_font_get_glyph_bitmap(&g, cp, NULL) && font->release_glyph) {
            font->release_glyph(font, &g);
        }
    }
}

static void nav_info_update_by_index(uint8_t idx)
{
    uint8_t nav_idx = idx % NAV_IMAGE_COUNT;
    
    if (image_nav && nav_image_paths[nav_idx]) {
        lv_img_set_src(image_nav, nav_image_paths[nav_idx]);
    }
    
    if (label_nav_text) {
        lv_label_set_text(label_nav_text, nav_info_texts[nav_idx]);
    }
}

/* ========== Gear Rotation ========== */

static void image_rotate_cb(lv_timer_t *timer)
{
    (void)timer;
    
    if (!image_obj || !image_array[image_idx]) {
        return;
    }

    /* set_src 会把路径复制一份，指针不相等。第一次 3 秒到时还是 P.png，再打开文件没有必要。 */
    {
        const char *cur = lv_image_get_src(image_obj);
        if (cur && strcmp(cur, image_array[image_idx]) == 0) {
            FREEZE_DBG("gear same %u\n", image_idx);
            image_idx = (image_idx + 1) % IMAGE_COUNT;
            return;
        }
    }

    aic_freeze_step("gear");
    FREEZE_DBG("gear -> %u\n", image_idx);
    lv_img_set_src(image_obj, image_array[image_idx]);
    image_idx = (image_idx + 1) % IMAGE_COUNT;
    aic_freeze_step("gear-done");
    FREEZE_DBG("gear done %u\n", image_idx);
}

static void image1_slider_init(lv_obj_t *img_obj, uint32_t interval_ms)
{
    if (!img_obj) {
        return;
    }
    
    image1_slider_stop();
    
    image_obj = img_obj;
    image_idx = 0;
    
    if (image_array[0]) {
        lv_img_set_src(image_obj, image_array[0]);
    }
    
    image_timer = lv_timer_create(image_rotate_cb, interval_ms, NULL);
}

static void image1_slider_stop(void)
{
    if (image_timer) {
        lv_timer_del(image_timer);
        image_timer = NULL;
    }
    image_obj = NULL;
    image_idx = 0;
}

/* ========== Display Update ========== */

void screen_2_set_speed_real(uint16_t target)
{
    screen_2_t *scr = screen_2_get(&ui_manager);
    if (!scr || !scr->arc_speed) {
        return;
    }
    
    if (target > SPEED_MAX) {
        target = SPEED_MAX;
    }
    
    uint16_t arc_value = (target * ARC_MAX) / SPEED_MAX;
    lv_arc_set_value(scr->arc_speed, arc_value);
    
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", target);
    lv_label_set_text(scr->label_speed, buf);
}

void screen_2_set_rpm_real(uint16_t target)
{
    screen_2_t *scr = screen_2_get(&ui_manager);
    if (!scr || !scr->arc_rmp) {
        return;
    }
    
    if (target > RPM_MAX) {
        target = RPM_MAX;
    }

    lv_arc_set_value(scr->arc_rmp, target);
    
    uint8_t left_digit = target / 10;
    uint8_t right_digit = target % 10;
    char buf[8];
    
    if (scr->label_left) {
        lv_snprintf(buf, sizeof(buf), "%d", left_digit);
        lv_label_set_text(scr->label_left, buf);
    }
    
    if (scr->label_right) {
        lv_snprintf(buf, sizeof(buf), "%d", right_digit);
        lv_label_set_text(scr->label_right, buf);
    }
}

/* ========== Test Simulator ========== */

static void test_simulator_cb(lv_timer_t *timer)
{
    (void)timer;

    uint16_t rpm = test_digit_high * 10 + test_digit_low;
    if (rpm > 100) {
        rpm = 100;
    }
    
    screen_2_set_speed_real(test_speed);
    screen_2_set_rpm_real(rpm);

    // Speed update
    if (test_speed < 50) {
        test_speed += 2;
    } else if (test_speed < 100) {
        test_speed += 5;
    } else if (test_speed < 150) {
        test_speed += 2;
    } else if (test_speed < SPEED_MAX) {
        test_speed += 5;
    } else {
        test_speed = 0;
    }

    // RPM update
    uint16_t rpm_temp = rpm;
    if (rpm_temp < 25) {
        rpm_temp += 1;
    } else if (rpm_temp < 50) {
        rpm_temp += 2;
    } else if (rpm_temp < 75) {
        rpm_temp += 1;
    } else if (rpm_temp < 99) {
        rpm_temp += 2;
    } else {
        rpm_temp = 0;
    }
    
    test_digit_high = rpm_temp / 10;
    test_digit_low = rpm_temp % 10;
}

void test_speed_simulator_init(void)
{
    if (test_timer) {
        return;
    }
    
    test_timer = lv_timer_create(test_simulator_cb, TEST_TIMER_PERIOD, NULL);

    screen_2_t *scr = screen_2_get(&ui_manager);
    if (scr && scr->image_P) {
        image1_slider_init(scr->image_P, 3000);
    }
    
    data_manager_init();
    get_nav_info_ui_objects(scr);
    
    if (label_distance) {
        lv_label_set_text(label_distance, data_manager_get_value());
    }
    
    nav_info_update_by_index(s_data_index);
    /* 只预渲染汉字。进页面时把导航和档位 PNG 再解一遍会把堆打散，仪表盘几秒后更容易死。 */
    aic_freeze_step("nav-font");
    FREEZE_DBG("nav font preload\n");
    preload_nav_font();
    preload_digit_font(fs_montserratmedium_28);
    preload_digit_font(fs_montserratmedium_22);
    aic_freeze_step("nav-font-done");
    FREEZE_DBG("nav font preload done\n");
}

/* ========== Cleanup ========== */

void screen_2_anim_cleanup(void)
{
    if (test_timer) {
        lv_timer_del(test_timer);
        test_timer = NULL;
        test_speed = 0;
        test_digit_low = 0;
        test_digit_high = 0;
    }
    
    image1_slider_stop();
    
    if (data_timer) {
        lv_timer_del(data_timer);
        data_timer = NULL;
    }
    
    image_nav = NULL;
    label_nav_text = NULL;
    label_distance = NULL;
}
