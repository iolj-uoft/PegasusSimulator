# Portfolio Plan — Hyarks UAV & USV Roles
---

## Gap analysis

### UAV role
| Requirement | Current status | Closed by |
|---|---|---|
| Hands-on PX4 / ArduPilot | Gap | Project 1, Phases 1 and 4 |
| Flight controller architecture, MAVLink | Gap | Project 1, Phases 1 and 4 |
| C++ / Python | ✅ | — |
| ROS / ROS 2 integration | Gap | Project 1, Phase 1 |
| Sim (Gazebo/AirSim) → hardware transfer | Partial (sim) | Project 1, Phase 4 |
| Deployed on a real UAV | Gap | Project 1, Phase 4 |
| Systematic reliability debugging | Partial | Project 1, Phase 3 |
| *Bonus:* perception + TF/TF2 | ✅ perception, TF2 to add | Project 1, Phase 2 |
| *Bonus:* vision-based relative navigation | ✅ (interceptor) | Reuse |
| *Bonus:* multi-agent coordination | Gap | Project 3 (combined) |
| *Bonus:* GNSS-denied navigation / precision landing | Gap | Project 1 (core topic) |

### USV role
| Requirement | Current status | Closed by |
|---|---|---|
| C++ and/or Python | ✅ | — |
| Testing on real hardware | Partial (STM32 bench) | Project 2 |
| *Bonus:* ROS / ROS 2 | Gap | Projects 1 and 2 |
| *Bonus:* embedded / microcontrollers | ✅ STM32 | Reuse |
| *Bonus:* PID, path planning, navigation | Partial (state-space background) | Project 2 |
| *Bonus:* IMU, GPS, LiDAR, cameras | IMU and camera ✅, GPS to add | Project 2 |

---

## Project 1 — Vision-Based Precision Landing on a Moving Platform

**One-line pitch:** A UAV autonomously tracks and lands on a moving, pitching deck using a fiducial marker. It is built on ArduPilot (or PX4) and ROS 2, stress-tested in simulation, and flown on real hardware.

### Architecture
```
Camera ─► Tag detector (AprilTag/ArUco) ─► Relative-state estimator (KF/LTVKF)
                                                   │
                         TF2: camera → base_link → odom/map
                                                   │
                                Landing state machine
                     (SEARCH → TRACK → DESCEND → LAND / ABORT → GO-AROUND)
                                                   │
                         Velocity setpoints over MAVLink
                         (MAVROS, or PX4 uXRCE-DDS)
                                                   │
                                ArduPilot / PX4 autopilot
```

### Phase 1 — Autopilot + ROS 2 bring-up in SITL (Week 1–2)
- [ ] Install ROS 2 (Humble or Jazzy) and Gazebo (Harmonic), plus ArduPilot SITL with `ardupilot_gz`, or PX4 SITL.
- [ ] Fly a scripted mission from a ROS 2 node: arm, take off, offboard/GUIDED velocity control, land.
- [ ] Learn the MAVLink basics: heartbeat, modes, `SET_POSITION_TARGET_LOCAL_NED`, `LANDING_TARGET`, and EKF status messages.
- [ ] Read the flight-controller architecture: the EKF3/EKF2 estimator, attitude and position controller cascade, and failsafes.
- **Deliverable:** A repo skeleton (`colcon` workspace), a README, and a short screen recording of a ROS 2-commanded flight.

### Phase 2 — Moving platform + perception + estimation (Week 3–4)
- [ ] Gazebo world with a "boat" model: constant-velocity path plus roll, pitch and heave oscillation, with an AprilTag pad on the deck.
- [ ] Downward camera on the drone and a tag-detection node that outputs the tag pose in the camera frame.
- [ ] Set up the TF2 tree properly (camera → base_link → odom), so the relative position is expressed in the right frame.
- [ ] Relative-state filter for the platform's relative position, velocity and deck motion. Reuse the LTVKF/KF work from the interceptor.
- [ ] Landing state machine with tracking control (PID or feedforward of platform velocity + feedback).
- **Deliverable:** Consistent landings in nominal conditions in sim, with plots of estimated vs. true relative state.

### Phase 3 — Robustness and failure-mode study (Week 5–6)
This is the phase that answers "identify failure modes, improve robustness, validate edge cases."
- [ ] Fault injection:
  - Tag dropout or occlusion (intermittent and long)
  - GPS degradation or loss (GNSS-denied relative navigation)
  - MAVLink latency and packet loss
  - Wind and gusts
  - Larger deck motion (sea state)
- [ ] Abort and go-around logic, timeouts, and safe-hover fallbacks.
- [ ] Monte Carlo campaign: N runs per condition, with success rate, touchdown error, and touchdown relative velocity.
- [ ] Failure-mode table: failure → symptom in logs → root cause → fix → result after the fix.
- **Deliverable:** A results section in the README with a success-rate table and before/after plots.

