#include "vehicle_controller/vehicle_controller.hpp"
#include <geometry_msgs/msg/twist.hpp>
#include <iomanip>
#include <sstream>
#include <vector>

#include "vehicle_controller/protocol.hpp"

#include <algorithm>
#include <cmath>

VehicleControllerNode::VehicleControllerNode()
    : Node("vehicle_controller_node")
{
    RCLCPP_INFO(
        this->get_logger(),
        "Vehicle Controller Node Started");

    vehicle_control_sub_ =
        this->create_subscription<
            geometry_msgs::msg::Twist>(
                "/vehicle_control",
                10,
                std::bind(
                    &VehicleControllerNode::vehicleControlCallback,
                    this,
                    std::placeholders::_1));

    control_timer_ =
        this->create_wall_timer(
            std::chrono::milliseconds(50),
            std::bind(
                &VehicleControllerNode::controlLoop,
                this));

    status_timer_ =
    this->create_wall_timer(
        std::chrono::seconds(1),
        std::bind(
            &VehicleControllerNode::printVehicleStatus,
            this));            

    perception_sub_ =
    this->create_subscription<
        ai_msgs::msg::PerceptionTargets>(
            "/hobot_dnn_detection",
            10,
            std::bind(
                &VehicleControllerNode::perceptionCallback,
                this,
                std::placeholders::_1));

    // constexpr char UART_DEVICE[] = "/dev/ttyACM0";  // /dev/ttyACM0
    // constexpr int UART_BAUDRATE = 115200;

    if (serial_port_.openPort(
            UART_DEVICE,
            UART_BAUDRATE))
    {
        RCLCPP_INFO(
            this->get_logger(),
            "UART opened successfully: %s @ %d",
            UART_DEVICE,
            UART_BAUDRATE);
    }
    else
    {
        RCLCPP_ERROR(
            this->get_logger(),
            "Failed to open UART: %s",
            UART_DEVICE);
    }                        

}

void VehicleControllerNode::vehicleControlCallback(
    const geometry_msgs::msg::Twist::SharedPtr msg)
{
    vehicle_state_.lane.steering_deg = msg->angular.z;
    vehicle_state_.lane.throttle = msg->linear.x;
    vehicle_state_.lane.valid = true;
    vehicle_state_.lane.last_update = this->now();
}

void VehicleControllerNode::perceptionCallback(
    const ai_msgs::msg::PerceptionTargets::SharedPtr msg)
{
    auto &detection = vehicle_state_.detection;

    detection.object_count = msg->targets.size();
    detection.last_update = this->now();
    detection.detections.clear();

    constexpr float IMAGE_WIDTH  = 640.0f;
    constexpr float IMAGE_HEIGHT = 480.0f;

    for (const auto &target : msg->targets)
    {
        DetectionInfo info;

        info.class_name = target.type;
        info.object_type = stringToObjectType(target.type);

        if (!target.rois.empty())
        {
            const auto &roi = target.rois.front();

            info.confidence = roi.confidence;

            info.xmin = roi.rect.x_offset / IMAGE_WIDTH;
            info.ymin = roi.rect.y_offset / IMAGE_HEIGHT;

            info.xmax =
                (roi.rect.x_offset + roi.rect.width) / IMAGE_WIDTH;

            info.ymax =
                (roi.rect.y_offset + roi.rect.height) / IMAGE_HEIGHT;

            info.center_x =
                (info.xmin + info.xmax) * 0.5f;

            info.center_y =
                (info.ymin + info.ymax) * 0.5f;

            info.in_danger_zone =
                isInsideDangerZone(
                    info.center_x,
                    info.center_y);    
        }

        detection.detections.push_back(info);
    }

    //printDetections();
}

void VehicleControllerNode::controlLoop()
{
    // Automatically reconnect UART if needed
    if (!serial_port_.isOpen())
    {
        if (serial_port_.openPort(UART_DEVICE, UART_BAUDRATE))
        {
            RCLCPP_INFO(
                this->get_logger(),
                "UART reconnected successfully.");
        }

        return;
    }

    if (!vehicle_state_.lane.valid)
        return;

    updateSafetyDecision();

    updateControllerState();

    updateDrivingCommand();

    sendAICommand();

    //printCommand();
}

// void VehicleControllerNode::controlLoop()
// {
//     if (!vehicle_state_.lane.valid)
//         return;

//     updateSafetyDecision();

//     updateControllerState();

