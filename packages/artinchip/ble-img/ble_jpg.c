/*
 * 推图 JPEG 走软件解码，不占用 VE。
 * MIPI 视频一直占着硬件解码器，这里不能去抢。
 */
#ifdef LPKG_USING_LVGL
#define LV_USE_TJPGD 1
#include "tjpgd.c"

struct jpg_mem {
    const uint8_t *buf;
    uint32_t len;
    uint32_t pos;
    uint16_t *rgb;
    uint16_t w;
    uint16_t h;
    uint16_t rows;
};

static size_t jpg_infunc(JDEC *jd, uint8_t *buff, size_t ndata)
{
    struct jpg_mem *m = jd->device;
    uint32_t left;

    if (!m || m->pos > m->len) {
        return 0;
    }
    left = m->len - m->pos;
    if (ndata > left) {
        ndata = left;
    }
    if (buff && ndata) {
        memcpy(buff, m->buf + m->pos, ndata);
    }
    m->pos += (uint32_t)ndata;
    return ndata;
}

static int jpg_outfunc(JDEC *jd, void *bitmap, JRECT *rect)
{
    struct jpg_mem *m = jd->device;
    const uint8_t *src = bitmap;
    int x;
    int y;
    int bw;

    if (!m || !rect || rect->right < rect->left || rect->bottom < rect->top) {
        return 0;
    }
    if (rect->right >= m->w || rect->bottom >= m->h) {
        return 0;
    }

    bw = rect->right - rect->left + 1;
    for (y = rect->top; y <= rect->bottom; y++) {
        uint16_t *dst = m->rgb + (uint32_t)y * m->w + rect->left;

        for (x = 0; x < bw; x++) {
            uint8_t b = src[0];
            uint8_t g = src[1];
            uint8_t r = src[2];

            dst[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            src += 3;
        }
    }
    /* 一行 MCU 要画到右边界才算能上屏，半行先不公布。 */
    if ((uint16_t)(rect->right + 1) >= m->w &&
        (uint16_t)(rect->bottom + 1) > m->rows) {
        m->rows = (uint16_t)(rect->bottom + 1);
    }
    return 1;
}

/* 返回已经解出的整行数。数据还没收完时允许中途停，调用方接着刷已有的行。 */
int ble_jpg_decode_rows(const uint8_t *jpg, uint32_t len, uint16_t *rgb,
                        uint16_t w, uint16_t h)
{
    static uint8_t pool[16 * 1024];
    struct jpg_mem m;
    JDEC jd;
    JRESULT rc;

    if (!jpg || !rgb || !len || !w || !h) {
        return -1;
    }

    m.buf = jpg;
    m.len = len;
    m.pos = 0;
    m.rgb = rgb;
    m.w = w;
    m.h = h;
    m.rows = 0;
    rc = jd_prepare(&jd, jpg_infunc, pool, sizeof(pool), &m);
    if (rc != JDR_OK || jd.width != w || jd.height != h) {
        return -1;
    }
    rc = jd_decomp(&jd, jpg_outfunc, 0);
    if (rc != JDR_OK && rc != JDR_INP) {
        return -1;
    }
    return (int)m.rows;
}
#else
int ble_jpg_decode_rows(const uint8_t *jpg, uint32_t len, uint16_t *rgb,
                        uint16_t w, uint16_t h)
{
    (void)jpg;
    (void)len;
    (void)rgb;
    (void)w;
    (void)h;
    return -1;
}
#endif
