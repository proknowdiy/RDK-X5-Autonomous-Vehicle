cmake_minimum_required(VERSION 3.8)
project(lane_detection)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Find Dependencies
# -----------------------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(hbm_img_msgs REQUIRED)
find_package(ai_msgs REQUIRED)

# -----------------------------
# Executable
# -----------------------------
add_executable(lane_detector_node
  src/main.cpp
  src/lane_detector.cpp
)

ament_target_dependencies(
  lane_detector_node
  rclcpp
  sensor_msgs
  cv_bridge
  hbm_img_msgs
  ai_msgs
)

target_include_directories(lane_detector_node PUBLIC
  include
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
  lane_detector_node
  ${OpenCV_LIBS}
)

# -----------------------------
# Install
# -----------------------------
install(TARGETS
  lane_detector_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
