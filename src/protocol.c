#include <string.h>

#include "protocol.h"

uint8_t crc8(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
    /*
     * TODO：按 CRC-8/ATM 实现
     * crc = 0
     * 对每个字节：crc ^= byte；再循环 8 次：
     *   crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1)
     * 注意每次移位后要截断到 8 位
     */
    return 0;
}

uint16_t frame_encode(uint8_t *out, uint8_t type, uint16_t seq,
                      const uint8_t *payload, uint8_t payload_len)
{
    (void)out;
    (void)type;
    (void)seq;
    (void)payload;
    (void)payload_len;
    /*
     * TODO：
     * 1. 参数检查，payload_len > FRAME_MAX_PAYLOAD 返回 0
     * 2. 写 magic、len、type、seq（小端）
     * 3. 拷贝 payload
     * 4. 对 out+2 起、长度 4+payload_len 算 CRC，写到末尾
     * 5. 返回 6 + payload_len + 1
     */
    return 0;
}

int frame_decode(const uint8_t *in, uint16_t in_len,
                 uint8_t *type, uint16_t *seq,
                 uint8_t *payload, uint8_t *payload_len)
{
    (void)in;
    (void)in_len;
    (void)type;
    (void)seq;
    (void)payload;
    (void)payload_len;
    /*
     * TODO：
     * 1. 至少 7 字节，magic 正确
     * 2. len <= FRAME_MAX_PAYLOAD，且 in_len == 6 + len + 1
     * 3. 校验 CRC
     * 4. 填 type / seq / payload / payload_len
     */
    return -1;
}
