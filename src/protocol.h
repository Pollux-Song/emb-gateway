#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define FRAME_MAGIC0        0xAA
#define FRAME_MAGIC1        0x55
#define FRAME_MAX_PAYLOAD   32

/* magic(2) + len(1) + type(1) + seq(2) = 6 字节头 */
#define FRAME_HEADER_LEN    6
#define FRAME_CRC_LEN       1
#define FRAME_MAX_LEN       (FRAME_HEADER_LEN + FRAME_MAX_PAYLOAD + FRAME_CRC_LEN)

#define FRAME_TYPE_SENSOR   0x01

/*
 * 帧格式：
 *   [0]     0xAA
 *   [1]     0x55
 *   [2]     payload_len
 *   [3]     type
 *   [4..5]  seq，小端
 *   [6..]   payload
 *   [6+len] CRC8，计算范围是 [2, 6+len)
 */

/* CRC-8/ATM：poly=0x07, init=0x00。校验值 crc8("123456789") == 0xF4 */
uint8_t  crc8(const uint8_t *data, uint16_t len);

/* 返回整帧长度；payload_len 超限或参数非法返回 0 */
uint16_t frame_encode(uint8_t *out, uint8_t type, uint16_t seq,
                      const uint8_t *payload, uint8_t payload_len);

/* 成功返回 0，magic/长度/CRC 任一不对返回 -1 */
int frame_decode(const uint8_t *in, uint16_t in_len,
                 uint8_t *type, uint16_t *seq,
                 uint8_t *payload, uint8_t *payload_len);

#endif /* PROTOCOL_H */
