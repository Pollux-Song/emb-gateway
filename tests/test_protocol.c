#include <stdio.h>
#include <string.h>

#include "../src/protocol.h"

void test_ringbuf(void);

static int g_total;
static int g_failed;

#define CHECK_INT(name, got, expect)                                      \
    do {                                                                  \
        g_total++;                                                        \
        if ((long)(got) != (long)(expect)) {                              \
            g_failed++;                                                   \
            printf("  FAIL | %-32s got=%ld expected=%ld\n",               \
                   (name), (long)(got), (long)(expect));                  \
        }                                                                 \
    } while (0)

#define CHECK_BYTES(name, p, q, n)                                        \
    do {                                                                  \
        g_total++;                                                        \
        if (memcmp((p), (q), (size_t)(n)) != 0) {                         \
            g_failed++;                                                   \
            printf("  FAIL | %-32s bytes differ\n", (name));              \
        }                                                                 \
    } while (0)

void test_protocol(void)
{
    const uint8_t check[9] = {'1','2','3','4','5','6','7','8','9'};
    const uint8_t payload[4] = {0x11, 0x22, 0x33, 0x44};
    uint8_t frame[FRAME_MAX_LEN];
    uint8_t decoded[FRAME_MAX_PAYLOAD];
    uint8_t type = 0;
    uint8_t plen = 0;
    uint16_t seq = 0;
    uint16_t n;
    uint8_t too_big[FRAME_MAX_PAYLOAD + 1];

    printf("\n== protocol ==\n");

    CHECK_INT("crc8 check vector", crc8(check, 9), 0xF4);
    CHECK_INT("crc8 empty", crc8(check, 0), 0x00);

    memset(too_big, 0, sizeof(too_big));
    CHECK_INT("encode payload too big",
              frame_encode(frame, FRAME_TYPE_SENSOR, 0, too_big,
                           FRAME_MAX_PAYLOAD + 1), 0);
    CHECK_INT("encode null out",
              frame_encode(0, FRAME_TYPE_SENSOR, 0, payload, 4), 0);

    n = frame_encode(frame, FRAME_TYPE_SENSOR, 0x1234, payload, 4);
    CHECK_INT("frame length", n, FRAME_HEADER_LEN + 4 + FRAME_CRC_LEN);
    CHECK_INT("magic0", frame[0], FRAME_MAGIC0);
    CHECK_INT("magic1", frame[1], FRAME_MAGIC1);
    CHECK_INT("payload len", frame[2], 4);
    CHECK_INT("type", frame[3], FRAME_TYPE_SENSOR);
    CHECK_INT("seq low", frame[4], 0x34);
    CHECK_INT("seq high", frame[5], 0x12);
    CHECK_BYTES("payload", frame + 6, payload, 4);
    CHECK_INT("crc position", frame[n - 1], crc8(frame + 2, 4 + 4));

    CHECK_INT("decode ok", frame_decode(frame, n, &type, &seq,
                                        decoded, &plen), 0);
    CHECK_INT("decode type", type, FRAME_TYPE_SENSOR);
    CHECK_INT("decode seq", seq, 0x1234);
    CHECK_INT("decode len", plen, 4);
    CHECK_BYTES("decode payload", decoded, payload, 4);

    frame[7] ^= 0x01;
    CHECK_INT("decode bad crc",
              frame_decode(frame, n, &type, &seq, decoded, &plen), -1);
    frame[7] ^= 0x01;

    CHECK_INT("decode truncated",
              frame_decode(frame, (uint16_t)(n - 1), &type, &seq,
                           decoded, &plen), -1);
    CHECK_INT("decode null", frame_decode(0, 0, &type, &seq,
                                          decoded, &plen), -1);

    frame[0] = 0x00;
    CHECK_INT("decode bad magic",
              frame_decode(frame, n, &type, &seq, decoded, &plen), -1);
}

int main(void)
{
    test_ringbuf();
    test_protocol();

    printf("\n----------------------------------------\n");
    printf("TOTAL %d   FAILED %d\n", g_total, g_failed);

    if (g_failed == 0) {
        printf("全部通过。可以接 mosquitto 跑端到云了。\n");
        return 0;
    }
    printf("还有 %d 项没过。\n", g_failed);
    return 1;
}
