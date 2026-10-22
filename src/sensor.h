#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

/* 一条采样 = 4 + 2 + 2 + 1 = 9 字节（显式序列化，不带 padding） */
#define SENSOR_PAYLOAD_LEN  9

typedef struct {
    uint32_t ts;         /* 秒级 uptime */
    int16_t  temp_x100;  /* 温度 ×100，例如 2530 = 25.30℃ */
    uint16_t volt_mv;    /* 电压，毫伏 */
    uint8_t  status;     /* 低 4 位状态标志 */
} sensor_sample_t;

void    sensor_init(void);
void    sensor_sample(sensor_sample_t *s);

/* 小端显式打包，返回写入字节数；容量不足返回 0 */
uint8_t sensor_pack(const sensor_sample_t *s, uint8_t *out, uint8_t cap);

/* 解包，成功返回 0 */
int     sensor_unpack(const uint8_t *in, uint8_t len, sensor_sample_t *s);

#endif /* SENSOR_H */
