# MiniHawk VTOL Simulation Validation

A senior design case study on a three-rotor tiltrotor in ArduPilot SITL and Gazebo. My work included setting up the simulator, designing flight-mode transition tests, inspecting MAVLink telemetry, and adjusting the simulation model and flight-control parameters to address loss of roll control during mode changes.

The simulation completed vertical takeoff, fixed-wing flight, return to hover, and automatic landing. To evaluate the return to hover, I compared a direct return with a staged return that slowed the aircraft before the mode change.

| Return trial | Peak absolute yaw rate |
| --- | ---: |
| Direct | 84.62°/s |
| Staged | 56.16°/s |

The staged trial had a 33.6% lower peak absolute yaw rate in this comparison. These were two simulation runs with different approach conditions. The result describes those runs, with physical flight verification outside this comparison.

## Reproduce the log comparison

Run `python compare_return.py` from this directory. It uses only the Python standard library. The two CSV files contain the selected return-to-hover segment from each run, with elapsed time relative to entry into hover mode.
