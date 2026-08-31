#!/usr/bin/env python3
"""Capture JSONL windows emitted by tinyml_shadow_h0.ino.

Each valid JSON line is flushed to disk immediately so a cable/power interruption
preserves all earlier windows. The output is intentionally local-only under
hardware/tinyml/data/raw/.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from statistics import fmean

import serial


TINYML_ROOT = Path(__file__).resolve().parents[1]
RAW_DIRECTORY = TINYML_ROOT / "data" / "raw"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect ESP32-S3 TinyML Shadow PoC JSON windows into JSONL."
    )
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--label", required=True, help="Run label, e.g. normal_pwm150_run1")
    parser.add_argument("--seconds", type=float, default=20.0, help="Capture duration (default: 20)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    args = parser.parse_args()
    if args.seconds <= 0:
        parser.error("--seconds must be greater than zero")
    if args.baud <= 0:
        parser.error("--baud must be greater than zero")
    return args


def output_path_for(label: str) -> Path:
    safe_label = re.sub(r"[^A-Za-z0-9_.-]+", "_", label).strip("._") or "capture"
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    return RAW_DIRECTORY / f"{safe_label}_{timestamp}.jsonl"


def send_command(device: serial.Serial, command: str) -> None:
    device.write(f"{command}\n".encode("ascii"))
    device.flush()


def main() -> int:
    args = parse_args()
    RAW_DIRECTORY.mkdir(parents=True, exist_ok=True)
    output_path = output_path_for(args.label)

    try:
        device = serial.Serial(args.port, args.baud, timeout=0.25)
    except serial.SerialException as exc:
        print(f"無法開啟序列埠 {args.port}: {exc}", file=sys.stderr)
        return 2

    windows_received = 0
    actual_sample_rates: list[float] = []
    interrupted = False
    capture_start = 0.0

    try:
        # Opening an ESP32 serial port can reset it. Let setup() finish, then discard
        # its boot status because the capture starts with an explicit `start` command.
        time.sleep(1.0)
        device.reset_input_buffer()
        with output_path.open("a", encoding="utf-8", buffering=1) as output_file:
            send_command(device, "start")
            capture_start = time.monotonic()
            deadline = capture_start + args.seconds
            print(f"開始收集 {args.label}，輸出：{output_path}")

            while time.monotonic() < deadline:
                raw_line = device.readline()
                if not raw_line:
                    continue

                line = raw_line.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                try:
                    payload = json.loads(line)
                except json.JSONDecodeError:
                    print(f"警告：略過非 JSON 序列資料：{line[:160]!r}", file=sys.stderr)
                    continue

                # Append and flush every valid JSON line; status/error records make
                # post-run diagnosis possible, while the analyser uses only windows.
                output_file.write(json.dumps(payload, separators=(",", ":")) + "\n")
                output_file.flush()

                if payload.get("type") == "window":
                    windows_received += 1
                    rate = payload.get("sample_rate_hz")
                    if isinstance(rate, (int, float)) and rate > 0:
                        actual_sample_rates.append(float(rate))
                    print(f"已收到 window {windows_received}（{rate} Hz）")
    except KeyboardInterrupt:
        interrupted = True
        print("\n收到 Ctrl+C，正在要求裝置停止串流…", file=sys.stderr)
    except serial.SerialException as exc:
        print(f"序列傳輸錯誤：{exc}", file=sys.stderr)
    finally:
        try:
            send_command(device, "stop")
        except serial.SerialException as exc:
            print(f"警告：無法送出 stop 指令：{exc}", file=sys.stderr)
        finally:
            device.close()

    elapsed = time.monotonic() - capture_start if capture_start else 0.0
    average_rate = fmean(actual_sample_rates) if actual_sample_rates else None
    print(f"收集結束：{windows_received} 個完整 window，耗時 {elapsed:.1f} 秒")
    if average_rate is None:
        print("平均實際取樣率：無完整 window 可計算")
    else:
        print(f"平均實際取樣率：{average_rate:.2f} Hz")
    print(f"資料保留於：{output_path}")
    return 130 if interrupted else 0


if __name__ == "__main__":
    raise SystemExit(main())
