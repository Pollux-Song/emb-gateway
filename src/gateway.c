#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "protocol.h"
#include "ringbuf.h"
#include "sensor.h"

#ifdef USE_MQTT
#include <mosquitto.h>
#endif

#define DEVICE_ID   "dev-001"
#define MQTT_HOST   "127.0.0.1"
#define MQTT_PORT   1883
#define TOPIC       "dev/" DEVICE_ID "/data"

#ifdef USE_MQTT
static int g_connected;

static void on_connect(struct mosquitto *m, void *user, int rc)
{
    (void)m;
    (void)user;
    g_connected = (rc == 0);
    printf("[mqtt] %s\n", g_connected ? "connected" : "connect failed");
}

static void on_disconnect(struct mosquitto *m, void *user, int rc)
{
    (void)m;
    (void)user;
    (void)rc;
    g_connected = 0;
    printf("[mqtt] disconnected, keep buffering\n");
}
#endif

static uint16_t build_frame(uint16_t seq, uint8_t *out)
{
    sensor_sample_t s;
    uint8_t payload[SENSOR_PAYLOAD_LEN];
    uint8_t plen;

    sensor_sample(&s);
    plen = sensor_pack(&s, payload, sizeof(payload));
    return frame_encode(out, FRAME_TYPE_SENSOR, seq, payload, plen);
}

int main(int argc, char **argv)
{
    ringbuf_t rb;
    uint8_t frame[FRAME_MAX_LEN];
    uint8_t out[RB_SLOT_SIZE];
    uint16_t flen;
    uint16_t olen;
    uint16_t seq = 0;
    unsigned long round = 0;
    unsigned long rounds = (argc > 1) ? strtoul(argv[1], 0, 10) : 0UL;

#ifdef USE_MQTT
    struct mosquitto *mosq = 0;
#endif

    rb_init(&rb);
    sensor_init();

#ifdef USE_MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new(DEVICE_ID, true, 0);
    if (mosq == 0) {
        fprintf(stderr, "mosquitto_new failed\n");
        return 1;
    }
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        printf("[mqtt] broker 不可用，进入离线缓存模式\n");
    }
    if (mosquitto_loop_start(mosq) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_loop_start failed\n");
    }
#endif

    printf("[gateway] start, device=%s, rounds=%s\n",
           DEVICE_ID, rounds ? "limited" : "infinite");

    for (;;) {
        int pushed;
        int sent = 0;

        flen = build_frame(seq++, frame);
        if (flen == 0) {
            fprintf(stderr, "[gateway] frame_encode failed\n");
            break;
        }

        pushed = rb_push(&rb, frame, flen);
        if (pushed > 0) {
            printf("[ring] full: drop oldest, dropped=%u\n",
                   (unsigned)rb_dropped(&rb));
        }

        if (rb_peek(&rb, out, &olen) == 0) {
#ifdef USE_MQTT
            if (g_connected &&
                mosquitto_publish(mosq, 0, TOPIC, (int)olen, out, 0, false)
                    == MOSQ_ERR_SUCCESS) {
                sent = 1;
            }
#else
            printf("[sim] seq=%u len=%u buffered=%u\n",
                   (unsigned)seq, (unsigned)olen, (unsigned)rb_count(&rb));
            sent = 1;
#endif
            if (sent) {
                rb_pop(&rb, out, &olen);
            }
        }

        round++;
        if (rounds != 0UL && round >= rounds) {
            break;
        }
        usleep(200000);
    }

    printf("[gateway] done: buffered=%u dropped=%u\n",
           (unsigned)rb_count(&rb), (unsigned)rb_dropped(&rb));

#ifdef USE_MQTT
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
#endif

    return 0;
}
