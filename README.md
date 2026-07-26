# RDK X5 Autonomous Vehicle

![Status](https://img.shields.io/badge/Status-Completed-brightgreen)
![Challenge](https://img.shields.io/badge/Challenge-Robotics%20Dream%20Keeper-blue)
![Stage](https://img.shields.io/badge/Stage-3%20Launch-success)
[![Platform: RDK X5](https://img.shields.io/badge/Brain-RDK%20X5%20·%2010%20TOPS%20BPU-e8491d.svg)](https://developer.d-robotics.cc/)
[![Sub-MCU: ESP32-C3](https://img.shields.io/badge/Sub--MCU-ESP32--C3-blue.svg)](https://www.espressif.com/)
![ROS2](https://img.shields.io/badge/ROS2-Humble-22314E?logo=ros)
![License](https://img.shields.io/badge/License-MIT-green)

An AI-powered **1/10 scale autonomous vehicle** built using the **D-Robotics RDK X5** and **ROS 2**, demonstrating real-time lane following, object detection, autonomous driving, and manual override.

This project was developed as part of the **D-Robotics Robotics Dream Keeper Challenge 2026**.

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/hero.jpg" width="850">
</p>

> **Demo Video:** *(Add YouTube Link Here)*

---

# Features

- 🚗 Autonomous lane following using OpenCV
- 🤖 YOLOv11 object detection
- 🛑 Stop sign recognition and safe vehicle stop
- 🚧 Obstacle detection with automatic stopping
- 🔄 Manual / Autonomous driving modes
- 🧠 ROS 2 modular software architecture
- 📷 Stereo Vision MIPI Camera
- 🔌 UART communication with ESP32-C3
- ⚡ Real-time inference on the RDK X5 BPU

---

# System Overview

```text
Stereo Vision Camera
          │
          ▼
+---------------------------+
|      RDK X5 (ROS 2)        |
|---------------------------|
| Lane Detection (OpenCV)   |
| YOLOv11 Object Detection  |
| Vehicle Controller        |
+---------------------------+
          │
      USB Serial
          │
          ▼
+---------------------------+
|      ESP32-C3             |
|---------------------------|
| Steering Servo            |
| ESC Control               |
| RC Receiver Interface     |
+---------------------------+
          │
          ▼
     Autonomous Vehicle
```

---

# Hardware

| Component | Description |
|-----------|-------------|
| Main Controller | D-Robotics RDK X5 |
| Camera | D-Robotics Stereo Vision MIPI Camera |
| Vehicle Controller | DFRobot Beetle ESP32-C3 |
| Steering Servo | MG996R |
| ESC | RadioLink 90A Brushed ESC |
| Motor | RS540 Brushed Motor |
| Battery | 3S 1000mAh LiPo |
| Radio System | RadioLink RC6GS |

---

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/Hardware-1.jpg" width="750">
</p>

# Software

- Ubuntu 22.04
- ROS 2 Humble
- OpenCV
- YOLOv11
- C++
- Python

---

# Repository Structure

```text
RDK-X5-Autonomous-Vehicle
│
├── autonomous_car_ws/
│   └── src/
│       ├── autonomous_vehicle/
│       ├── lane_detection/
│       └── vehicle_controller/
│
├── esp32_firmware/
│
├── docs/
│   ├── STAGE1.md
│   ├── PROPOSAL.md
│   ├── ROADMAP.md
│   └── STAGE3.md
│
├── images/
│
└── README.md
```

---

# Building the Workspace

Clone the repository:

```bash
git clone https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle.git

cd RDK-X5-Autonomous-Vehicle/autonomous_car_ws
```

Build:

```bash
colcon build
```

Source the workspace:

```bash
source install/setup.bash
```

Launch:

```bash
ros2 launch autonomous_vehicle autonomous_vehicle.launch.py
```

---

# Vehicle Operation

The vehicle supports two operating modes:

- **Manual Mode** – Controlled using the RadioLink RC transmitter.
- **Autonomous Mode** – Lane following and object detection executed on the RDK X5 while the ESP32-C3 controls steering and throttle.

When a stop sign or obstacle is detected within the configured safety region, the vehicle safely stops.

---

### Lane Detection + Object Detection

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-4.jpg" width="900">
</p>

# Performance

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-2.jpg" width="900">
</p>

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-1.jpg" width="900">
</p>



# Results

✔ OpenCV-based lane following

✔ YOLOv11 object detection

✔ ROS 2 modular architecture

✔ Manual / Autonomous mode switching

✔ UART communication between RDK X5 and ESP32-C3

✔ Real-time vehicle control

<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-6.jpg" width="900">
</p>
<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-8.jpg" width="900">
</p>
<p align="center">
  <img src="https://github.com/proknowdiy/RDK-X5-Autonomous-Vehicle/blob/main/images/benchmarks/RDK-X5_Performance%20-7.jpg" width="900">
</p>

---

# Documentation

Detailed challenge documentation is available in the **docs** directory.

| Document | Description |
|----------|-------------|
| STAGE1.md | Ignite Challenge Submission |
| PROPOSAL.md | Stage 2 Proposal |
| ROADMAP.md | Development Roadmap |
| STAGE3.md | Final Launch Challenge Submission |

---

# Future Improvements

- Dynamic obstacle avoidance
- Autonomous lane changing
- Traffic sign behaviour expansion
- Depth estimation using stereo vision
- SLAM and autonomous navigation

---

# Acknowledgements

Developed using:

- D-Robotics RDK X5
- ROS 2
- OpenCV
- Ultralytics YOLO
- ESP32-C3

Special thanks to the **D-Robotics Robotics Dream Keeper Challenge 2026** for providing the opportunity to build this project.

---

# License

This project is licensed under the MIT License.

---

# Author

**Vishal Sharma**

**YouTube:** https://www.youtube.com/@proknow

If you found this project useful, consider ⭐ starring the repository.
