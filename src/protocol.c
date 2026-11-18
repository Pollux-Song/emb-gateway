#include <string.h>

#include "protocol.h"

uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    // CRC-8/ATM：poly=0x07, init=0x00

    if (!data) return 0;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];  // 当前字节异或进CRC
        for (int j = 0; j < 8; j++) {
            // 逐位处理：最高位是1就左移后异或多项式，否则只左移
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

uint16_t frame_encode(uint8_t *out, uint8_t type, uint16_t seq,
                      const uint8_t *payload, uint8_t payload_len)
{
    // 参数检查
    if (!out || !payload || payload_len > FRAME_MAX_PAYLOAD)
        return 0;

    uint16_t pos = 0;

    // 写帧头
    out[pos++] = FRAME_MAGIC0;        // [0] 0xAA
    out[pos++] = FRAME_MAGIC1;        // [1] 0x55
    out[pos++] = payload_len;         // [2] payload长度
    out[pos++] = type;                // [3] 帧类型
    out[pos++] = (uint8_t)(seq & 0xFF);     // [4] seq低字节（小端）
    out[pos++] = (uint8_t)(seq >> 8);       // [5] seq高字节

    // 拷贝payload
    memcpy(out + pos, payload, payload_len);
    pos += payload_len;

    // 计算CRC：从[2]开始，长度是 4(payload_len+type+seq) + payload_len
    // 即 out+2 到 out+pos（不含CRC本身）
    out[pos] = crc8(out + 2, (uint16_t)(pos - 2));
    pos++;

    return pos;  // 返回整帧长度
}

int frame_decode(const uint8_t *in, uint16_t in_len,
                 uint8_t *type, uint16_t *seq,
                 uint8_t *payload, uint8_t *payload_len)
{
    // 参数检查
    if (!in || !type || !seq || !payload || !payload_len)
        return -1;

    // 至少要有最小帧长：6头 + 0 payload + 1 CRC = 7字节
    if (in_len < FRAME_HEADER_LEN + FRAME_CRC_LEN)
        return -1;

    // 检查magic
    if (in[0] != FRAME_MAGIC0 || in[1] != FRAME_MAGIC1)
        return -1;

    uint8_t plen = in[2];

    // 长度检查
    if (plen > FRAME_MAX_PAYLOAD)
        return -1;
    if (in_len != FRAME_HEADER_LEN + plen + FRAME_CRC_LEN)
        return -1;

    // 校验CRC
    uint8_t crc_pos = FRAME_HEADER_LEN + plen;
    uint8_t expected_crc = crc8(in + 2, (uint16_t)(crc_pos - 2));
    if (in[crc_pos] != expected_crc)
        return -1;

    // 填结果
    *type = in[3];
    *seq = (uint16_t)in[4] | ((uint16_t)in[5] << 8);  // 小端组装
    memcpy(payload, in + FRAME_HEADER_LEN, plen);
    *payload_len = plen;

    return 0;
}
