/*
 * Copyright (C) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TFT042B008 0.42" (ST7735P3 / BOE), 96x54, SPI + D/C on PC.1 (J6 IO1).
 * Init from BOE0.42+ST7735P3-gamma2.2.ini
 */

#include <rtdevice.h>
#include "lv_aic_spi.h"

#define ST7735S_RST_PIN "PD.10"

/* GRAM window：厂家 CASET 0x10..0x6F，RASET 0x6A..0x9F → 96x54 */
#define ST7735S_X_OFFSET 0x10
#define ST7735S_Y_OFFSET 0x6A
#define ST7735S_WIDTH    96
#define ST7735S_HEIGHT   54

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

    lv_spi_write_seq(dev, 0xB1, 0x05, 0x3A, 0x3A);
    lv_spi_write_seq(dev, 0xB2, 0x05, 0x3A, 0x3A);
    lv_spi_write_seq(dev, 0xB3, 0x05, 0x3A, 0x3A, 0x05, 0x3A, 0x3A);
    lv_spi_write_seq(dev, 0xB4, 0x03); /* inversion */

    lv_spi_write_seq(dev, 0xC0, 0x62, 0x02, 0x84);
    lv_spi_write_seq(dev, 0xC1, 0xC0);
    lv_spi_write_seq(dev, 0xC2, 0x0D, 0x00);
    lv_spi_write_seq(dev, 0xC3, 0x8D, 0xEA);
    lv_spi_write_seq(dev, 0xC4, 0x8B, 0xEE);
    lv_spi_write_seq(dev, 0xC5, 0x0F); /* VCOM */

    lv_spi_write_seq(dev, 0x36, 0xC8); /* MX, MY, RGB（厂家稿） */

    lv_spi_write_seq(dev, 0xE0,
                     0x11, 0x0E, 0x08, 0x00, 0x18, 0x0F, 0x09, 0x09,
                     0x0A, 0x0D, 0x0F, 0x36, 0x00, 0x0B, 0x04, 0x3F);
    lv_spi_write_seq(dev, 0xE1,
                     0x0E, 0x0A, 0x05, 0x00, 0x0E, 0x06, 0x00, 0x01,
                     0x02, 0x05, 0x12, 0x32, 0x00, 0x0C, 0x05, 0x3F);

    lv_spi_write_seq(dev, 0x3A, 0x05); /* RGB565 */

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
