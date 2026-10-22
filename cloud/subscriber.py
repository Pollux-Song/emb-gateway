"""订阅设备数据、校验帧、落库。

运行：python3 subscriber.py
"""
import sqlite3
import struct
import time

import paho.mqtt.client as mqtt

DB_PATH = "data.db"
TOPIC = "dev/+/data"


def crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def parse_frame(buf: bytes):
    if len(buf) < 7:
        return None
    if buf[0] != 0xAA or buf[1] != 0x55:
        return None

    plen = buf[2]
    if plen > 32 or len(buf) != 6 + plen + 1:
        return None
    if crc8(buf[2:6 + plen]) != buf[6 + plen]:
        return None

    ftype = buf[3]
    seq = buf[4] | (buf[5] << 8)
    return ftype, seq, buf[6:6 + plen]


def init_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS samples (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            device  TEXT    NOT NULL,
            seq     INTEGER NOT NULL,
            ts      INTEGER NOT NULL,
            temp    REAL    NOT NULL,
            volt    INTEGER NOT NULL,
            status  INTEGER NOT NULL,
            recv_at INTEGER NOT NULL,
            UNIQUE(device, seq)
        )
        """
    )
    conn.commit()
    conn.close()


def on_connect(client, userdata, flags, rc):
    print(f"[mqtt] connected rc={rc}")
    client.subscribe(TOPIC)
    print(f"[mqtt] subscribed {TOPIC}")


def on_message(client, userdata, msg):
    parts = msg.topic.split("/")
    if len(parts) < 3:
        return
    device = parts[1]

    parsed = parse_frame(msg.payload)
    if parsed is None:
        print(f"[warn] bad frame from {device}, {len(msg.payload)} bytes")
        return

    ftype, seq, payload = parsed
    if ftype != 0x01 or len(payload) != 9:
        return

    ts, temp_x100, volt, status = struct.unpack("<IhHB", payload)

    conn = sqlite3.connect(DB_PATH)
    cur = conn.execute(
        "INSERT OR IGNORE INTO samples"
        "(device, seq, ts, temp, volt, status, recv_at)"
        " VALUES (?,?,?,?,?,?,?)",
        (device, seq, ts, temp_x100 / 100.0, volt, status, int(time.time())),
    )
    inserted = cur.rowcount
    conn.commit()
    conn.close()

    if inserted:
        print(f"[db] {device} seq={seq} temp={temp_x100/100:.2f}C "
              f"volt={volt}mV status={status}")


def main():
    init_db()
    client = mqtt.Client(client_id="cloud-subscriber")
    client.on_connect = on_connect
    client.on_message = on_message

    print("[mqtt] connecting 127.0.0.1:1883 ...")
    client.connect("127.0.0.1", 1883, 60)
    client.loop_forever()


if __name__ == "__main__":
    main()
