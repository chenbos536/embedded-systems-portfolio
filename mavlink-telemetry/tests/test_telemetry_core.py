import math
import unittest
from types import SimpleNamespace
from telemetry_core import TelemetryState


def message(kind, **kwargs):
    return SimpleNamespace(
        get_type=lambda: kind,
        get_srcSystem=lambda: 1,
        **kwargs
    )


class TelemetryTests(unittest.TestCase):
    def setUp(self):
        self.state = TelemetryState()
        self.kwargs = {
            "mode_decoder": lambda msg: "GUIDED",
            "armed_mask": 128,
            "invalid_autopilot": 8,
        }

    def test_all_fields_and_units(self):
        self.state.update(message("HEARTBEAT", autopilot=3, base_mode=128), 10, **self.kwargs)
        self.state.update(message("GLOBAL_POSITION_INT", lat=-353632620, lon=1491652370,
                                  relative_alt=5000, vx=300, vy=400), 10, **self.kwargs)
        self.state.update(message("ATTITUDE", roll=math.pi/2, pitch=-math.pi/6,
                                  yaw=-math.pi/2), 10, **self.kwargs)
        self.state.update(message("SYS_STATUS", voltage_battery=15600,
                                  battery_remaining=80), 10, **self.kwargs)
        self.state.update(message("GPS_RAW_INT", fix_type=3,
                                  satellites_visible=12), 10, **self.kwargs)
        data = self.state.snapshot(11)
        self.assertTrue(data["connected"])
        self.assertTrue(data["armed"])
        self.assertEqual(data["flight_mode"], "GUIDED")
        self.assertEqual(data["latitude"], -35.363262)
        self.assertEqual(data["longitude"], 149.165237)
        self.assertEqual(data["relative_altitude_m"], 5)
        self.assertEqual(data["ground_speed_m_s"], 5)
        self.assertEqual(data["yaw_deg"], 270)
        self.assertEqual(data["battery_voltage_v"], 15.6)
        self.assertEqual(data["gps_fix_type"], 3)

    def test_missing_battery_and_disconnect_do_not_use_stale_data(self):
        self.state.update(message("HEARTBEAT", autopilot=3, base_mode=0), 10, **self.kwargs)
        self.state.update(message("SYS_STATUS", voltage_battery=65535,
                                  battery_remaining=-1), 10, **self.kwargs)
        self.assertIsNone(self.state.snapshot(11)["battery_voltage_v"])
        self.assertIsNone(self.state.snapshot(11)["battery_remaining_pct"])
        data = self.state.snapshot(15)
        self.assertFalse(data["connected"])
        self.assertIsNone(data["armed"])

    def test_missing_messages_remain_null(self):
        self.state.update(message("HEARTBEAT", autopilot=3, base_mode=0), 10, **self.kwargs)
        data = self.state.snapshot(11)
        self.assertIsNone(data["latitude"])
        self.assertIsNone(data["roll_deg"])


if __name__ == "__main__":
    unittest.main()
