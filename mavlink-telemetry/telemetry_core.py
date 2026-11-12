"""Read-only state aggregation for MAVLink telemetry messages.

This module intentionally has no pymavlink dependency, so conversion and
staleness behavior can be unit-tested independently of a running aircraft.
"""
from datetime import datetime, timezone
import math

_FIELDS_BY_GROUP = {
    "HEARTBEAT": ("flight_mode", "armed"),
    "GLOBAL_POSITION_INT": (
        "latitude", "longitude", "relative_altitude_m", "ground_speed_m_s"),
    "ATTITUDE": ("roll_deg", "pitch_deg", "yaw_deg"),
    "SYS_STATUS": ("battery_voltage_v", "battery_remaining_pct"),
    "GPS_RAW_INT": ("gps_fix_type", "satellites_visible"),
}


def _nullable_voltage(millivolts):
    # MAVLink SYS_STATUS uses UINT16_MAX (65535) for an unavailable voltage.
    return round(millivolts / 1000.0, 3) if 0 < millivolts < 65535 else None


def _nullable_percent(pct):
    return pct if 0 <= pct <= 100 else None


class TelemetryState:
    """Keep the newest values and discard old values when publishing a snapshot."""

    def __init__(self, heartbeat_timeout_s=3.0, data_timeout_s=5.0):
        self.heartbeat_timeout_s = heartbeat_timeout_s
        self.data_timeout_s = data_timeout_s
        self.values = {}
        self.seen = {}
        self.last_heartbeat = None
        self.vehicle_sysid = None

    def update(self, msg, now, mode_decoder, armed_mask, invalid_autopilot):
        group = msg.get_type()
        if group not in _FIELDS_BY_GROUP:
            return
        if group == "HEARTBEAT":
            if msg.autopilot == invalid_autopilot:
                return  # Do not interpret GCS or camera heartbeats as a vehicle.
            if self.vehicle_sysid is None:
                self.vehicle_sysid = msg.get_srcSystem()
            if msg.get_srcSystem() != self.vehicle_sysid:
                return
            self.last_heartbeat = now
            self.values.update({
                "flight_mode": mode_decoder(msg),
                "armed": bool(msg.base_mode & armed_mask),
            })
        else:
            # Single-vehicle implementation; ignore telemetry from other systems
            # after we know which vehicle we are monitoring.
            if self.vehicle_sysid is not None and msg.get_srcSystem() != self.vehicle_sysid:
                return
            if group == "GLOBAL_POSITION_INT":
                self.values.update({
                    "latitude": round(msg.lat / 1e7, 7),
                    "longitude": round(msg.lon / 1e7, 7),
                    "relative_altitude_m": round(msg.relative_alt / 1000.0, 3),
                    "ground_speed_m_s": round(math.hypot(msg.vx, msg.vy) / 100.0, 3),
                })
            elif group == "ATTITUDE":
                self.values.update({
                    "roll_deg": round(math.degrees(msg.roll), 2),
                    "pitch_deg": round(math.degrees(msg.pitch), 2),
                    "yaw_deg": round(math.degrees(msg.yaw) % 360.0, 2),
                })
            elif group == "SYS_STATUS":
                self.values.update({
                    "battery_voltage_v": _nullable_voltage(msg.voltage_battery),
                    "battery_remaining_pct": _nullable_percent(msg.battery_remaining),
                })
            elif group == "GPS_RAW_INT":
                satellites = msg.satellites_visible
                self.values.update({
                    "gps_fix_type": msg.fix_type,
                    "satellites_visible": None if satellites == 255 else satellites,
                })
        self.seen[group] = now

    def snapshot(self, now, source="sitl"):
        heartbeat_age_s = (
            round(max(0.0, now - self.last_heartbeat), 2)
            if self.last_heartbeat is not None else None
        )
        connected = (heartbeat_age_s is not None
                     and heartbeat_age_s <= self.heartbeat_timeout_s)
        result = {
            "schema_version": 1,
            "source": source,
            "timestamp": datetime.now(timezone.utc).isoformat(
                timespec="milliseconds").replace("+00:00", "Z"),
            "connected": connected,
            "heartbeat_age_s": heartbeat_age_s,
        }
        for group, fields in _FIELDS_BY_GROUP.items():
            fresh = (connected and group in self.seen
                     and now - self.seen[group] <= self.data_timeout_s)
            for field in fields:
                result[field] = self.values.get(field) if fresh else None
        return result
