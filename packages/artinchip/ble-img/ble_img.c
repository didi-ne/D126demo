#define LOG_TAG "ble_img"
#define LOG_LVL LOG_LVL_INFO
#include <ulog.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_osal.h>
#include <aic_crc32.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/sys/byteorder.h>

#include "ble_img.h"

void __attribute__((weak)) screen_1_slideshow_pause(void) {}
void __attribute__((weak)) screen_1_slideshow_start(void) {}
void __attribute__((weak)) qspi_carousel_pause(void) {}
void __attribute__((weak)) qspi_carousel_resume(void) {}

#ifdef LPKG_USING_LVGL
#include <lvgl.h>
extern int ble_jpg_decode_rows(const uint8_t *jpg, uint32_t len, uint16_t *rgb,
                               uint16_t w, uint16_t h);
#endif

#define BLE_IMG_UUID_SVC  0xFF00
#define BLE_IMG_UUID_CTRL 0xFF01
#define BLE_IMG_UUID_DATA 0xFF02
#define BLE_IMG_UUID_STAT 0xFF03

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BLE_IMG_MEM MEM_CMA

enum ble_img_phase {
    BLE_IMG_IDLE = 0,
    BLE_IMG_WAIT_DATA,
    BLE_IMG_WAIT_DEC,
    BLE_IMG_SHOW,
};

struct ble_img_sess {
    enum ble_img_phase phase;
    uint8_t abort;
    uint8_t *buf;
    uint32_t total_len;
    uint32_t crc_file;
    uint16_t chunk_cnt;
    uint16_t expect_seq;
    uint16_t win_got;
    uint32_t filled;
};

static struct bt_conn *s_conn;
static uint8_t s_status_ccc;
static aicos_mutex_t s_lock;
static aicos_sem_t s_dec_sem;
static aicos_sem_t s_st_sem;
static rt_tick_t s_last_data_tick;
static volatile uint8_t s_paint_kick;
static volatile uint8_t s_sess_gen;

/* STATUS 不在 GATT 写回调里发，也不占用蓝牙的系统工作队列。
   那条队列还要回收发送缓冲；在里面睡觉或等缓冲，链路会卡大约 30 秒，
   手机会关掉通知再断开。 */
#define BLE_IMG_ST_Q 8
#define BLE_IMG_ST_LEN 6
/* 手机写 START 的回调经常晚于 200ms。通知若在这之前发出去，会被丢掉，
   手机空等 2 秒后整张重来。OK 在这段等待里排三个时刻；收到数据就不再发。 */
#define BLE_IMG_ST_DELAY_MS 200
static aicos_mutex_t s_st_lock;
static uint8_t s_st_q[BLE_IMG_ST_Q][BLE_IMG_ST_LEN];
static rt_tick_t s_st_due[BLE_IMG_ST_Q];
static uint8_t s_st_r;
static uint8_t s_st_n;
static uint8_t s_st_seq;

static int ble_img_status_enqueue(uint8_t code, uint32_t value, uint32_t delay_ms);
static void ble_img_status_q_clear(void);
static void ble_img_status_arm(uint8_t code, uint32_t value);

#ifdef LPKG_USING_LVGL
static uint16_t *s_rgb;
static lv_image_dsc_t s_rgb_dsc;
static uint16_t s_shown_rows;
static volatile uint16_t s_ready_rows;
static volatile uint8_t s_rgb_ready;
static volatile uint8_t s_img_final;
static volatile uint8_t s_need_attach;
static volatile uint8_t s_slide_resume;
static volatile uint8_t s_slide_pause;
static volatile uint8_t s_done_sent;
#endif
static struct ble_img_sess s_sess;
static uint16_t s_partial_rx;

#ifdef LPKG_USING_LVGL
static lv_timer_t *s_lv_timer;
static lv_obj_t *s_img;
#endif

static const uint8_t s_magic[4] = { 'I', 'M', 'G', 'P' };

/* 名称放进 AD（主广播），被动扫描也能看到 XMZX-D126；SD 再带一份兼容主动扫描 */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BLE_IMG_UUID_SVC)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static uint16_t chunk_cnt_of(uint32_t total_len)
{
    return (uint16_t)((total_len + BLE_IMG_CHUNK_PAYLOAD - 1) / BLE_IMG_CHUNK_PAYLOAD);
}

static void sess_clear_locked(void)
{
    if (s_sess.buf) {
        aicos_free(BLE_IMG_MEM, s_sess.buf);
        s_sess.buf = NULL;
    }
    memset(&s_sess, 0, sizeof(s_sess));
    s_sess_gen++;
    s_partial_rx = 0;
}

