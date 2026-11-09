#ifndef BLE_IMG_H
#define BLE_IMG_H

#include <stdint.h>

#define BLE_IMG_CHUNK_PAYLOAD 232
#define BLE_IMG_WIN           4
#define BLE_IMG_MAX           98304u
#define BLE_IMG_W             360
#define BLE_IMG_H             360
#define BLE_IMG_FMT_JPEG      0
#define BLE_IMG_MTU_MIN       100
#define BLE_IMG_START_LEN     26

#define BLE_IMG_FLAG_START  0x01
#define BLE_IMG_FLAG_MIDDLE 0x02
#define BLE_IMG_FLAG_END    0x03

#define BLE_IMG_ST_OK       0x00
#define BLE_IMG_ST_BUSY     0x01
#define BLE_IMG_ST_RX       0x02
#define BLE_IMG_ST_CRC_PKT  0x10
#define BLE_IMG_ST_CRC_FILE 0x11
#define BLE_IMG_ST_SIZE_ERR 0x12
#define BLE_IMG_ST_SEQ_ERR  0x13
#define BLE_IMG_ST_FLAG_ERR 0x14
#define BLE_IMG_ST_DONE     0x20

int ble_img_init(void);

/* STATUS Notify：code 是协议结果类型，value 是小端附加参数 */
int ble_img_status_notify(uint8_t code, uint32_t value);

#endif