//     updateDrivingCommand();

//     sendAICommand();

//     //printCommand();
// }

void VehicleControllerNode::updateSafetyDecision()
{
    // Mission already completed.
    // Stay in this state until explicitly reset.
    if (vehicle_state_.state ==
        ControllerState::MissionComplete)
    {
        vehicle_state_.safety =
            SafetyDecision::StopSign;

        return;
    }
    
    auto &detection = vehicle_state_.detection;

    // Reset decision state
    vehicle_state_.safety = SafetyDecision::Safe;

    detection.obstacle_detected = false;
    detection.stop_sign_detected = false;
    detection.danger_zone = false;
    detection.target_name = "None";
    detection.confidence = 0.0f;

    bool obstacle_found = false;
    bool stop_sign_found = false;

    DetectionInfo obstacle_info;
    DetectionInfo stop_sign_info;

    for (const auto &obj : detection.detections)
    {
        if (!obj.in_danger_zone)
            continue;

        switch (obj.object_type)
        {
            case ObjectType::Person:
            case ObjectType::Bicycle:
            case ObjectType::Car:
            case ObjectType::Motorcycle:
            case ObjectType::Bus:
            case ObjectType::Truck:

                obstacle_found = true;
                obstacle_info = obj;

                break;

            case ObjectType::StopSign:

                stop_sign_found = true;
                stop_sign_info = obj;
                break;

            default:
                break;
        }

        if (obstacle_found)
            break;
    }

    //----------------------------------------------------
    // Mission Complete (Stop Sign)
    //----------------------------------------------------

    if (stop_sign_found)
    {
        vehicle_state_.safety = SafetyDecision::StopSign;

        detection.stop_sign_detected = true;
        detection.danger_zone = true;

        detection.target_name =
            objectTypeToString(stop_sign_info.object_type);

        detection.confidence =
            stop_sign_info.confidence;

        return;
    }

    //----------------------------------------------------
    // Detection Debounce
    //----------------------------------------------------

    if (obstacle_found)
    {
        obstacle_detect_counter_++;
        obstacle_clear_counter_ = 0;

        if (obstacle_detect_counter_ >=
            config_.detection_confirm_frames)
        {
            vehicle_state_.safety =
                SafetyDecision::Obstacle;

            detection.obstacle_detected = true;
            detection.danger_zone = true;

            detection.target_name =
                objectTypeToString(obstacle_info.object_type);

            detection.confidence =
                obstacle_info.confidence;
        }
    }
    else
    {
        obstacle_detect_counter_ = 0;
        obstacle_clear_counter_++;

        if (obstacle_clear_counter_ <
            config_.clear_confirm_frames)
        {
            vehicle_state_.safety =
                SafetyDecision::Obstacle;

            detection.obstacle_detected = true;
            detection.danger_zone = true;
        }
    }
}

void VehicleControllerNode::updateControllerState()
{
    switch (vehicle_state_.safety)
    {
        case SafetyDecision::StopSign:

            vehicle_state_.state =
                ControllerState::MissionComplete;
            break;

        case SafetyDecision::Obstacle:

            vehicle_state_.state =
                ControllerState::TemporaryStop;
            break;

        case SafetyDecision::Safe:

            if (vehicle_state_.state !=
                ControllerState::MissionComplete)
            {
                vehicle_state_.state =
                    ControllerState::FollowingLane;
            }
            break;
    }
}

void VehicleControllerNode::updateDrivingCommand()
{
    // Steering always follows the lane detector
    vehicle_state_.command.steering_deg =
        vehicle_state_.lane.steering_deg;

    vehicle_state_.command.mode =
        DriveMode::Autonomous;

    //----------------------------------------------------
    // Mission Complete
    //----------------------------------------------------

    if (vehicle_state_.safety == SafetyDecision::StopSign)
    {
        // vehicle_state_.state =
        //     ControllerState::MissionComplete;

        vehicle_state_.command.mode =
            DriveMode::MissionComplete;

        vehicle_state_.command.steering_deg = 0.0f;
        vehicle_state_.command.throttle = 0.0f;

        return;
    }    

    // ----------------------------------------------------
    // Obstacle detected
    // ----------------------------------------------------
    if (vehicle_state_.safety == SafetyDecision::Obstacle)
    {
        vehicle_state_.command.throttle = 0.0f;

        waiting_for_resume_ = true;
        obstacle_clear_time_ = this->now();

        return;
    }

    // ----------------------------------------------------
    // Obstacle just disappeared
    // ----------------------------------------------------
    if (waiting_for_resume_)
    {
        auto elapsed =
            (this->now() - obstacle_clear_time_).seconds();

        if (elapsed < 1.0)
        {
            vehicle_state_.command.throttle = 0.0f;
            return;
        }

        waiting_for_resume_ = false;
    }

    // ----------------------------------------------------
    // Normal driving
    // ----------------------------------------------------
    // vehicle_state_.command.throttle =
    //     vehicle_state_.lane.throttle;
    vehicle_state_.command.throttle =
        config_.default_throttle;
}