static int adv_start(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_E("adv start %d", err);
    } else {
        LOG_I("advertising %s", DEVICE_NAME);
    }
    return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_E("connect failed 0x%02x", err);
        adv_start();
        return;
    }
    s_conn = bt_conn_ref(conn);
    LOG_I("connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_I("disconnected 0x%02x", reason);
    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.phase == BLE_IMG_WAIT_DEC) {
        s_sess.abort = 1;
#ifdef LPKG_USING_LVGL
        s_rgb_ready = 0;
#endif
    } else {
        sess_clear_locked();
    }
    if (s_conn) {
        bt_conn_unref(s_conn);
        s_conn = NULL;
    }
    s_status_ccc = 0;
    aicos_mutex_give(s_lock);
    (void)conn;
    adv_start();
}

BT_CONN_CB_DEFINE(ble_img_conn_cb) = {
    .connected = connected,
    .disconnected = disconnected,
};

extern const struct bt_gatt_service_static ble_img_svc;

/* 不是图片开始帧的写入，按原始字节收下。 */
static void ble_raw_rx(const uint8_t *p, uint16_t len)
{
    uint16_t i;

    rt_kprintf("BLE RX %u:", len);
    for (i = 0; i < len; i++) {
        rt_kprintf(" %02X", p[i]);
    }
    rt_kprintf("\n");

    if (s_conn && s_status_ccc && len) {
        int err = bt_gatt_notify(s_conn, &ble_img_svc.attrs[6], p, len);

        if (err) {
            LOG_W("raw echo %d", err);
        }
    }
}

static int ble_is_img_start(const uint8_t *p, uint16_t len)
{
    return len == BLE_IMG_START_LEN && p[0] == BLE_IMG_FLAG_START &&
           memcmp(&p[3], s_magic, 4) == 0;
}

static ssize_t ctrl_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *buf, uint16_t len, uint16_t offset,
                          uint8_t flags)
{
    const uint8_t *p = buf;
    uint8_t *jpeg;
    uint32_t total_len;
    uint32_t crc_file;
    uint32_t crc_pkt;
    uint16_t seq;
    uint16_t width;
    uint16_t height;
    uint16_t chunk_cnt;
    uint16_t mtu;
    uint8_t flag;
    uint8_t fmt;

    (void)attr;
    (void)flags;
    if (offset) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.phase == BLE_IMG_WAIT_DATA) {
        rt_tick_t now = rt_tick_get();
        int live = s_last_data_tick &&
                   (now - s_last_data_tick) < rt_tick_from_millisecond(400);

        /* 这轮还在收。重复的 START 直接丢掉，别清 seq，也别再发一条 STATUS。 */
        if (live) {
            aicos_mutex_give(s_lock);
            return len;
        }
        LOG_W("restart idle session seq=%u filled=%u",
              (unsigned)s_sess.expect_seq, (unsigned)s_sess.filled);
        sess_clear_locked();
    } else if (s_sess.phase != BLE_IMG_IDLE) {
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_BUSY, 0);
        return len;
    }
    aicos_mutex_give(s_lock);

    if (!ble_is_img_start(p, len)) {
        ble_raw_rx(p, len);
        return len;
    }
    flag = p[0];

    mtu = bt_gatt_get_mtu(conn);
    if (mtu < BLE_IMG_MTU_MIN) {
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, mtu);
        return len;
    }

    crc_pkt = sys_get_le32(&p[22]);
    if (crc32(0, p, 22) != crc_pkt) {
        ble_img_status_notify(BLE_IMG_ST_CRC_PKT, 0);
        return len;
    }

    seq = sys_get_le16(&p[1]);
    if (seq != 0 || memcmp(&p[3], s_magic, 4) != 0) {
        ble_img_status_notify(BLE_IMG_ST_FLAG_ERR, flag);
        return len;
    }

    total_len = sys_get_le32(&p[7]);
    width = sys_get_le16(&p[11]);
    height = sys_get_le16(&p[13]);
    fmt = p[15];
    chunk_cnt = sys_get_le16(&p[16]);
    crc_file = sys_get_le32(&p[18]);

    if (width != BLE_IMG_W || height != BLE_IMG_H || fmt != BLE_IMG_FMT_JPEG) {
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, width);
        return len;
    }
    if (total_len == 0 || total_len > BLE_IMG_MAX) {
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, total_len);
        return len;
    }
    if (chunk_cnt != chunk_cnt_of(total_len)) {
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, chunk_cnt);
        return len;
    }

    jpeg = aicos_malloc(BLE_IMG_MEM, total_len);
    if (!jpeg) {
        LOG_E("malloc %u fail", (unsigned)total_len);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, total_len);
        return len;
    }

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.phase != BLE_IMG_IDLE) {
        aicos_mutex_give(s_lock);
        aicos_free(BLE_IMG_MEM, jpeg);
        ble_img_status_notify(BLE_IMG_ST_BUSY, 0);
        return len;
    }
    s_sess.buf = jpeg;
    s_sess.total_len = total_len;
    s_sess.crc_file = crc_file;
    s_sess.chunk_cnt = chunk_cnt;
    s_sess.expect_seq = 1;
    s_sess.win_got = 0;
    s_sess.filled = 0;
    s_sess.abort = 0;
    s_sess.phase = BLE_IMG_WAIT_DATA;
    s_last_data_tick = rt_tick_get();
    aicos_mutex_give(s_lock);
