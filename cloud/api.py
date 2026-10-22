"""查询接口。

运行：uvicorn api:app --reload
"""
import sqlite3

from fastapi import FastAPI

DB_PATH = "data.db"

app = FastAPI(title="Gateway API")


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


def query(sql: str, params=()):
    init_db()
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    try:
        return [dict(row) for row in conn.execute(sql, params).fetchall()]
    finally:
        conn.close()


@app.get("/health")
def health():
    return {"ok": True}


@app.get("/devices")
def devices():
    return query(
        "SELECT device, COUNT(*) AS count, MAX(ts) AS last_ts "
        "FROM samples GROUP BY device"
    )


@app.get("/data")
def data(device: str, start: int = 0, end: int = 2147483647, limit: int = 100):
    limit = max(1, min(limit, 1000))
    return query(
        "SELECT seq, ts, temp, volt, status, recv_at FROM samples "
        "WHERE device = ? AND ts BETWEEN ? AND ? "
        "ORDER BY ts ASC LIMIT ?",
        (device, start, end, limit),
    )