void VehicleControllerNode::printCommand()
{
    RCLCPP_INFO(
        this->get_logger(),
        "Command -> Steering: %.2f  Throttle: %.2f",
        vehicle_state_.command.steering_deg,
        vehicle_state_.command.throttle);
}

// void VehicleControllerNode::printVehicleStatus()
// {
//     RCLCPP_INFO(
//         this->get_logger(),
//         "\n"
//         "=========================================================\n"
//         "            VEHICLE CONTROLLER STATUS\n"
//         "=========================================================\n"
//         "Mode        : %s\n"
//         "Safety      : %s\n"
//         "State       : %s\n"
//         "\n"
//         "Steering    : %.2f deg\n"
//         "Throttle    : %.2f\n"
//         "YOLO Objects: %d\n"
//         "Danger Zone : %s\n"
//         "Target      : %s\n"
//         "Confidence  : %.2f\n"
//         "=========================================================",
//         driveModeToString(vehicle_state_.command.mode).c_str(),
//         safetyDecisionToString(vehicle_state_.safety).c_str(),
//         controllerStateToString(vehicle_state_.state).c_str(),
//         vehicle_state_.command.steering_deg,
//         vehicle_state_.command.throttle,
//         vehicle_state_.detection.object_count,
//         vehicle_state_.detection.danger_zone ? "YES" : "NO",
//         vehicle_state_.detection.target_name.c_str(),
//         vehicle_state_.detection.confidence);
// }

void VehicleControllerNode::printVehicleStatus()
{
    std::stringstream yolo_stream;

    const auto &detections =
        vehicle_state_.detection.detections;

    if (detections.empty())
    {
        yolo_stream << "None";
    }
    else
    {
        for (size_t i = 0; i < detections.size(); ++i)
        {
            const auto &obj = detections[i];

            yolo_stream
                << i + 1 << ". "
                << objectTypeToString(obj.object_type)
                << " ("
                << obj.class_name
                << ")  "
                << std::fixed
                << std::setprecision(2)
                << obj.confidence
                << "  ROI:"
                << (obj.in_danger_zone ? "YES" : "NO");

            if (i + 1 < detections.size())
                yolo_stream << "\n";
        }
    }

    RCLCPP_INFO(
        this->get_logger(),
        "\n"
        "=========================================================\n"
        "            VEHICLE CONTROLLER STATUS\n"
        "=========================================================\n"
        "Mode        : %s\n"
        "Safety      : %s\n"
        "State       : %s\n"
        "\n"
        "Steering    : %.2f deg\n"
        "Throttle    : %.2f\n"
        "\n"
        // "YOLO Objects: %d\n"
        // "%s\n"
        "YOLO Objects: %d\n"
        "Danger Zone : %s\n"
        "Target      : %s\n"
        "Confidence  : %.2f\n"
        "\n"
        "%s\n"
        "=========================================================",
        driveModeToString(vehicle_state_.command.mode).c_str(),
        safetyDecisionToString(vehicle_state_.safety).c_str(),
        controllerStateToString(vehicle_state_.state).c_str(),
        vehicle_state_.command.steering_deg,
        vehicle_state_.command.throttle,
        static_cast<int>(detections.size()),
        vehicle_state_.detection.danger_zone ? "YES" : "NO",
        vehicle_state_.detection.target_name.c_str(),
        vehicle_state_.detection.confidence,
        yolo_stream.str().c_str());
}

std::string VehicleControllerNode::driveModeToString(
    DriveMode mode)
{
    switch (mode)
    {
        case DriveMode::Manual:
            return "Manual";

        case DriveMode::Assisted:
            return "Assisted";

        case DriveMode::Autonomous:
            return "Autonomous";

        case DriveMode::EmergencyStop:
            return "Emergency Stop";

        case DriveMode::MissionComplete:
            return "Mission Complete";

        default:
            return "Unknown";
    }
}