#ifdef LPKG_USING_LVGL
    s_rgb_ready = 0;
    s_ready_rows = 0;
    s_img_final = 0;
    s_done_sent = 0;
    s_need_attach = 1;
    /* 第一窗的数据还没来。先停 QSPI 轮播，免得整屏刷新把这一窗的字节冲掉。 */
    s_slide_pause = 1;
#endif

    LOG_I("START len=%u chunks=%u", (unsigned)total_len, chunk_cnt);
    ble_img_status_q_clear();
    ble_img_status_arm(BLE_IMG_ST_OK, BLE_IMG_WIN);
    return len;
}

static ssize_t data_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                          const void *buf, uint16_t len, uint16_t offset,
                          uint8_t flags)
{
    const uint8_t *p = buf;
    uint32_t crc_pkt;
    uint32_t off;
    uint32_t remain;
    uint16_t seq;
    uint16_t plen;
    uint8_t flag;
    uint8_t end;

    (void)conn;
    (void)attr;
    (void)flags;
    if (offset) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.phase != BLE_IMG_WAIT_DATA || !s_sess.buf) {
        aicos_mutex_give(s_lock);
        ble_raw_rx(buf, len);
        return len;
    }

    if (len < 9) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, len);
        return len;
    }

    flag = p[0];
    seq = sys_get_le16(&p[1]);
    plen = sys_get_le16(&p[3]);
    if (flag != BLE_IMG_FLAG_MIDDLE && flag != BLE_IMG_FLAG_END) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_FLAG_ERR, flag);
        return len;
    }
    if (plen == 0 || plen > BLE_IMG_CHUNK_PAYLOAD || len != (uint16_t)(9 + plen)) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, plen);
        return len;
    }

    crc_pkt = sys_get_le32(&p[5 + plen]);
    if (crc32(0, p, 5 + plen) != crc_pkt) {
        uint16_t bad_seq = seq;
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_CRC_PKT, bad_seq);
        return len;
    }

    if (seq != s_sess.expect_seq) {
        uint16_t want = s_sess.expect_seq;
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SEQ_ERR, want);
        return len;
    }

    off = (uint32_t)(seq - 1) * BLE_IMG_CHUNK_PAYLOAD;
    if (off >= s_sess.total_len) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, off);
        return len;
    }
    remain = s_sess.total_len - off;
    end = (seq == s_sess.chunk_cnt);
    if (end) {
        if (flag != BLE_IMG_FLAG_END) {
            sess_clear_locked();
            aicos_mutex_give(s_lock);
            ble_img_status_notify(BLE_IMG_ST_FLAG_ERR, flag);
            return len;
        }
        if (plen != remain) {
            sess_clear_locked();
            aicos_mutex_give(s_lock);
            ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, plen);
            return len;
        }
    } else if (flag != BLE_IMG_FLAG_MIDDLE) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_FLAG_ERR, flag);
        return len;
    } else if (plen != BLE_IMG_CHUNK_PAYLOAD) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, plen);
        return len;
    }

    memcpy(s_sess.buf + off, &p[5], plen);
    s_sess.expect_seq++;
    s_sess.win_got++;
    s_sess.filled = off + plen;
    s_last_data_tick = rt_tick_get();
    if (end) {
        s_sess.phase = BLE_IMG_WAIT_DEC;
        aicos_mutex_give(s_lock);
        aicos_sem_give(s_dec_sem);
        return len;
    }
    if (s_sess.win_got >= BLE_IMG_WIN) {
        s_sess.win_got = 0;
        seq = s_sess.expect_seq;
        s_paint_kick = 1;
        aicos_mutex_give(s_lock);
        /* 这一窗的字节已经够解出新的行，别等整张图。 */
        aicos_sem_give(s_dec_sem);
        ble_img_status_q_clear();
        ble_img_status_arm(BLE_IMG_ST_RX, seq);
        return len;
    }
    aicos_mutex_give(s_lock);
    return len;
}

