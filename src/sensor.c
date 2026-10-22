#include "sensor.h"

static uint32_t s_tick;

void sensor_init(void)
{
    s_tick = 0;
}

void sensor_sample(sensor_sample_t *s)
{
    uint32_t n;

    if (s == 0) {
        return;
    }

    n = s_tick++;

    s->ts        = n / 5U;                       /* 每 5 次采样算 1 秒 */
    s->temp_x100 = (int16_t)(2000 + (n * 37U) % 1001U);   /* 20.00 ~ 30.00 */
    s->volt_mv   = (uint16_t)(3250U + (n * 13U) % 101U);  /* 3250 ~ 3350 mV */
    s->status    = (uint8_t)(n & 0x0FU);
}

uint8_t sensor_pack(const sensor_sample_t *s, uint8_t *out, uint8_t cap)
{
    uint16_t t;

    if (s == 0 || out == 0 || cap < SENSOR_PAYLOAD_LEN) {
        return 0;
    }

    out[0] = (uint8_t)(s->ts);
    out[1] = (uint8_t)(s->ts >> 8);
    out[2] = (uint8_t)(s->ts >> 16);
    out[3] = (uint8_t)(s->ts >> 24);

    t = (uint16_t)s->temp_x100;
    out[4] = (uint8_t)(t);
    out[5] = (uint8_t)(t >> 8);

    out[6] = (uint8_t)(s->volt_mv);
    out[7] = (uint8_t)(s->volt_mv >> 8);

    out[8] = s->status;

    return SENSOR_PAYLOAD_LEN;
}

int sensor_unpack(const uint8_t *in, uint8_t len, sensor_sample_t *s)
{
    uint16_t t;

    if (in == 0 || s == 0 || len < SENSOR_PAYLOAD_LEN) {
        return -1;
    }

    s->ts = (uint32_t)in[0]
          | ((uint32_t)in[1] << 8)
          | ((uint32_t)in[2] << 16)
          | ((uint32_t)in[3] << 24);

    t = (uint16_t)in[4] | ((uint16_t)in[5] << 8);
    s->temp_x100 = (int16_t)t;

    s->volt_mv = (uint16_t)in[6] | ((uint16_t)in[7] << 8);
    s->status  = in[8];

    return 0;
}
