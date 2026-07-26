#ifndef LANE_DETECTOR_HPP_
#define LANE_DETECTOR_HPP_

#include <rclcpp/rclcpp.hpp>

#include <opencv2/opencv.hpp>

#include <hbm_img_msgs/msg/hbm_msg1080_p.hpp>

#include <sensor_msgs/msg/image.hpp>

#include <cv_bridge/cv_bridge.h>

#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>

#include <opencv2/opencv.hpp>

#include <geometry_msgs/msg/twist.hpp>

// #include <ai_msgs/msg/perception_targets.hpp>
// #include <unordered_set>

//==========================================================
// Debug View Selection
//==========================================================

enum class DebugView
{
    ORIGINAL,
    ROI,
    GRAYSCALE,
    BLUR,
    CANNY,
    HOUGH,
    FINAL
};

struct LineSegment
{
    cv::Point start;
    cv::Point end;

    //double slope;
    double length;

    cv::Point midpoint;

    double angle;
};

struct LaneModel
{
    bool valid = false;

    cv::Point start;
    cv::Point end;

    cv::Point2f direction;
    cv::Point2f point;
};

class LaneDetectorNode : public rclcpp::Node
{
public:
    LaneDetectorNode();

private:

    //----------------------------------------
    // Look-ahead
    //----------------------------------------

    float lookahead_ratio_ = 0.75f;

    //----------------------------------------
    // Steering Controller
    //----------------------------------------

    float steering_angle_deg_ = 0.0f;

    float steering_kp_ = 0.50f;

    float max_steering_angle_deg_ = 25.0f;

    // //------------------------------------------------------
    // // YOLO Safety
    // //------------------------------------------------------

    // bool emergency_stop_ = false;

    // bool permanent_stop_ = false;

    // bool object_inside_zone_ = false;

    // std::string detected_object_;

    // //------------------------------------------------------
    // // Danger Zone (Normalized)
    // //------------------------------------------------------

    // float zone_left_   = 0.20f;
    // float zone_right_  = 0.80f;

    // float zone_top_    = 0.40f;
    // float zone_bottom_ = 0.90f;

    // std::unordered_set<std::string> danger_classes_ =
    // {
    //     "person",
    //     "bicycle",
    //     "car",
    //     "motorcycle",
    //     "bus",
    //     "truck",
    //     "bottle"
    // };

    DebugView debug_view_ = DebugView::HOUGH; //HOUGH //CANNY

    // ROS callback
    void imageCallback(
        const hbm_img_msgs::msg::HbmMsg1080P::SharedPtr msg);

    // Image conversion
    bool convertImage(
        const hbm_img_msgs::msg::HbmMsg1080P::SharedPtr msg,
        cv::Mat &frame);

        cv::Mat original_frame_;

        cv::Mat working_frame_;

        cv::Mat debug_frame_;

    // Image processing
    void processFrame(cv::Mat &frame);

    rclcpp::Subscription<
        hbm_img_msgs::msg::HbmMsg1080P>::SharedPtr image_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_image_pub_;

    // rclcpp::Subscription<
    //     ai_msgs::msg::PerceptionTargets>::SharedPtr
    //     yolo_sub_;

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr
        vehicle_control_pub_;

    void applyROI();
    void convertToGray();
    void applyGaussianBlur();
    void detectEdges();

    void detectLines();
        std::vector<cv::Vec4i> detected_lines_;
        std::vector<LineSegment> filtered_lines_;
	std::vector<LineSegment> left_lines_;
	std::vector<LineSegment> right_lines_;
	LaneModel left_lane_;
	LaneModel right_lane_;
    // Smoothed Lane Models
    LaneModel smoothed_left_lane_;
    LaneModel smoothed_right_lane_;
    // Lane Center
    int lane_center_x_ = 0;
    int image_center_x_ = 0;
    int lateral_error_ = 0;
    // Perspective Transform
    cv::Mat perspective_matrix_;
    cv::Size perspective_size_;

    std::vector<cv::Point2f> perspective_src_;
    std::vector<cv::Point2f> perspective_dst_;
    cv::Mat bird_eye_frame_;

    void drawDetectedLines();
    void filterLines();
    void splitLeftRightLines();
    void fitLaneLines();
    void smoothLaneModels();
    void calculateLaneCenter();
    void calculateSteering();
    void publishVehicleControl();

    // void yoloCallback(
    //     const ai_msgs::msg::PerceptionTargets::SharedPtr msg);
};

#endif