static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    (void)attr;
    s_status_ccc = (value == BT_GATT_CCC_NOTIFY);
    LOG_I("STATUS CCC %s", s_status_ccc ? "on" : "off");
}

BT_GATT_SERVICE_DEFINE(ble_img_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(BLE_IMG_UUID_SVC)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BLE_IMG_UUID_CTRL),
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, ctrl_write, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BLE_IMG_UUID_DATA),
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           NULL, data_write, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BLE_IMG_UUID_STAT),
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(status_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* 手机已经开始收下一窗时，迟到的 OK 会被当成失败。发出去之前再看一眼会话。 */
static int ble_img_status_still_wanted(const uint8_t pkt[BLE_IMG_ST_LEN])
{
    uint32_t value = sys_get_le32(&pkt[1]);
    int wanted = 1;

    if (!s_lock) {
        return 1;
    }
    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (pkt[0] == BLE_IMG_ST_OK) {
        wanted = (s_sess.phase == BLE_IMG_WAIT_DATA &&
                  s_sess.filled == 0 && s_sess.expect_seq == 1);
    } else if (pkt[0] == BLE_IMG_ST_RX) {
        /* 这一窗没收齐时，value 就是还缺的序号，手机从这里续传。 */
        wanted = (s_sess.phase == BLE_IMG_WAIT_DATA &&
                  s_sess.expect_seq == value);
    }
    aicos_mutex_give(s_lock);
    return wanted;
}

static void ble_img_status_q_remove(int idx)
{
    int off = (idx - s_st_r + BLE_IMG_ST_Q) % BLE_IMG_ST_Q;
    int i;

    for (i = off; i < s_st_n - 1; i++) {
        int from = (s_st_r + i + 1) % BLE_IMG_ST_Q;
        int to = (s_st_r + i) % BLE_IMG_ST_Q;

        memcpy(s_st_q[to], s_st_q[from], BLE_IMG_ST_LEN);
        s_st_due[to] = s_st_due[from];
    }
    s_st_n--;
}

static void ble_img_status_flush(void)
{
    for (;;) {
        uint8_t pkt[BLE_IMG_ST_LEN];
        rt_tick_t due;
        rt_tick_t now;
        int idx;
        int i;
        int err;
        uint32_t ms;

        if (!s_st_lock) {
            return;
        }
        aicos_mutex_take(s_st_lock, AICOS_WAIT_FOREVER);
        if (!s_st_n) {
            aicos_mutex_give(s_st_lock);
            return;
        }
        idx = s_st_r;
        due = s_st_due[idx];
        for (i = 1; i < s_st_n; i++) {
            int j = (s_st_r + i) % BLE_IMG_ST_Q;

            if ((rt_int32_t)(s_st_due[j] - due) < 0) {
                idx = j;
                due = s_st_due[j];
            }
        }
        now = rt_tick_get();
        if ((rt_int32_t)(due - now) > 0) {
            ms = (uint32_t)(due - now) * 1000 / RT_TICK_PER_SECOND;
            if (ms > 20) {
                ms = 20;
            }
            if (!ms) {
                ms = 1;
            }
            aicos_mutex_give(s_st_lock);
            /* 短睡再看队列。新来的 RX 不能被还没到期的 OK 挡住。 */
            aicos_msleep(ms);
            continue;
        }
        memcpy(pkt, s_st_q[idx], sizeof(pkt));
        ble_img_status_q_remove(idx);
        aicos_mutex_give(s_st_lock);

        if (!ble_img_status_still_wanted(pkt)) {
            LOG_I("STATUS skip 0x%02x val=%u", pkt[0],
                  (unsigned)sys_get_le32(&pkt[1]));
            continue;
        }
        err = bt_gatt_notify(s_conn, &ble_img_svc.attrs[6], pkt, sizeof(pkt));
        if (err) {
            LOG_W("STATUS tx 0x%02x fail %d", pkt[0], err);
            /* 缓冲暂时没有就过一会儿再试，别把这一条从队列里丢掉。 */
            if (err != -ENOTCONN && ble_img_status_still_wanted(pkt)) {
                ble_img_status_enqueue(pkt[0], sys_get_le32(&pkt[1]), 40);
            }
        } else {
            LOG_I("STATUS tx 0x%02x", pkt[0]);
        }
    }
}

static int ble_img_status_enqueue(uint8_t code, uint32_t value, uint32_t delay_ms)
{
    uint8_t pkt[BLE_IMG_ST_LEN];
    uint8_t slot;

    if (!s_status_ccc) {
        LOG_W("STATUS CCC off, drop 0x%02x", code);
        return -1;
    }
    pkt[0] = code;
    sys_put_le32(value, &pkt[1]);
    LOG_I("STATUS 0x%02x val=%u +%ums", code, (unsigned)value, (unsigned)delay_ms);

    if (!s_st_lock || !s_st_sem) {
        pkt[5] = s_st_seq++;
        return bt_gatt_notify(s_conn, &ble_img_svc.attrs[6], pkt, sizeof(pkt));
    }

    aicos_mutex_take(s_st_lock, AICOS_WAIT_FOREVER);
    pkt[5] = s_st_seq++;
    if (s_st_n >= BLE_IMG_ST_Q) {
        s_st_r = (uint8_t)((s_st_r + 1) % BLE_IMG_ST_Q);
        s_st_n--;
    }
    slot = (uint8_t)((s_st_r + s_st_n) % BLE_IMG_ST_Q);
    memcpy(s_st_q[slot], pkt, sizeof(pkt));
    s_st_due[slot] = rt_tick_get() + rt_tick_from_millisecond(delay_ms);
    s_st_n++;
    aicos_mutex_give(s_st_lock);
    aicos_sem_give(s_st_sem);
    return 0;
}

static void ble_img_status_q_clear(void)
{
    if (!s_st_lock) {
        return;
    }
    aicos_mutex_take(s_st_lock, AICOS_WAIT_FOREVER);
    s_st_n = 0;
    s_st_r = 0;
    aicos_mutex_give(s_st_lock);
}

/* 同一个状态排几个发送时刻。手机 2 秒超时内总能撞上一次；数据一动，没发出去的会被丢掉。 */
static void ble_img_status_arm(uint8_t code, uint32_t value)
{
    static const uint16_t ok_ms[] = { 500, 1200, 1900 };
    /* 第一包晚 50ms，躲开手机还没返回的写。没被接住再补一次，仍落在 2 秒超时里。 */
    static const uint16_t rx_ms[] = { 50, 900, 1600 };
    const uint16_t *ms = (code == BLE_IMG_ST_OK) ? ok_ms : rx_ms;
    int i;

    for (i = 0; i < 3; i++) {
        ble_img_status_enqueue(code, value, ms[i]);
    }
}

int ble_img_status_notify(uint8_t code, uint32_t value)
{
    return ble_img_status_enqueue(code, value, BLE_IMG_ST_DELAY_MS);
}

#ifdef LPKG_USING_LVGL
static lv_disp_t *find_qspi_disp(void)
{
    lv_disp_t *d;

    for (d = lv_disp_get_next(NULL); d; d = lv_disp_get_next(d)) {
        if (lv_disp_get_hor_res(d) == BLE_IMG_W &&
            lv_disp_get_ver_res(d) == BLE_IMG_H) {
            return d;
        }
    }
    return NULL;
}

static void qspi_attach_image(lv_disp_t *disp)
{
    lv_obj_t *top;

    memset(&s_rgb_dsc, 0, sizeof(s_rgb_dsc));
    s_rgb_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_rgb_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_rgb_dsc.header.w = BLE_IMG_W;
    s_rgb_dsc.header.h = BLE_IMG_H;
    s_rgb_dsc.header.stride = BLE_IMG_W * 2;
    s_rgb_dsc.data_size = (uint32_t)BLE_IMG_W * BLE_IMG_H * 2;
    s_rgb_dsc.data = (const uint8_t *)s_rgb;

    top = lv_display_get_layer_top(disp);
    if (!s_img || !lv_obj_is_valid(s_img)) {
        s_img = lv_image_create(top);
        lv_obj_remove_flag(s_img, LV_OBJ_FLAG_CLICKABLE);
        lv_image_set_inner_align(s_img, LV_IMAGE_ALIGN_TOP_LEFT);
        lv_obj_set_pos(s_img, 0, 0);
    }
    lv_image_cache_drop(&s_rgb_dsc);
    lv_image_set_src(s_img, &s_rgb_dsc);
    lv_obj_move_foreground(s_img);
    s_shown_rows = 0;
    lv_obj_set_size(s_img, BLE_IMG_W, 0);
}

static void ble_img_lv_timer(lv_timer_t *timer)
{
    (void)timer;

    /* 这一窗没收齐就先别整张重来。安静 400ms 后把下一个序号告诉手机，让它只补后面的。 */
    if (s_lock) {
        uint16_t expect = 0;
        int stalled = 0;

        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        if (s_sess.phase == BLE_IMG_WAIT_DATA && s_sess.win_got > 0 &&
            s_last_data_tick &&
            (rt_tick_get() - s_last_data_tick) >= rt_tick_from_millisecond(400) &&
            s_partial_rx != s_sess.expect_seq) {
            s_partial_rx = s_sess.expect_seq;
            expect = s_sess.expect_seq;
            stalled = 1;
        }
        aicos_mutex_give(s_lock);
        if (stalled) {
            LOG_I("RX resume seq=%u", (unsigned)expect);
            ble_img_status_q_clear();
            ble_img_status_arm(BLE_IMG_ST_RX, expect);
        }
    }

    if (s_slide_pause) {
        s_slide_pause = 0;
        s_slide_resume = 0;
        screen_1_slideshow_pause();
        qspi_carousel_pause();
    } else if (s_slide_resume) {
        s_slide_resume = 0;
        screen_1_slideshow_start();
        qspi_carousel_resume();
    }
    if (!s_rgb_ready || !s_rgb || !s_ready_rows) {
        return;
    }
    /* 一窗里的包大约 20ms 一个。整屏拷贝和 QSPI 刷新若卡在这些包上，
       串口会丢字节，这一窗就收不齐。安静 60ms 说明这一窗已经收完，
       下一窗要等 50ms 后的 RX 通知才会来，这段空档里刷完。 */
    if (s_last_data_tick &&
        (rt_tick_get() - s_last_data_tick) < rt_tick_from_millisecond(60)) {
        return;
    }
    if (s_need_attach || !s_img || !lv_obj_is_valid(s_img)) {
        lv_disp_t *disp = find_qspi_disp();

        if (!disp) {
            LOG_E("no 360x360 disp");
            s_rgb_ready = 0;
            s_slide_resume = 1;
            aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
            sess_clear_locked();
            aicos_mutex_give(s_lock);
            ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, 0);
            return;
        }
        /* 第一行能画了才停 QSPI 轮播。MIPI 不动。 */
        screen_1_slideshow_pause();
        qspi_carousel_pause();
        qspi_attach_image(disp);
        s_need_attach = 0;
        s_shown_rows = 0;
    }
    if (s_shown_rows < s_ready_rows) {
        s_shown_rows = s_ready_rows;
        if (s_shown_rows > BLE_IMG_H) {
            s_shown_rows = BLE_IMG_H;
        }
        lv_obj_set_size(s_img, BLE_IMG_W, s_shown_rows);
        lv_obj_invalidate(s_img);
    }
    if (s_img_final && s_shown_rows >= BLE_IMG_H && !s_done_sent) {
        uint32_t total;

        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        if (s_sess.phase != BLE_IMG_SHOW) {
            aicos_mutex_give(s_lock);
            return;
        }
        s_done_sent = 1;
        s_rgb_ready = 0;
        total = s_sess.total_len;
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        LOG_I("QSPI show %u B", (unsigned)total);
        ble_img_status_notify(BLE_IMG_ST_DONE, total);
    }
}
#endif

