"""Read-only MAVLink -> JSONL telemetry service for SITL and companion computer.

Run with an independent MAVProxy UDP output, e.g.:
    MAVProxy> output add 127.0.0.1:14552
    python telemetry_service.py --connection udpin:127.0.0.1:14552
"""
import argparse
import json
from pathlib import Path
import sys
import time

from telemetry_core import TelemetryState


def main():
    parser = argparse.ArgumentParser(description="Read-only MAVLink telemetry collector")
    parser.add_argument("--connection", default="udpin:127.0.0.1:14552",
                        help="pymavlink endpoint: e.g. udpin:127.0.0.1:14552")
    parser.add_argument("--rate", type=float, default=1.0,
                        help="JSON snapshots per second (default 1 Hz)")
    parser.add_argument("--count", type=int, default=0,
                        help="Stop after N snapshots; 0 means continuous")
    parser.add_argument("--source", default="sitl", help="Label source (sitl/pi5)")
    parser.add_argument("--output", default="logs/telemetry.jsonl",
                        help="JSON Lines log path; use '-' to disable log file")
    args = parser.parse_args()
    if args.rate <= 0 or args.count < 0:
        parser.error("--rate must be > 0 and --count must be >= 0")

    try:
        from pymavlink import mavutil
    except ImportError as exc:
        raise SystemExit("pymavlink is missing: activate ~/venv-ardupilot") from exc

    output = None
    if args.output != "-":
        path = Path(args.output)
        path.parent.mkdir(parents=True, exist_ok=True)
        output = path.open("a", encoding="utf-8", buffering=1)

    print(f"Listening on {args.connection}; output rate {args.rate:g} Hz",
          file=sys.stderr)
    if output:
        print(f"Saving JSON lines to {args.output}", file=sys.stderr)

    conn = mavutil.mavlink_connection(args.connection)
    state = TelemetryState()
    interval = 1.0 / args.rate
    next_emit = time.monotonic() + interval
    emitted = 0
    try:
        while True:
            remaining = max(0.0, next_emit - time.monotonic())
            # Receive MAVLink traffic without starving periodic JSON output.
            msg = conn.recv_match(blocking=True, timeout=min(remaining, 0.2))
            if msg is not None:
                state.update(
                    msg, time.monotonic(),
                    mode_decoder=mavutil.mode_string_v10,
                    armed_mask=mavutil.mavlink.MAV_MODE_FLAG_SAFETY_ARMED,
                    invalid_autopilot=mavutil.mavlink.MAV_AUTOPILOT_INVALID,
                )
            now = time.monotonic()
            if now >= next_emit:
                line = json.dumps(state.snapshot(now, source=args.source),
                                  ensure_ascii=False, allow_nan=False)
                print(line, flush=True)
                if output:
                    output.write(line + "\n")
                emitted += 1
                if args.count and emitted >= args.count:
                    break
                next_emit += interval
                if next_emit < now:
                    next_emit = now + interval
    except KeyboardInterrupt:
        print("Stopped.", file=sys.stderr)
    finally:
        if output:
            output.close()
        conn.close()


if __name__ == "__main__":
    main()
