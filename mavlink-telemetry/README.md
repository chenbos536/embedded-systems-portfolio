# MAVLink Telemetry Collector

A read-only Python collector for a single ArduPilot vehicle. It converts MAVLink messages into JSON Lines snapshots with connection status, flight mode, GPS, speed, attitude, and optional battery status. Heartbeat timeouts and stale measurements are handled explicitly.

## Run against ArduPilot SITL

Install `pymavlink` in a Python virtual environment. In a running MAVProxy console, add an output:

```text
output add 127.0.0.1:14552
```

From this directory, start the collector:

```sh
python telemetry_service.py --connection udpin:127.0.0.1:14552 --count 10
```

Snapshots are written to standard output and appended to `logs/telemetry.jsonl`. The default rate is one snapshot per second. Use `--help` for additional options. Relative altitude is measured from home. Missing or stale values appear as `null`.

## Test

```sh
python -m unittest discover -s tests -v
```

The unit tests use synthetic messages and cover field and unit conversion, missing messages, stale data, and disconnects. They do not need a running simulator or physical aircraft.