static void ble_img_finish_jpeg(void)
{
    uint8_t *buf;
    uint32_t total_len;
    uint32_t want;
    uint32_t got;
    uint16_t *rgb;

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.phase != BLE_IMG_WAIT_DEC) {
        aicos_mutex_give(s_lock);
        return;
    }
    if (s_sess.abort || !s_sess.buf) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        return;
    }
    buf = s_sess.buf;
    total_len = s_sess.total_len;
    want = s_sess.crc_file;
    aicos_mutex_give(s_lock);

    got = crc32(0, buf, total_len);

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.abort || s_sess.phase != BLE_IMG_WAIT_DEC) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        return;
    }
    if (got != want) {
        LOG_E("crc_file got=0x%08x want=0x%08x", (unsigned)got, (unsigned)want);
        sess_clear_locked();
        aicos_mutex_give(s_lock);
#ifdef LPKG_USING_LVGL
        s_slide_resume = 1;
#endif
        ble_img_status_notify(BLE_IMG_ST_CRC_FILE, 0);
        return;
    }

    LOG_I("jpeg ready %u B", (unsigned)total_len);
#ifdef LPKG_USING_LVGL
    if (!s_lv_timer) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, 0);
        return;
    }
    aicos_mutex_give(s_lock);

    rgb = s_rgb;
    if (!rgb) {
        rgb = aicos_malloc(BLE_IMG_MEM, (uint32_t)BLE_IMG_W * BLE_IMG_H * 2);
        if (!rgb) {
            LOG_E("rgb malloc fail");
            aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
            sess_clear_locked();
            aicos_mutex_give(s_lock);
            s_slide_resume = 1;
            ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, total_len);
            return;
        }
        s_rgb = rgb;
        memset(rgb, 0, (uint32_t)BLE_IMG_W * BLE_IMG_H * 2);
    }
    if (ble_jpg_decode_rows(buf, total_len, rgb, BLE_IMG_W, BLE_IMG_H) != BLE_IMG_H) {
        LOG_E("soft jpeg decode fail");
        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        s_slide_resume = 1;
        ble_img_status_notify(BLE_IMG_ST_SIZE_ERR, 0);
        return;
    }

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (s_sess.abort || s_sess.phase != BLE_IMG_WAIT_DEC) {
        sess_clear_locked();
        aicos_mutex_give(s_lock);
        return;
    }
    if (s_sess.buf) {
        aicos_free(BLE_IMG_MEM, s_sess.buf);
        s_sess.buf = NULL;
    }
    s_sess.phase = BLE_IMG_SHOW;
    aicos_mutex_give(s_lock);

    s_done_sent = 0;
    s_ready_rows = BLE_IMG_H;
    s_img_final = 1;
    s_rgb_ready = 1;
