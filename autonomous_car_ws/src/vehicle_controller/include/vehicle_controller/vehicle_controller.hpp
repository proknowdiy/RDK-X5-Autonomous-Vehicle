#pragma once

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/timer.hpp>

#include <ai_msgs/msg/perception_targets.hpp>

#include <chrono>
#include <string>

#include "vehicle_controller/serial_port.hpp"

//////////////////////////////////////////////////////////////
// Enums
//////////////////////////////////////////////////////////////

enum class DriveMode
{
    Manual = 0,
    Assisted,
    Autonomous,
    EmergencyStop,
    MissionComplete
};

enum class SafetyDecision
{
    Safe = 0,
    Obstacle,
    StopSign
};

//////////////////////////////////////////////////////////////
// Controller State
//////////////////////////////////////////////////////////////

enum class ControllerState
{
    FollowingLane = 0,

    TemporaryStop,

    WaitingAtStopSign,

    MissionComplete
};

//////////////////////////////////////////////////////////////
// Lane Information
//////////////////////////////////////////////////////////////

struct LaneState
{
    float steering_deg = 0.0f;
    float throttle = 0.0f;

    bool valid = false;

    rclcpp::Time last_update;
};

//////////////////////////////////////////////////////////////
// Object Type
//////////////////////////////////////////////////////////////

enum class ObjectType
{
    Unknown = 0,

    Person,
    Bicycle,
    Car,
    Motorcycle,
    Bus,
    Truck,

    Bottle,
    Chair,

    StopSign
};

struct DetectionInfo
{
    ObjectType object_type = ObjectType::Unknown;

    std::string class_name;

    float confidence = 0.0f;

    // Normalized bounding box (0~1)
    float xmin = 0.0f;
    float ymin = 0.0f;
    float xmax = 0.0f;
    float ymax = 0.0f;

    // Normalized center
    float center_x = 0.0f;
    float center_y = 0.0f;

    bool in_danger_zone = false;
};

//////////////////////////////////////////////////////////////
// Detection Information
//////////////////////////////////////////////////////////////

struct DetectionState
{
    // Raw YOLO information
    int object_count = 0;

    // Parsed detections from YOLO
    std::vector<DetectionInfo> detections;

    // Controller-relevant object
    bool obstacle_detected = false;
    bool stop_sign_detected = false;

    std::string target_name = "None";
    float confidence = 0.0f;

    bool danger_zone = false;

    rclcpp::Time last_update;
};

//////////////////////////////////////////////////////////////
// Final Driving Command
//////////////////////////////////////////////////////////////

struct DrivingCommand
{
    float steering_deg = 0.0f;

    float throttle = 0.0f;

    DriveMode mode = DriveMode::Autonomous;
};

//////////////////////////////////////////////////////////////
// Vehicle State
//////////////////////////////////////////////////////////////

struct VehicleState
{
    LaneState lane;

    DetectionState detection;

    SafetyDecision safety = SafetyDecision::Safe;

    ControllerState state =
        ControllerState::FollowingLane;

    DrivingCommand command;
};

//////////////////////////////////////////////////////////////
// Controller Configuration
//////////////////////////////////////////////////////////////

struct ControllerConfig
{
    // Default lane following
    float default_throttle = 0.4f;

    // Danger Zone (normalized image coordinates)
    float roi_xmin = 0.20f;
    float roi_xmax = 0.80f;
    float roi_ymin = 0.45f;
    float roi_ymax = 0.85f;

    // Debounce
    int detection_confirm_frames = 3;
    int clear_confirm_frames = 3;
};


//////////////////////////////////////////////////////////////
// Vehicle Controller
//////////////////////////////////////////////////////////////

class VehicleControllerNode : public rclcpp::Node
{
public:
    VehicleControllerNode();

private:

    //----------------------------------------------------
    // Callbacks
    //----------------------------------------------------

    void vehicleControlCallback(
        const geometry_msgs::msg::Twist::SharedPtr msg);

    void perceptionCallback(
        const ai_msgs::msg::PerceptionTargets::SharedPtr msg);

    void controlLoop();

    //----------------------------------------------------
    // Internal Functions
    //----------------------------------------------------

    void updateSafetyDecision();

    void updateControllerState();

    void updateDrivingCommand();

    void printCommand();

    //----------------------------------------------------
    // ROS Interfaces
    //----------------------------------------------------

    rclcpp::Subscription<
        geometry_msgs::msg::Twist>::SharedPtr
        vehicle_control_sub_;

    rclcpp::Subscription<
        ai_msgs::msg::PerceptionTargets>::SharedPtr
        perception_sub_;

    rclcpp::TimerBase::SharedPtr
        control_timer_;

    rclcpp::TimerBase::SharedPtr
        status_timer_;    

    //----------------------------------------------------
    // Vehicle State
    //----------------------------------------------------

    VehicleState vehicle_state_;
    void printVehicleStatus();

    std::string driveModeToString(DriveMode mode);

    std::string safetyDecisionToString(SafetyDecision safety);

    std::string controllerStateToString(
        ControllerState state);

    ControllerConfig config_;

    ObjectType stringToObjectType(
        const std::string &name);

    std::string objectTypeToString(
        ObjectType type);

    void printDetections();  
    bool isInsideDangerZone(float x, float y);

    rclcpp::Time obstacle_clear_time_;

    bool waiting_for_resume_ = false; 

    //----------------------------------------------------
    // Detection Debounce
    //----------------------------------------------------

    int obstacle_detect_counter_ = 0;
    int obstacle_clear_counter_ = 0; 

    SerialPort serial_port_;

    void sendAICommand();

    static constexpr char UART_DEVICE[] = "/dev/ttyACM0";
    static constexpr int UART_BAUDRATE = 115200;

};