### Phase 4 — Hardware transfer (Week 7+; likely after arriving in Taiwan)
- [ ] **Check whether the F405 V3 board has an official ArduPilot (or PX4) target.** 1 MB-flash F405 boards run a trimmed ArduPilot feature set, so confirm GUIDED mode, precision landing and the MAVLink companion link are included. If they aren't, buy an inexpensive board with ArduPilot/PX4 support. Staying on Betaflight does not count for this role.
- [ ] Add a companion computer (Raspberry Pi 5 or Jetson Orin Nano) with a downward camera, and a MAVLink link over UART.
- [ ] Add a GPS and compass module, plus optional rangefinder or optical flow for low-altitude and GNSS-denied work.
- [ ] Tuning sequence: rate PIDs → attitude → position hold → GUIDED velocity control. Keep logs for each step.
- [ ] Progressive tests: hover over a static tag → land on static tag → land on a slowly pushed cart → land on a moving rover (from Project 2).
- [ ] Compare sim vs. real: what didn't transfer and why (latency, camera exposure or motion blur, vibration in the IMU, wind).
- **Deliverable:** A real-flight video, DataFlash/ULog log analysis, and a "sim-to-real gaps" writeup.

### Regulations (before any outdoor flight)
- [ ] Check Taiwan CAA drone registration and operating rules for the build's takeoff weight (registration applies at ≥250 g), and where flying is allowed in Kaohsiung.
- [ ] In Canada, check Transport Canada rules if flying before leaving.

---

## Project 2 — Mini USV: Embedded Sensing to ROS 2 Autonomy

**One-line pitch:** A small surface vehicle (or ground rover as a stand-in) with STM32-based IMU/GPS sensing, a ROS 2 bridge, PID heading control and line-of-sight waypoint following, tested on real hardware.

### Phase A — Sensor bench upgrade (Week 1–2, in parallel with Project 1)
- [ ] Add a GPS module (u-blox class) to the STM32F411 + MPU-6050 bench.
- [ ] Define a framed serial protocol with checksums, and log at fixed rates.
- [ ] Bridge to ROS 2: either a host-side serial node that publishes `sensor_msgs/Imu` and `NavSatFix`, or micro-ROS on the STM32.
- [ ] IMU calibration (bias, scale) and a simple heading estimate (complementary filter or EKF with GPS course).
- **Deliverable:** Live ROS 2 topics, RViz/Foxglove visualization, and recorded bags.

### Phase B — Control and guidance (Week 3–4)
- [ ] Simulate first: a simple 3-DOF surface-vessel model (surge, sway, yaw) in Python or Gazebo.
- [ ] PID heading controller: tune it and show step responses, overshoot and settling time.
- [ ] Line-of-sight (LOS) waypoint guidance with a lookahead distance, and cross-track error plots.
- [ ] Optional: compare against ArduPilot Rover in boat mode on the same hardware.

### Phase C — Real hardware (Week 5+)
- [ ] Platform: a small RC boat (pond or pool), or a rover as a stand-in in a parking lot.
- [ ] Waypoint mission test with logs, cross-track error and heading tracking plots.
- [ ] Debug writeup: issues found on hardware (GPS noise, magnetic interference, actuator saturation) and how they were fixed.
- **Deliverable:** A video, plots, and a README section on sim vs. real.

---

## Project 3 (stretch) — UAV–USV cooperation
The drone from Project 1 follows and lands on the rover or boat from Project 2.
- [ ] Shared ROS 2 graph (or MAVLink routing) so the drone receives the platform's GPS/velocity as a prior for the landing filter.
- [ ] Rendezvous logic: the drone flies to the predicted platform position, then switches to vision-based terminal landing.
- [ ] Demonstrates multi-agent coordination, which is the core of Hyarks's USV–UAV product.

---

## Timeline overview

| Week | Project 1 (UAV) | Project 2 (USV) | Job search |
|---|---|---|---|
| 1–2 | SITL + ROS 2 + MAVLink bring-up | GPS + ROS 2 bridge | **Apply to both roles** with the repo link |
| 3–4 | Moving platform, perception, TF2, estimator | PID + LOS in sim | Follow up; prep for interview topics |
| 5–6 | Fault injection, Monte Carlo, failure table | Hardware build | Update repo READMEs with results |
| 7+ | Hardware flights (likely in Taiwan) | Field tests | Bring video and logs to interviews |

---

## How to present it

- **Repo README order:** 60-second video → what it does → architecture diagram → results table → failure modes → sim-to-real notes → how to run.
- **Resume bullets (fill in once results exist):**
  - "Built a ROS 2 + ArduPilot precision-landing pipeline for a moving, pitching platform. It achieved X% landing success over N Monte Carlo runs under tag dropout, GPS loss and wind."
  - "Transferred the pipeline to custom quadrotor hardware with a companion computer over MAVLink and documented sim-to-real gaps."
  - "Developed STM32-based IMU/GPS sensing with a ROS 2 bridge and PID/LOS waypoint guidance for a small surface vehicle."
- **Interview talking points:**
  - Why the TF2 frame chain matters for relative navigation on a moving deck.
  - How the landing state machine handles tag loss and when it aborts.
  - One concrete bug found on hardware that sim never showed, and how it was diagnosed from logs.
  - How the interceptor work (guidance against a moving target) maps onto landing (rendezvous with a moving target).

---

## Hardware shopping list (check before buying)
- [ ] ArduPilot/PX4-capable flight controller (only if the F405 V3 lacks support)
- [ ] GPS + compass module
- [ ] Companion computer (Pi 5 or Jetson Orin Nano) and a global-shutter or low-latency camera
- [ ] Optional: downward rangefinder or optical-flow sensor
- [ ] Telemetry radio, or Wi-Fi to the companion computer
- [ ] GPS module for the STM32 bench
- [ ] Small RC boat or rover chassis, ESC, servo
- [ ] Printed AprilTag pads (several sizes)