#else
    sess_clear_locked();
    aicos_mutex_give(s_lock);
    ble_img_status_notify(BLE_IMG_ST_DONE, total_len);
    (void)rgb;
#endif
}

/* 用已经收到的字节解出完整行。解不出来就等下一窗，不报错。 */
static void ble_img_paint_rows(void)
{
    uint8_t *snap = NULL;
    uint32_t filled = 0;
    uint8_t gen = 0;
    int rows = -1;

    s_paint_kick = 0;
#ifdef LPKG_USING_LVGL
    {
        uint8_t *src = NULL;

        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        gen = s_sess_gen;
        if (s_sess.phase == BLE_IMG_WAIT_DATA && s_sess.buf && s_sess.filled) {
            filled = s_sess.filled;
            src = s_sess.buf;
        }
        aicos_mutex_give(s_lock);
        if (!src || !filled) {
            return;
        }
        snap = aicos_malloc(BLE_IMG_MEM, filled);
        if (!snap) {
            return;
        }
        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        if (gen != s_sess_gen || s_sess.buf != src || s_sess.filled < filled) {
            aicos_mutex_give(s_lock);
            aicos_free(BLE_IMG_MEM, snap);
            return;
        }
        memcpy(snap, src, filled);
        aicos_mutex_give(s_lock);
    }
    if (!snap) {
        return;
    }

    if (!s_rgb) {
        s_rgb = aicos_malloc(BLE_IMG_MEM, (uint32_t)BLE_IMG_W * BLE_IMG_H * 2);
        if (!s_rgb) {
            aicos_free(BLE_IMG_MEM, snap);
            return;
        }
        memset(s_rgb, 0, (uint32_t)BLE_IMG_W * BLE_IMG_H * 2);
    }
    rows = ble_jpg_decode_rows(snap, filled, s_rgb, BLE_IMG_W, BLE_IMG_H);
    aicos_free(BLE_IMG_MEM, snap);

    aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
    if (gen != s_sess_gen || s_sess.phase != BLE_IMG_WAIT_DATA) {
        aicos_mutex_give(s_lock);
        return;
    }
    if (rows > (int)s_ready_rows) {
        s_ready_rows = (uint16_t)rows;
        s_rgb_ready = 1;
        aicos_mutex_give(s_lock);
        LOG_I("QSPI rows %d", rows);
        return;
    }
    aicos_mutex_give(s_lock);
#else
    (void)gen;
    (void)rows;
    (void)snap;
    (void)filled;
#endif
}

