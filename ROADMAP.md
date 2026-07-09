
# RDK X5 Autonomous Vehicle
## Development Roadmap

**Version:** 1.1

---

# Project Objective

Develop a 1/10 scale autonomous vehicle using the D-Robotics RDK X5 capable of:

- Road tracking using **D-Robotics Racing Track Detection (ResNet18)**
- Vehicle & obstacle detection using **YOLOv11**
- Autonomous lane following
- Autonomous lane changing
- ROS2-based software architecture
- Manual RC override

---

# Milestones

| ID | Milestone | Exit Criteria |
|----|-----------|---------------|
| M0 | Stage 1 | RDK Studio, Stereo Camera, ResNet18 demo, YOLO demo and UART verified |
| M1 | Stage 2 | Proposal, Architecture and Roadmap completed |
| M2 | Vehicle Assembly | Complete mechanical platform |
| M3 | ROS2 Bring-up | All ROS2 nodes communicate correctly |
| M4 | ResNet18 Integration | Road tracking works continuously |
| M5 | YOLOv11 Integration | Vehicles and obstacles detected in real time |
| M6 | Multi-AI Pipeline | ResNet18 and YOLO execute simultaneously |
| M7 | Behavior Planner | Lane following and lane change implemented |
| M8 | Autonomous Vehicle | Complete autonomous lap |
| M9 | Final Release | Video, documentation and GitHub release |

---

# Acceptance Criteria

| ID | Goal |
|----|------|
| G1 | Camera ≥30 FPS |
| G2 | ResNet18 ≥20 FPS |
| G3 | YOLOv11 ≥15 FPS |
| G4 | Concurrent inference on RDK X5 |
| G5 | Autonomous road tracking |
| G6 | Obstacle avoidance |
| G7 | Autonomous lane change |
| G8 | Manual override available |

---

# Stage 1 Assets Reused

- RDK Studio setup
- Ubuntu image
- Stereo Vision MIPI Camera
- D-Robotics Racing Track Detection (ResNet18)
- YOLO Demo
- UART Communication
- GitHub repository

---

# Planned Releases

| Version | Description |
|---------|-------------|
| v0.1 | Stage 1 Complete |
| v0.2 | Vehicle Platform |
| v0.3 | ROS2 Bring-up |
| v0.4 | ResNet18 Integration |
| v0.5 | YOLOv11 Integration |
| v0.6 | Multi-AI Pipeline |
| v0.7 | Behavior Planner |
| v0.8 | Autonomous Driving |
| v1.0 | Final Challenge Submission |
