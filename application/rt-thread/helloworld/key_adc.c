/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * K2/K5/K6/K7 share GPADC7. Centers are measured on D126-DEMO, not the
 * ideal divider ratio. Released sits at about 4090.
 *   K2 -> 413    (406-419)
 *   K5 -> 846    (840-851)
 *   K6 -> 1351   (1346-1356)
 *   K7 -> 2113   (2109-2116)
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "aic_osal.h"

extern void qspi_carousel_key_step(int dir);
extern void qspi_carousel_key_long(void);
extern int qspi_carousel_is_paused(void);

#ifdef AIC_USING_GPAI7

#define KEY_ADC_NAME        "gpai"
#define KEY_ADC_CH          7
#define KEY_ADC_WIN         120
#define KEY_ADC_IDLE        3000
#define KEY_ADC_PERIOD_MS   20
#define KEY_ADC_DEBOUNCE    3
#define KEY_ADC_LONG_MS     3000

enum {
    KEY_NONE = 0,
    KEY_K2,
    KEY_K5,
    KEY_K6,
    KEY_K7,
    KEY_UNKNOWN,
};

static rt_adc_device_t key_adc;
static int g_key = KEY_NONE;

static int key_in_win(int adc, int center)
{
    return (adc >= center - KEY_ADC_WIN) && (adc <= center + KEY_ADC_WIN);
}

static int key_from_adc(int adc)
{
    if (key_in_win(adc, 413))
        return KEY_K2;
    if (key_in_win(adc, 846))
        return KEY_K5;
    if (key_in_win(adc, 1351))
        return KEY_K6;
    if (key_in_win(adc, 2113))
        return KEY_K7;
    if (adc >= KEY_ADC_IDLE)
        return KEY_NONE;
    return KEY_UNKNOWN;
}

static const char *key_kn(int key)
{
    switch (key) {
    case KEY_K2:
        return "K2";
    case KEY_K5:
        return "K5";
    case KEY_K6:
        return "K6";
    case KEY_K7:
        return "K7";
    default:
        return "none";
    }
}

static int key_is_dir(int key)
{
    return key == KEY_K2 || key == KEY_K5 || key == KEY_K6 || key == KEY_K7;
}

static void key_fire_short(int key)
{
    if (key == KEY_K6 || key == KEY_K2)
        qspi_carousel_key_step(1);
    else if (key == KEY_K5 || key == KEY_K7)
        qspi_carousel_key_step(-1);
}

static const char *key_dir(int key)
{
    switch (key) {
    case KEY_K5:
        return "Up";
    case KEY_K6:
        return "Down";
    case KEY_K7:
        return "Left";
    case KEY_K2:
        return "Right";
    default:
        return "none";
    }
}

static void key_adc_thread(void *arg)
{
    int pending = KEY_NONE;
    int count = 0;
    int long_sent = 0;
    rt_tick_t press_tick = 0;

    (void)arg;

    while (1) {
        int adc = (int)rt_adc_read(key_adc, KEY_ADC_CH);
        int raw = key_from_adc(adc);

        /* 按住时偶发的野值不算松开，否则 3 秒计时会被掐断，只剩短按。 */
        if (key_is_dir(g_key) && raw == KEY_UNKNOWN)
            raw = g_key;

        if (raw == pending) {
            if (count < KEY_ADC_DEBOUNCE)
                count++;
        } else {
            pending = raw;
            count = 1;
        }

        /* Accept only after N identical samples, and only if it differs from the last key. */
        if (count >= KEY_ADC_DEBOUNCE && pending != g_key) {
            int prev = g_key;

            g_key = pending;
            /* 松开时还没到长按，才算短按翻页。长按已经处理过就不再翻。 */
            if (key_is_dir(prev) && !long_sent)
                key_fire_short(prev);
            long_sent = 0;
            if (key_is_dir(g_key))
                press_tick = rt_tick_get();
#ifndef AIC_KEYADC_DEBUG
            if (g_key == KEY_NONE)
                rt_kprintf("[keyadc] release adc=%d\n", adc);
            else if (g_key != KEY_UNKNOWN)
                rt_kprintf("[keyadc] press %s %s adc=%d\n",
                           key_kn(g_key), key_dir(g_key), adc);
#endif
        } else if (count >= KEY_ADC_DEBOUNCE && key_is_dir(g_key) &&
                   !long_sent &&
                   (rt_tick_get() - press_tick) >= rt_tick_from_millisecond(KEY_ADC_LONG_MS)) {
            /* 满 3 秒一律打印。推图暂停才继续轮播，平时长按没有动作。 */
            long_sent = 1;
            if (qspi_carousel_is_paused())
                qspi_carousel_key_long();
#ifndef AIC_KEYADC_DEBUG
            rt_kprintf("[keyadc] long %s %s adc=%d\n",
                       key_kn(g_key), key_dir(g_key), adc);
#endif
        }

#ifdef AIC_KEYADC_DEBUG
        rt_kprintf("[keyadc] adc=%d key=%s %s\n", adc, key_kn(raw), key_dir(raw));
#endif
        aicos_msleep(KEY_ADC_PERIOD_MS);
    }
}

static int key_adc_init(void)
{
    key_adc = (rt_adc_device_t)rt_device_find(KEY_ADC_NAME);
    if (!key_adc) {
        rt_kprintf("[keyadc] gpai device not found\n");
        return -1;
    }

    if (rt_adc_enable(key_adc, KEY_ADC_CH) != RT_EOK) {
        rt_kprintf("[keyadc] enable ch%d failed\n", KEY_ADC_CH);
        return -1;
    }

#ifdef AIC_KEYADC_DEBUG
    rt_kprintf("[keyadc] GPADC7 sampling, K5 Up, K6 Down, K7 Left, K2 Right\n");
#endif

    if (!aicos_thread_create("keyadc", 2048, 25, key_adc_thread, NULL)) {
        rt_kprintf("[keyadc] create thread failed\n");
        return -1;
    }

    return 0;
}

INIT_APP_EXPORT(key_adc_init);

#endif /* AIC_USING_GPAI7 */