std::string VehicleControllerNode::safetyDecisionToString(
    SafetyDecision safety)
{
    switch (safety)
    {
        case SafetyDecision::Safe:
            return "SAFE";

        case SafetyDecision::Obstacle:
            return "OBSTACLE";

        case SafetyDecision::StopSign:
            return "STOP SIGN";

        default:
            return "UNKNOWN";
    }
}

// std::string VehicleControllerNode::controllerStateToString()
// {
//     return "Following Lane";
// }

std::string VehicleControllerNode::controllerStateToString(
    ControllerState state)
{
    switch (state)
    {
        case ControllerState::FollowingLane:
            return "Following Lane";

        case ControllerState::TemporaryStop:
            return "Temporary Stop";

        case ControllerState::WaitingAtStopSign:
            return "Waiting at STOP Sign";

        case ControllerState::MissionComplete:
            return "Mission Complete";

        default:
            return "Unknown";
    }
}

ObjectType VehicleControllerNode::stringToObjectType(
    const std::string &name)
{
    if (name == "person")
        return ObjectType::Person;

    if (name == "bicycle")
        return ObjectType::Bicycle;

    if (name == "car")
        return ObjectType::Car;

    if (name == "motorcycle")
        return ObjectType::Motorcycle;

    if (name == "bus")
        return ObjectType::Bus;

    if (name == "truck")
        return ObjectType::Truck;

    if (name == "chair")
        return ObjectType::Chair;

    if (name == "bottle")
        return ObjectType::Bottle;

    if (name == "stop sign")
        return ObjectType::StopSign;

    return ObjectType::Unknown;
}

std::string VehicleControllerNode::objectTypeToString(
    ObjectType type)
{
    switch (type)
    {
        case ObjectType::Person:      return "Person";
        case ObjectType::Bicycle:     return "Bicycle";
        case ObjectType::Car:         return "Car";
        case ObjectType::Motorcycle:  return "Motorcycle";
        case ObjectType::Bus:         return "Bus";
        case ObjectType::Truck:       return "Truck";
        case ObjectType::Bottle:      return "Bottle";
        case ObjectType::Chair:       return "Chair";
        case ObjectType::StopSign:    return "Stop Sign";
        default:                      return "Unknown";
    }
}

void VehicleControllerNode::printDetections()
{
    const auto &detections = vehicle_state_.detection.detections;

    RCLCPP_INFO(
        this->get_logger(),
        "\n================ YOLO DETECTIONS ================");

    if (detections.empty())
    {
        RCLCPP_INFO(this->get_logger(), "No objects detected.");
        return;
    }

    for (size_t i = 0; i < detections.size(); ++i)
    {
        const auto &obj = detections[i];

        RCLCPP_INFO(
            this->get_logger(),
            "[%zu]\n"
            "Class      : %s\n"
            "Type       : %s\n"
            "Confidence : %.2f\n"
            "Center     : (%.3f, %.3f)\n"
            "BBox       : (%.3f, %.3f) -> (%.3f, %.3f)\n",
            i + 1,
            obj.class_name.c_str(),
            objectTypeToString(obj.object_type).c_str(),
            obj.confidence,
            obj.center_x,
            obj.center_y,
            obj.xmin,
            obj.ymin,
            obj.xmax,
            obj.ymax);
    }

    RCLCPP_INFO(
        this->get_logger(),
        "===============================================\n");
}

bool VehicleControllerNode::isInsideDangerZone(
    float x,
    float y)
{
    return (x >= config_.roi_xmin &&
            x <= config_.roi_xmax &&
            y >= config_.roi_ymin &&
            y <= config_.roi_ymax);
}

void VehicleControllerNode::sendAICommand()
{
    AICommandPacket packet;

    float steering = std::clamp(
        vehicle_state_.command.steering_deg,
        -30.0f,
        30.0f);

    packet.steering =
        static_cast<int8_t>(
            std::round((steering / 30.0f) * 100.0f));

    float throttle = std::clamp(
        vehicle_state_.command.throttle,
        0.0f,
        1.0f);

    packet.throttle =
        static_cast<int8_t>(
            std::round(throttle * 100.0f));

    packet.requested_mode =
        static_cast<RequestedMode>(
            vehicle_state_.command.mode);

    packet.checksum =
        calculateChecksum(packet);

    serial_port_.writeBytes(
        &packet,
        sizeof(packet));
}