static void ble_img_on_wake(void)
{
    for (;;) {
        enum ble_img_phase ph;

        aicos_mutex_take(s_lock, AICOS_WAIT_FOREVER);
        ph = s_sess.phase;
        aicos_mutex_give(s_lock);
        if (ph == BLE_IMG_WAIT_DEC) {
            ble_img_finish_jpeg();
            return;
        }
        if (ph == BLE_IMG_WAIT_DATA && s_paint_kick) {
            ble_img_paint_rows();
            if (s_paint_kick) {
                continue;
            }
        }
        return;
    }
}

#ifdef CONFIG_AIC8800_BT_SUPPORT
#include <wlan_mgnt.h>
#include <drivers/sdio_mgr.h>
extern int aicbt_is_ready(void);
extern void aic8800_fwload_set(int busy);
#endif

static void ble_img_thread(void *arg)
{
    int err;
    int n;

    (void)arg;

#ifdef CONFIG_AIC8800_BT_SUPPORT
    /* 盖住从开机到 fdrv 返回的整段。这段里读 SD 上的大视频会把 SDIO 命令拖超时。 */
    aic8800_fwload_set(1);
#endif

    for (n = 0; n < 50; n++) {
        if (rt_device_find(LPKG_BT_UART) && rt_device_find("hci")) {
            break;
        }
        aicos_msleep(200);
    }
    if (!rt_device_find(LPKG_BT_UART) || !rt_device_find("hci")) {
        LOG_E("uart/hci not ready");
#ifdef CONFIG_AIC8800_BT_SUPPORT
        aic8800_fwload_set(0);
#endif
        return;
    }

#ifdef CONFIG_AIC8800_BT_SUPPORT
    /* SDMC1 扫描是推迟的，开机时 sdio_mgr 等不到设备。
     * 直接 set_mode，在本线程里扫描并灌固件，再开广播。 */
    err = rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    aic8800_fwload_set(0);
    if (err) {
        LOG_E("wlan set_mode %d", err);
        return;
    }
    for (n = 0; n < 150; n++) {
        err = aicbt_is_ready();
        if (err != 0) {
            break;
        }
        aicos_msleep(200);
    }
    if (aicbt_is_ready() != 1) {
        LOG_E("aicbt patch not ready (%d)", aicbt_is_ready());
        return;
    }
    LOG_I("aicbt patch ready");
    aicos_msleep(200);
#else
    aicos_msleep(5000);
#endif

    LOG_I("bt_enable...");
    err = bt_enable(NULL);
    if (err) {
        LOG_E("bt_enable %d", err);
        return;
    }
    LOG_I("bt ready");

    if (adv_start()) {
        return;
    }
    LOG_I("scan for %s", DEVICE_NAME);

#ifdef LPKG_USING_LVGL
    for (n = 0; n < 200; n++) {
        if (lv_is_initialized()) {
            break;
        }
        aicos_msleep(50);
    }
    if (lv_is_initialized()) {
        s_lv_timer = lv_timer_create(ble_img_lv_timer, 20, NULL);
        if (s_lv_timer) {
            LOG_I("lv timer ready");
        } else {
            LOG_E("lv_timer_create fail");
        }
    } else {
        LOG_E("lvgl not ready");
    }
#endif

    for (;;) {
        aicos_sem_take(s_dec_sem, AICOS_WAIT_FOREVER);
        ble_img_on_wake();
    }
}

