#include <string.h>

#include "ringbuf.h"

void rb_init(ringbuf_t *rb)
{
    (void)rb;
    /* TODO：把 head / tail / count / dropped 清零 */
}

int rb_push(ringbuf_t *rb, const uint8_t *data, uint16_t len)
{
    (void)rb;
    (void)data;
    (void)len;
    /*
     * TODO：
     * 1. 参数检查：rb/data 非空，len <= RB_SLOT_SIZE，否则返回 -1
     * 2. 队列满时：tail 前进一格（丢最旧），count 不变，dropped++，记为 r=1
     * 3. 把数据写进 head 槽，记录长度
     * 4. head 前进一步（取模），未满时 count++
     * 5. 返回 r
     */
    return -1;
}

int rb_peek(const ringbuf_t *rb, uint8_t *out, uint16_t *out_len)
{
    (void)rb;
    (void)out;
    (void)out_len;
    /* TODO：空则返回 -1；否则拷贝 tail 槽，不动指针 */
    return -1;
}

int rb_pop(ringbuf_t *rb, uint8_t *out, uint16_t *out_len)
{
    (void)rb;
    (void)out;
    (void)out_len;
    /* TODO：空则返回 -1；否则拷贝 tail 槽，tail 前进，count-- */
    return -1;
}

uint16_t rb_count(const ringbuf_t *rb)
{
    (void)rb;
    /* TODO */
    return 0;
}

uint32_t rb_dropped(const ringbuf_t *rb)
{
    (void)rb;
    /* TODO */
    return 0;
}

int rb_is_empty(const ringbuf_t *rb)
{
    (void)rb;
    /* TODO */
    return 1;
}
