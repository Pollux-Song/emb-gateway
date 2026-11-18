#include <string.h>

#include "ringbuf.h"

void rb_init(ringbuf_t *rb)
{
    if (!rb) return;
    rb->head = 0;      // 下一个写入位置：从0开始
    rb->tail = 0;      // 下一个读取位置：从0开始
    rb->count = 0;     // 当前有效条数：0
    rb->dropped = 0;   // 丢弃计数：0
}

int rb_push(ringbuf_t *rb, const uint8_t *data, uint16_t len)
{
    int r = 0;  // 默认返回0：正常写入

    // 1. 参数检查：rb/data不能空，len不能超过槽位大小
    if (!rb || !data || len == 0 || len > RB_SLOT_SIZE)
        return -1;

    // 2. 队列满时：丢最旧的一条
    if (rb->count >= RB_SLOT_COUNT) {
        rb->tail = (rb->tail + 1) % RB_SLOT_COUNT;  // tail前进一格（丢最旧）
        rb->dropped++;      // 丢弃计数+1
        r = 1;              // 返回1表示满了，丢了一条
        // 注意：count不变，因为丢了一条又写了一条，总数还是满的
    } else {
        rb->count++;        // 没满，条数+1
    }

    // 3. 把数据写进head槽
    memcpy(rb->data[rb->head], data, len);  // 拷贝数据到当前写入槽
    rb->len[rb->head] = len;                 // 记录这条数据的长度

    // 4. head前进一步（取模，实现环形）
    rb->head = (rb->head + 1) % RB_SLOT_COUNT;

    return r;
}

int rb_peek(const ringbuf_t *rb, uint8_t *out, uint16_t *out_len)
{
    // 空队列：返回-1
    if (!rb || !out || !out_len || rb->count == 0)
        return -1;

    // 拷贝tail槽的数据，但不动tail和count
    memcpy(out, rb->data[rb->tail], rb->len[rb->tail]);
    *out_len = rb->len[rb->tail];

    return 0;
}

int rb_pop(ringbuf_t *rb, uint8_t *out, uint16_t *out_len)
{
    // 空队列：返回-1
    if (!rb || !out || !out_len || rb->count == 0)
        return -1;

    // 拷贝tail槽的数据
    memcpy(out, rb->data[rb->tail], rb->len[rb->tail]);
    *out_len = rb->len[rb->tail];

    // tail前进一步（出队）
    rb->tail = (rb->tail + 1) % RB_SLOT_COUNT;
    rb->count--;  // 条数-1

    return 0;
}

uint16_t rb_count(const ringbuf_t *rb)
{
    return rb ? rb->count : 0;
}

uint32_t rb_dropped(const ringbuf_t *rb)
{
    return rb ? rb->dropped : 0;
}

int rb_is_empty(const ringbuf_t *rb)
{
    return (rb && rb->count == 0) ? 1 : 0;
}
