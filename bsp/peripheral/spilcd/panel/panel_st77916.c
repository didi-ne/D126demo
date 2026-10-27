/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui.mai <huahui.mai@artinchip.com>
 */

#include <rtdef.h>
#include "aic_spi_lcd.h"

static void panel_st77916_enable(aic_spi_lcd_dev_t *dev)
{
    aic_spi_lcd_write_seq(dev, 0xF0, 0x28);
    aic_spi_lcd_write_seq(dev, 0xF2, 0x28);
    aic_spi_lcd_write_seq(dev, 0x73, 0xF0);
    aic_spi_lcd_write_seq(dev, 0x76, 0x0F);
    aic_spi_lcd_write_seq(dev, 0x7C, 0xD1);
    aic_spi_lcd_write_seq(dev, 0x83, 0xE0);
    aic_spi_lcd_write_seq(dev, 0x84, 0x61);
    aic_spi_lcd_write_seq(dev, 0xF2, 0x82);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x01);
    aic_spi_lcd_write_seq(dev, 0xF1, 0x01);
    aic_spi_lcd_write_seq(dev, 0xB0, 0x52);
    aic_spi_lcd_write_seq(dev, 0xB1, 0x49);
    aic_spi_lcd_write_seq(dev, 0xB2, 0x24);
    aic_spi_lcd_write_seq(dev, 0xB3, 0x01);
    aic_spi_lcd_write_seq(dev, 0xB4, 0x66);
    aic_spi_lcd_write_seq(dev, 0xB5, 0x44);
    aic_spi_lcd_write_seq(dev, 0xB6, 0xC5);
    aic_spi_lcd_write_seq(dev, 0xB7, 0x40);
    aic_spi_lcd_write_seq(dev, 0xB8, 0x86);
    aic_spi_lcd_write_seq(dev, 0xB9, 0x15);
    aic_spi_lcd_write_seq(dev, 0xBA, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBB, 0x08);
    aic_spi_lcd_write_seq(dev, 0xBC, 0x08);
    aic_spi_lcd_write_seq(dev, 0xBD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBF, 0x07);
    aic_spi_lcd_write_seq(dev, 0xC0, 0x80);
    aic_spi_lcd_write_seq(dev, 0xC1, 0x10);
    aic_spi_lcd_write_seq(dev, 0xC2, 0x37);
    aic_spi_lcd_write_seq(dev, 0xC3, 0x80);
    aic_spi_lcd_write_seq(dev, 0xC4, 0x10);
    aic_spi_lcd_write_seq(dev, 0xC5, 0x37);
    aic_spi_lcd_write_seq(dev, 0xC6, 0xA9);
    aic_spi_lcd_write_seq(dev, 0xC7, 0x41);
    aic_spi_lcd_write_seq(dev, 0xC8, 0x01);
    aic_spi_lcd_write_seq(dev, 0xC9, 0xA9);
    aic_spi_lcd_write_seq(dev, 0xCA, 0x41);
    aic_spi_lcd_write_seq(dev, 0xCB, 0x01);
    aic_spi_lcd_write_seq(dev, 0xCC, 0x7F);
    aic_spi_lcd_write_seq(dev, 0xCD, 0x7F);
    aic_spi_lcd_write_seq(dev, 0xCE, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xD0, 0x91);
    aic_spi_lcd_write_seq(dev, 0xD1, 0x68);
    aic_spi_lcd_write_seq(dev, 0xD2, 0x68);
    aic_spi_lcd_write_seq(dev, 0xF5, 0x00, 0xA5);
    aic_spi_lcd_write_seq(dev, 0xF1, 0x10);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x02);
    aic_spi_lcd_write_seq(dev, 0xE0, 0xF0, 0x0E, 0x14, 0x0B, 0x0B, 0x16, 0x3A, 0x44, 0x4E, 0x18, 0x14, 0x13, 0x2F, 0x35);
    aic_spi_lcd_write_seq(dev, 0xE1, 0xF0, 0x0D, 0x13, 0x0B, 0x0A, 0x16, 0x39, 0x43, 0x4E, 0x17, 0x13, 0x13, 0x2E, 0x34);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x10);
    aic_spi_lcd_write_seq(dev, 0xF3, 0x10);
    aic_spi_lcd_write_seq(dev, 0xE0, 0x09);
    aic_spi_lcd_write_seq(dev, 0xE1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE2, 0x03);
    aic_spi_lcd_write_seq(dev, 0xE3, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE4, 0xE0);
    aic_spi_lcd_write_seq(dev, 0xE5, 0x06);
    aic_spi_lcd_write_seq(dev, 0xE6, 0x21);
    aic_spi_lcd_write_seq(dev, 0xE7, 0x00);
    aic_spi_lcd_write_seq(dev, 0xE8, 0x05);
    aic_spi_lcd_write_seq(dev, 0xE9, 0x82);
    aic_spi_lcd_write_seq(dev, 0xEA, 0xDE);
    aic_spi_lcd_write_seq(dev, 0xEB, 0xC0);
    aic_spi_lcd_write_seq(dev, 0xEC, 0x40);
    aic_spi_lcd_write_seq(dev, 0xED, 0x84);
    aic_spi_lcd_write_seq(dev, 0xEE, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xEF, 0x71);
    aic_spi_lcd_write_seq(dev, 0xF8, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xF9, 0x50);
    aic_spi_lcd_write_seq(dev, 0xFA, 0xFF);
    aic_spi_lcd_write_seq(dev, 0xFB, 0xF3);
    aic_spi_lcd_write_seq(dev, 0xFC, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xFF, 0x00);
    aic_spi_lcd_write_seq(dev, 0x60, 0x42);
    aic_spi_lcd_write_seq(dev, 0x61, 0xDF);
    aic_spi_lcd_write_seq(dev, 0x62, 0x40);
    aic_spi_lcd_write_seq(dev, 0x63, 0x40);
    aic_spi_lcd_write_seq(dev, 0x64, 0x02);
    aic_spi_lcd_write_seq(dev, 0x65, 0x00);
    aic_spi_lcd_write_seq(dev, 0x66, 0x00);
    aic_spi_lcd_write_seq(dev, 0x67, 0x00);
    aic_spi_lcd_write_seq(dev, 0x68, 0x00);
    aic_spi_lcd_write_seq(dev, 0x69, 0x00);
    aic_spi_lcd_write_seq(dev, 0x6A, 0x00);
    aic_spi_lcd_write_seq(dev, 0x6B, 0x00);
    aic_spi_lcd_write_seq(dev, 0x70, 0x42);
    aic_spi_lcd_write_seq(dev, 0x71, 0xDF);
    aic_spi_lcd_write_seq(dev, 0x72, 0x40);
    aic_spi_lcd_write_seq(dev, 0x73, 0x40);
    aic_spi_lcd_write_seq(dev, 0x74, 0x01);
    aic_spi_lcd_write_seq(dev, 0x75, 0x00);
    aic_spi_lcd_write_seq(dev, 0x76, 0x00);
    aic_spi_lcd_write_seq(dev, 0x77, 0x00);
    aic_spi_lcd_write_seq(dev, 0x78, 0x00);
    aic_spi_lcd_write_seq(dev, 0x79, 0x00);
    aic_spi_lcd_write_seq(dev, 0x7A, 0x00);
    aic_spi_lcd_write_seq(dev, 0x7B, 0x00);
    aic_spi_lcd_write_seq(dev, 0x80, 0x48);
    aic_spi_lcd_write_seq(dev, 0x81, 0x00);
    aic_spi_lcd_write_seq(dev, 0x82, 0x04);
    aic_spi_lcd_write_seq(dev, 0x83, 0x02);
    aic_spi_lcd_write_seq(dev, 0x84, 0xDC);
    aic_spi_lcd_write_seq(dev, 0x85, 0x00);
    aic_spi_lcd_write_seq(dev, 0x86, 0x00);
    aic_spi_lcd_write_seq(dev, 0x87, 0x00);
    aic_spi_lcd_write_seq(dev, 0x88, 0x48);
    aic_spi_lcd_write_seq(dev, 0x89, 0x00);
    aic_spi_lcd_write_seq(dev, 0x8A, 0x06);
    aic_spi_lcd_write_seq(dev, 0x8B, 0x02);
    aic_spi_lcd_write_seq(dev, 0x8C, 0xDE);
    aic_spi_lcd_write_seq(dev, 0x8D, 0x00);
    aic_spi_lcd_write_seq(dev, 0x8E, 0x00);
    aic_spi_lcd_write_seq(dev, 0x8F, 0x00);
    aic_spi_lcd_write_seq(dev, 0x90, 0x48);
    aic_spi_lcd_write_seq(dev, 0x91, 0x00);
    aic_spi_lcd_write_seq(dev, 0x92, 0x08);
    aic_spi_lcd_write_seq(dev, 0x93, 0x02);
    aic_spi_lcd_write_seq(dev, 0x94, 0xE0);
    aic_spi_lcd_write_seq(dev, 0x95, 0x00);
    aic_spi_lcd_write_seq(dev, 0x96, 0x00);
    aic_spi_lcd_write_seq(dev, 0x97, 0x00);
    aic_spi_lcd_write_seq(dev, 0x98, 0x48);
    aic_spi_lcd_write_seq(dev, 0x99, 0x00);
    aic_spi_lcd_write_seq(dev, 0x9A, 0x0A);
    aic_spi_lcd_write_seq(dev, 0x9B, 0x02);
    aic_spi_lcd_write_seq(dev, 0x9C, 0xE2);
    aic_spi_lcd_write_seq(dev, 0x9D, 0x00);
    aic_spi_lcd_write_seq(dev, 0x9E, 0x00);
    aic_spi_lcd_write_seq(dev, 0x9F, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA0, 0x48);
    aic_spi_lcd_write_seq(dev, 0xA1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA2, 0x03);
    aic_spi_lcd_write_seq(dev, 0xA3, 0x02);
    aic_spi_lcd_write_seq(dev, 0xA4, 0xDB);
    aic_spi_lcd_write_seq(dev, 0xA5, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA7, 0x00);
    aic_spi_lcd_write_seq(dev, 0xA8, 0x48);
    aic_spi_lcd_write_seq(dev, 0xA9, 0x00);
    aic_spi_lcd_write_seq(dev, 0xAA, 0x05);
    aic_spi_lcd_write_seq(dev, 0xAB, 0x02);
    aic_spi_lcd_write_seq(dev, 0xAC, 0xDD);
    aic_spi_lcd_write_seq(dev, 0xAD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xAE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xAF, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB0, 0x48);
    aic_spi_lcd_write_seq(dev, 0xB1, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB2, 0x07);
    aic_spi_lcd_write_seq(dev, 0xB3, 0x02);
    aic_spi_lcd_write_seq(dev, 0xB4, 0xDF);
    aic_spi_lcd_write_seq(dev, 0xB5, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB7, 0x00);
    aic_spi_lcd_write_seq(dev, 0xB8, 0x48);
    aic_spi_lcd_write_seq(dev, 0xB9, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBA, 0x09);
    aic_spi_lcd_write_seq(dev, 0xBB, 0x02);
    aic_spi_lcd_write_seq(dev, 0xBC, 0xE1);
    aic_spi_lcd_write_seq(dev, 0xBD, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBE, 0x00);
    aic_spi_lcd_write_seq(dev, 0xBF, 0x00);
    aic_spi_lcd_write_seq(dev, 0xC0, 0x65);
    aic_spi_lcd_write_seq(dev, 0xC1, 0x74);
    aic_spi_lcd_write_seq(dev, 0xC2, 0x47);
    aic_spi_lcd_write_seq(dev, 0xC3, 0x56);
    aic_spi_lcd_write_seq(dev, 0xC4, 0xAA);
    aic_spi_lcd_write_seq(dev, 0xC5, 0x11);
    aic_spi_lcd_write_seq(dev, 0xC6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xC7, 0x2A);
    aic_spi_lcd_write_seq(dev, 0xC8, 0xA2);
    aic_spi_lcd_write_seq(dev, 0xC9, 0x33);
    aic_spi_lcd_write_seq(dev, 0xD0, 0x65);
    aic_spi_lcd_write_seq(dev, 0xD1, 0x74);
    aic_spi_lcd_write_seq(dev, 0xD2, 0x47);
    aic_spi_lcd_write_seq(dev, 0xD3, 0x56);
    aic_spi_lcd_write_seq(dev, 0xD4, 0xAA);
    aic_spi_lcd_write_seq(dev, 0xD5, 0x11);
    aic_spi_lcd_write_seq(dev, 0xD6, 0x00);
    aic_spi_lcd_write_seq(dev, 0xD7, 0x2A);
    aic_spi_lcd_write_seq(dev, 0xD8, 0xA2);
    aic_spi_lcd_write_seq(dev, 0xD9, 0x33);
    aic_spi_lcd_write_seq(dev, 0xF3, 0x01);
    aic_spi_lcd_write_seq(dev, 0xF0, 0x00);
    aic_spi_lcd_write_seq(dev, 0x21);
    aic_spi_lcd_write_seq(dev, 0x11);
    rt_thread_mdelay(120);
    aic_spi_lcd_write_seq(dev, 0x29);
    aic_spi_lcd_write_seq(dev, 0x3A, 0x55);
    aicos_mdelay(20);
}

