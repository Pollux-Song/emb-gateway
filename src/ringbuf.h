#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>

/* 每个槽位存一条完整帧，避免半帧问题 */
#define RB_SLOT_SIZE    64
#define RB_SLOT_COUNT   64

typedef struct {
    uint8_t  data[RB_SLOT_COUNT][RB_SLOT_SIZE];
    uint16_t len[RB_SLOT_COUNT];
    uint16_t head;      /* 下一个写入位置 */
    uint16_t tail;      /* 下一个读取位置 */
    uint16_t count;     /* 当前有效条数 */
    uint32_t dropped;   /* 因满而丢弃的条数 */
} ringbuf_t;

void     rb_init(ringbuf_t *rb);

/* 返回 0 正常写入；1 表示队列满，丢掉了最旧的一条后写入；-1 参数错误 */
int      rb_push(ringbuf_t *rb, const uint8_t *data, uint16_t len);

/* 只看不出队。成功返回 0 并填出 out/out_len；空返回 -1 */
int      rb_peek(const ringbuf_t *rb, uint8_t *out, uint16_t *out_len);

/* 出队。成功返回 0；空返回 -1 */
int      rb_pop(ringbuf_t *rb, uint8_t *out, uint16_t *out_len);

uint16_t rb_count(const ringbuf_t *rb);
uint32_t rb_dropped(const ringbuf_t *rb);
int      rb_is_empty(const ringbuf_t *rb);

#endif /* RINGBUF_H */
