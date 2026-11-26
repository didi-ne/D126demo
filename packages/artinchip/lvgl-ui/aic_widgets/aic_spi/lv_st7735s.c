/*
 * Copyright (C) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TFT106B001 1.06" (ST7735SV / BOE TN), 96x160, SPI + D/C on PC.1 (J6 IO1).
 * Init from ST7735SV+BOE1.06TN-2.2gamma-20210709.ini
 */

#include <rtdevice.h>
#include "lv_aic_spi.h"

#define ST7735S_RST_PIN "PD.10"

/* GRAM window：厂家 CASET 0x12..0x71，RASET 0x01..0xA0 → 96x160 */
#define ST7735S_X_OFFSET 0x12
#define ST7735S_Y_OFFSET 0x01
#define ST7735S_WIDTH    96
#define ST7735S_HEIGHT   160

static void lv_spi_panel(struct lv_spi_dev *dev)
{
    u32 rst_pin;

    rst_pin = rt_pin_get(ST7735S_RST_PIN);
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    /* HW reset */
    rt_pin_write(rst_pin, 0);
    rt_thread_mdelay(20);
    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(120);

    lv_spi_write_seq(dev, 0x11); /* Sleep out */
    rt_thread_mdelay(120);

    lv_spi_write_seq(dev, 0xB1, 0x05, 0x3C, 0x3C);
    lv_spi_write_seq(dev, 0xB2, 0x05, 0x3C, 0x3C);
    lv_spi_write_seq(dev, 0xB3, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C);
    lv_spi_write_seq(dev, 0xB4, 0x03); /* Dot inversion */

    lv_spi_write_seq(dev, 0xC0, 0x88, 0x08, 0x84);
    lv_spi_write_seq(dev, 0xC1, 0xC4);
    lv_spi_write_seq(dev, 0xC2, 0x0D, 0x00);
    lv_spi_write_seq(dev, 0xC3, 0x8D, 0x2A);
    lv_spi_write_seq(dev, 0xC4, 0x8D, 0xEE);
    lv_spi_write_seq(dev, 0xC5, 0x05); /* VCOM */

    lv_spi_write_seq(dev, 0xE0,
                     0x13, 0x1E, 0x0F, 0x14, 0x28, 0x21, 0x1A, 0x1E,
                     0x1E, 0x20, 0x2A, 0x38, 0x00, 0x0C, 0x01, 0x10);
    lv_spi_write_seq(dev, 0xE1,
                     0x16, 0x1C, 0x0E, 0x17, 0x30, 0x2B, 0x26, 0x2B,
                     0x2B, 0x27, 0x2F, 0x3C, 0x00, 0x0E, 0x04, 0x10);

    lv_spi_write_seq(dev, 0x35, 0x00); /* TE off */
    lv_spi_write_seq(dev, 0x3A, 0x05); /* RGB565 */
    lv_spi_write_seq(dev, 0x36, 0xC8); /* BGR + MX/MY（厂家稿） */

    lv_spi_write_seq(dev, 0x29); /* Display on */

    /* Full panel window with vendor offsets */
    lv_spi_write_seq(dev, 0x2A,
                     0x00, ST7735S_X_OFFSET,
                     0x00, (u8)(ST7735S_X_OFFSET + ST7735S_WIDTH - 1));
    lv_spi_write_seq(dev, 0x2B,
                     0x00, ST7735S_Y_OFFSET,
                     0x00, (u8)(ST7735S_Y_OFFSET + ST7735S_HEIGHT - 1));
    lv_spi_write_seq(dev, 0x2C);

    rt_thread_mdelay(10);
}

struct lv_spi_panel_driver lv_spi_st7735s = {
    .name = "st7735s",
    .enable = lv_spi_panel,
    .width = ST7735S_WIDTH,
    .height = ST7735S_HEIGHT,
};