static const struct qspi_disp_priv st77916 = {
    .instruction = {
        .cmd_content = 0x02002C00,
        .flush_content = 0x32002C00,
        .qspi_lines = 1,
    },
    .data_lines = 4,
};

static const struct qspi_cfg qspi = {
    .disp_mode = false,
    .priv = &st77916,
};

#define ST77916_RST_PIN "PD.10"
#define ST77916_BL_PIN  "PE.0"

static void panel_st77916_reset(void)
{
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(ST77916_RST_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_bias_pull(g, p, 0);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_direction_output(g, p);

    hal_gpio_set_output(g, p);
    aicos_mdelay(10);
    hal_gpio_clr_output(g, p);
    aicos_mdelay(20);
    hal_gpio_set_output(g, p);
    aicos_mdelay(120);
}

static int panel_st77916_bringup(void)
{
    static aic_spi_lcd_dev_t *spi_dev;
    aic_spi_lcd_cfg_t config = {
        .bus_name = "qspi1",
        .dev_name = "qspi1dev",
        .max_hz = 25000000,
        .mode = RT_SPI_MODE_0 | RT_SPI_MSB,
        .qspi_cfg = &qspi,
    };

    if (!spi_dev) {
        panel_st77916_reset();

        spi_dev = aic_spi_lcd_init(&config);
        if (!spi_dev) {
            printf("ST77916: qspi1 init failed\n");
            return -1;
        }

        panel_st77916_enable(spi_dev);
    }

    printf("ST77916: panel init done\n");
    return 0;
}

INIT_APP_EXPORT(panel_st77916_bringup);

#ifdef RT_USING_FINSH
static int cmd_spi_st77916(int argc, char **argv)
{
    return panel_st77916_bringup();
}

MSH_CMD_EXPORT_ALIAS(cmd_spi_st77916, spi_st77916, Light ST77916 QSPI panel);
#endif
