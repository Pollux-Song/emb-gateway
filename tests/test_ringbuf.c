#include <stdio.h>
#include <string.h>

#include "../src/ringbuf.h"

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

void test_ringbuf(void)
{
    ringbuf_t rb;
    uint8_t in[RB_SLOT_SIZE];
    uint8_t out[RB_SLOT_SIZE];
    uint16_t olen = 0;
    uint8_t first = 0;
    int i;

    printf("\n== ringbuf ==\n");

    rb_init(&rb);
    CHECK_INT("init count", rb_count(&rb), 0);
    CHECK_INT("init empty", rb_is_empty(&rb), 1);
    CHECK_INT("init dropped", rb_dropped(&rb), 0);
    CHECK_INT("pop empty", rb_pop(&rb, out, &olen), -1);
    CHECK_INT("peek empty", rb_peek(&rb, out, &olen), -1);

    memset(in, 0, sizeof(in));
    in[0] = 'a'; in[1] = 'b'; in[2] = 'c';

    CHECK_INT("push one", rb_push(&rb, in, 3), 0);
    CHECK_INT("count after push", rb_count(&rb), 1);
    CHECK_INT("empty after push", rb_is_empty(&rb), 0);

    olen = 0;
    CHECK_INT("peek ok", rb_peek(&rb, out, &olen), 0);
    CHECK_INT("peek len", olen, 3);
    CHECK_BYTES("peek data", out, in, 3);
    CHECK_INT("count after peek", rb_count(&rb), 1);

    olen = 0;
    CHECK_INT("pop ok", rb_pop(&rb, out, &olen), 0);
    CHECK_INT("pop len", olen, 3);
    CHECK_BYTES("pop data", out, in, 3);
    CHECK_INT("count after pop", rb_count(&rb), 0);

    CHECK_INT("push oversize", rb_push(&rb, in, RB_SLOT_SIZE + 1), -1);
    CHECK_INT("push null", rb_push(&rb, 0, 3), -1);

    rb_init(&rb);
    for (i = 0; i < RB_SLOT_COUNT + 5; i++) {
        memset(in, 0, sizeof(in));
        in[0] = (uint8_t)i;
        (void)rb_push(&rb, in, 1);
    }
    CHECK_INT("overflow count capped", rb_count(&rb), RB_SLOT_COUNT);
    CHECK_INT("overflow dropped", rb_dropped(&rb), 5);

    olen = 0;
    CHECK_INT("overflow peek ok", rb_peek(&rb, out, &olen), 0);
    CHECK_INT("overflow peek len", olen, 1);
    first = out[0];
    CHECK_INT("overflow keeps newest", first, 5);
}