/* 通知单独一条线程发。解码线程优先级低，挂在它上面的话 OK 会赶不上面机的 2 秒超时。 */
static void ble_img_tx_thread(void *arg)
{
    (void)arg;

    for (;;) {
        aicos_sem_take(s_st_sem, AICOS_WAIT_FOREVER);
        ble_img_status_flush();
    }
}

int ble_img_init(void)
{
    s_lock = aicos_mutex_create();
    s_st_lock = aicos_mutex_create();
    s_dec_sem = aicos_sem_create(0);
    s_st_sem = aicos_sem_create(0);
    if (!s_lock || !s_st_lock || !s_dec_sem || !s_st_sem) {
        LOG_E("ipc create failed");
        return -1;
    }
    /* 低于蓝牙 RX(13)，高于 LVGL。只负责把 STATUS 发出去。 */
    if (!aicos_thread_create("ble_img_tx", 8192, 16, ble_img_tx_thread, NULL)) {
        LOG_E("tx thread create failed");
        return -1;
    }
    /* 高于 LVGL，避免界面线程把 SDIO 扫描和灌固件饿死。平时堵在信号量上。 */
    if (!aicos_thread_create("ble_img", 8192, 10, ble_img_thread, NULL)) {
        LOG_E("thread create failed");
        return -1;
    }
    return 0;
}
INIT_APP_EXPORT(ble_img_init);
