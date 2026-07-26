#include "lane_detection/lane_detector.hpp"
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <opencv2/imgcodecs.hpp>

//==========================================================
// Debug Display Configuration
//==========================================================

constexpr int DISPLAY_WIDTH = 320;
constexpr int DISPLAY_HEIGHT = 240;

constexpr int DISPLAY_JPEG_QUALITY = 60;

// Publish one debug frame every N frames
constexpr int DISPLAY_FRAME_SKIP = 2;
constexpr bool ENABLE_DEBUG_LOG = false;

//==========================================================
// Region Of Interest (ROI)
//==========================================================

constexpr int ROI_TOP = 0; //190
constexpr int ROI_BOTTOM_MARGIN = 0; //60

//==========================================================
// Gaussian Blur
//==========================================================

constexpr int GAUSSIAN_KERNEL_SIZE = 5;

//==========================================================
// Canny Edge Detection
//==========================================================

constexpr int CANNY_LOW_THRESHOLD = 70;
constexpr int CANNY_HIGH_THRESHOLD = 180;
constexpr int CANNY_APERTURE_SIZE = 3;

//==========================================================
// Hough Line Transform
//==========================================================

constexpr double HOUGH_RHO = 1.0;
constexpr double HOUGH_THETA = CV_PI / 180.0;

constexpr int HOUGH_THRESHOLD = 30;
constexpr int HOUGH_MIN_LINE_LENGTH = 40;
constexpr int HOUGH_MAX_LINE_GAP = 20;

//==========================================================
// Lane Classification
//==========================================================

// Ignore almost vertical lines
constexpr double MAX_ABS_SLOPE = 8.0;

// Image center tolerance
constexpr int IMAGE_CENTER_OFFSET = 20;



using std::placeholders::_1;

LaneDetectorNode::LaneDetectorNode()
    : Node("lane_detector_node")
{
    RCLCPP_INFO(this->get_logger(),
                "Lane Detector Node Started");

    auto qos = rclcpp::QoS(rclcpp::KeepLast(5));
    qos.best_effort();

    image_sub_ = this->create_subscription<
        hbm_img_msgs::msg::HbmMsg1080P>(
            "/hbmem_img",
            qos,
            std::bind(
                &LaneDetectorNode::imageCallback,
                this,
                _1));

    image_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
        "/lane_debug_image",   // /image
        10);
    vehicle_control_pub_ =
        this->create_publisher<geometry_msgs::msg::Twist>(
            "/vehicle_control",
            10);

    // yolo_sub_ =
    //     this->create_subscription<
    //         ai_msgs::msg::PerceptionTargets>(
    //             "/hobot_dnn_detection",
    //             10,
    //             std::bind(
    //                 &LaneDetectorNode::yoloCallback,
    //                 this,
    //                 std::placeholders::_1));        

    compressed_image_pub_ =
    this->create_publisher<sensor_msgs::msg::CompressedImage>(
        "/lane_debug_jpeg",   // /image
        10);

    // ==============================
    // Perspective Transform
    // ==============================

    perspective_src_ =
    {
        cv::Point2f(210.0f,190.0f),
        cv::Point2f(430.0f,190.0f),
        cv::Point2f(100.0f,470.0f),
        cv::Point2f(540.0f,470.0f)
    };

    perspective_dst_ =
    {
        cv::Point2f(140.0f,0.0f),
        cv::Point2f(500.0f,0.0f),
        cv::Point2f(140.0f,479.0f),
        cv::Point2f(500.0f,479.0f)
    };

    perspective_size_ = cv::Size(640,480);

    perspective_matrix_ =
        cv::getPerspectiveTransform(
            perspective_src_,
            perspective_dst_);

}

void LaneDetectorNode::imageCallback(
    const hbm_img_msgs::msg::HbmMsg1080P::SharedPtr msg)
{
    cv::Mat frame;

    if (!convertImage(msg, frame))
    {
        RCLCPP_ERROR(
            this->get_logger(),
            "Failed to convert NV12 image to BGR.");

        return;
    }

    processFrame(frame);
}

bool LaneDetectorNode::convertImage(
    const hbm_img_msgs::msg::HbmMsg1080P::SharedPtr msg,
    cv::Mat &frame)
{
    cv::Mat nv12(
        msg->height * 3 / 2,
        msg->width,
        CV_8UC1,
        const_cast<uint8_t*>(msg->data.data()));
 
    cv::cvtColor(
        nv12,
        frame,
        cv::COLOR_YUV2BGR_NV12);

    return !frame.empty();
}

void LaneDetectorNode::processFrame(cv::Mat &frame)
{
    //----------------------------------------------------------
    // Keep original camera frame untouched
    //----------------------------------------------------------

    original_frame_ = frame;

    //----------------------------------------------------------
    // Perspective Transform
    //----------------------------------------------------------

    cv::warpPerspective(
        original_frame_,
        bird_eye_frame_,
        perspective_matrix_,
        perspective_size_);

    //----------------------------------------------------------
    // Working copy for image processing
    //----------------------------------------------------------

    working_frame_ = bird_eye_frame_.clone();

    applyROI();
    convertToGray();
    applyGaussianBlur();
    detectEdges();
    detectLines();
    filterLines();
    splitLeftRightLines();
    fitLaneLines();
    smoothLaneModels();
    calculateLaneCenter();
    calculateSteering();
    publishVehicleControl();

    //----------------------------------------------------------
    // Default debug image
    //----------------------------------------------------------

    debug_frame_ = bird_eye_frame_.clone();   //debug_frame_ = original_frame_.clone();
    

    //----------------------------------------------------------
    // Debug information
    //----------------------------------------------------------

    static int counter = 0;

    if (++counter % 30 == 0)
    {
        RCLCPP_INFO(
            this->get_logger(),
            "Received BGR Frame : %dx%d",
            original_frame_.cols,
            original_frame_.rows);
    }

    //----------------------------------------------------------
    // Select debug view
    //----------------------------------------------------------

    //RCLCPP_INFO(
    //    this->get_logger(),
    //    "Debug Mode = %d",
    //    static_cast<int>(debug_view_));

    switch (debug_view_)
    {
        case DebugView::ORIGINAL:
        {
            debug_frame_ = bird_eye_frame_.clone();   //debug_frame_ = original_frame_.clone();  
            break;
        }

        case DebugView::ROI:
        {
            debug_frame_ = working_frame_.clone();
            break;
        }

        case DebugView::GRAYSCALE:
        {
            cv::cvtColor(
                working_frame_,
                debug_frame_,
                cv::COLOR_GRAY2BGR);

            break;
        }

        case DebugView::BLUR:
        {
            cv::cvtColor(
                working_frame_,
                debug_frame_,
                cv::COLOR_GRAY2BGR);

            break;
        }

        case DebugView::CANNY:
        {
            cv::cvtColor(
                working_frame_,
                debug_frame_,
                cv::COLOR_GRAY2BGR);

            break;
        }

        case DebugView::HOUGH:
        {
            drawDetectedLines();
            break;
        }

        default:
        {
            debug_frame_ = original_frame_.clone();
            break;
        }
    }

    //----------------------------------------------------------
    // Publish ROS Image
    //----------------------------------------------------------

    cv_bridge::CvImage cv_image;

    cv_image.header.stamp = this->now();
    cv_image.header.frame_id = "camera";

    cv_image.encoding = "bgr8";
    cv_image.image = debug_frame_;

    image_pub_->publish(*cv_image.toImageMsg());

    //----------------------------------------------------------
    // Publish Browser Image
    //----------------------------------------------------------

    static int display_counter = 0;

    if (++display_counter % DISPLAY_FRAME_SKIP == 0)
    {
        cv::Mat display_frame;

        cv::resize(
            debug_frame_,
            display_frame,
            cv::Size(
                DISPLAY_WIDTH,
                DISPLAY_HEIGHT),
            0,
            0,
            cv::INTER_LINEAR);

        std::vector<int> jpeg_params =
        {
            cv::IMWRITE_JPEG_QUALITY,
            DISPLAY_JPEG_QUALITY
        };

        std::vector<uchar> jpeg_buffer;

        cv::imencode(
            ".jpg",
            display_frame,
            jpeg_buffer,
            jpeg_params);

        sensor_msgs::msg::CompressedImage compressed_msg;

        compressed_msg.header.stamp = this->now();
        compressed_msg.header.frame_id = "camera";
        compressed_msg.format = "jpeg";

        compressed_msg.data.assign(
            jpeg_buffer.begin(),
            jpeg_buffer.end());

        compressed_image_pub_->publish(compressed_msg);
    }
}

void LaneDetectorNode::applyROI()
{
    cv::Rect roi(
        0,
        ROI_TOP,
        working_frame_.cols,
        working_frame_.rows - ROI_TOP - ROI_BOTTOM_MARGIN);

    working_frame_ = working_frame_(roi).clone();
}

void LaneDetectorNode::convertToGray()
{
    cv::cvtColor(
        working_frame_,
        working_frame_,
        cv::COLOR_BGR2GRAY);
}

void LaneDetectorNode::applyGaussianBlur()
{
    cv::GaussianBlur(
        working_frame_,
        working_frame_,
        cv::Size(
            GAUSSIAN_KERNEL_SIZE,
            GAUSSIAN_KERNEL_SIZE),
        0);
}

void LaneDetectorNode::detectEdges()
{
    cv::Canny(
        working_frame_,
        working_frame_,
        CANNY_LOW_THRESHOLD,
        CANNY_HIGH_THRESHOLD,
        CANNY_APERTURE_SIZE);
}

void LaneDetectorNode::detectLines()
{
    detected_lines_.clear();

    cv::HoughLinesP(
        working_frame_,
        detected_lines_,
        HOUGH_RHO,
        HOUGH_THETA,
        HOUGH_THRESHOLD,
        HOUGH_MIN_LINE_LENGTH,
        HOUGH_MAX_LINE_GAP);

    if (ENABLE_DEBUG_LOG)
    {
        RCLCPP_INFO(
            this->get_logger(),
            "Raw Hough Lines: %zu",
            detected_lines_.size()); 
    }       

    if (ENABLE_DEBUG_LOG)
    {
        RCLCPP_INFO(
            this->get_logger(),
            "Detected %zu lines",
            detected_lines_.size());
    }

    if (ENABLE_DEBUG_LOG)
    {
        RCLCPP_INFO(
            this->get_logger(),
            "Lane Error : %d px",
            lateral_error_);
    }            
}

void LaneDetectorNode::drawDetectedLines()
{
    debug_frame_ = bird_eye_frame_.clone();   //debug_frame_ = original_frame_.clone();

    for (const auto &line : detected_lines_)
    {
        cv::line(
            debug_frame_,
            cv::Point(line[0], line[1]),
            cv::Point(line[2], line[3]),
            cv::Scalar(0,255,255),   // Yellow
            2,
            cv::LINE_AA);
    }
    
/*
    for (const auto &segment : filtered_lines_)
    {
        cv::line(
            debug_frame_,
            segment.start,
            segment.end,
            cv::Scalar(0, 0, 255),
            2,
            cv::LINE_AA);
    }
*/

    // for (const auto &segment : left_lines_)
    // {
    //     cv::line(
    //         debug_frame_,
    //         segment.start,
    //         segment.end,
    //         cv::Scalar(255,0,0),
    //         2,
    //         cv::LINE_AA);
    // }

    // for (const auto &segment : right_lines_)
    // {
    //     cv::line(
    //         debug_frame_,
    //         segment.start,
    //         segment.end,
    //         cv::Scalar(0,255,0),
    //         2,
    //         cv::LINE_AA);
    // }


///////////////////////////////////////////////////////////////////////

    auto drawLane =
        [&](const LaneModel &lane,
            const cv::Scalar &color)
    {
        if (!lane.valid)
            return;

        //------------------------------------------------------
        // Direction returned by cv::fitLine()
        //------------------------------------------------------

        const float vx = lane.direction.x;
        const float vy = lane.direction.y;

        //------------------------------------------------------
        // One point on the fitted line
        //------------------------------------------------------

        const float x0 = lane.point.x;
        const float y0 = lane.point.y;

        //------------------------------------------------------
        // ROI limits
        //------------------------------------------------------

        const float topY =
            static_cast<float>(ROI_TOP);

        const float bottomY =
            static_cast<float>(
                bird_eye_frame_.rows - ROI_BOTTOM_MARGIN);

        //------------------------------------------------------
        // Avoid horizontal lines
        //------------------------------------------------------

        if (std::abs(vy) < 1e-6f)
            return;

        //------------------------------------------------------
        // Parameter t for top and bottom
        //------------------------------------------------------

        const float tTop =
            (topY - y0) / vy;

        const float tBottom =
            (bottomY - y0) / vy;

        //------------------------------------------------------
        // Compute endpoints
        //------------------------------------------------------

        const cv::Point ptTop(
            static_cast<int>(x0 + tTop * vx),
            static_cast<int>(topY));

        const cv::Point ptBottom(
            static_cast<int>(x0 + tBottom * vx),
            static_cast<int>(bottomY));

        //------------------------------------------------------
        // Draw lane
        //------------------------------------------------------

        cv::line(
            debug_frame_,
            ptTop,
            ptBottom,
            color,
            4,
            cv::LINE_AA);
    };

    drawLane(
        smoothed_left_lane_,
        cv::Scalar(255,0,0));

    drawLane(
        smoothed_right_lane_,
        cv::Scalar(0,255,0));

    // drawLane(
    //     left_lane_,
    //     cv::Scalar(255,0,0));

    // drawLane(
    //     right_lane_,
    //     cv::Scalar(0,255,0));

    


    // ===============================
    // Perspective Calibration Points
    // ===============================

    // cv::circle(debug_frame_,
    //            cv::Point(220,190),
    //            8,
    //            cv::Scalar(0,0,255),
    //            -1);

    // cv::putText(debug_frame_,
    //             "0",
    //             cv::Point(190,290),
    //             cv::FONT_HERSHEY_SIMPLEX,
    //             0.6,
    //             cv::Scalar(255,255,255),
    //             2);


    // cv::circle(debug_frame_,
    //            cv::Point(420,190),
    //            8,
    //            cv::Scalar(0,0,255),
    //            -1);

    // cv::putText(debug_frame_,
    //             "1",
    //             cv::Point(470,290),
    //             cv::FONT_HERSHEY_SIMPLEX,
    //             0.6,
    //             cv::Scalar(255,255,255),
    //             2);


    // cv::circle(debug_frame_,
    //            cv::Point(70,470),
    //            8,
    //            cv::Scalar(0,0,255),
    //            -1);

    // cv::putText(debug_frame_,
    //             "2",
    //             cv::Point(70,460),
    //             cv::FONT_HERSHEY_SIMPLEX,
    //             0.6,
    //             cv::Scalar(255,255,255),
    //             2);


    // cv::circle(debug_frame_,
    //            cv::Point(570,470),
    //            8,
    //            cv::Scalar(0,0,255),
    //            -1);

    // cv::putText(debug_frame_,
    //             "3",
    //             cv::Point(590,460),
    //             cv::FONT_HERSHEY_SIMPLEX,
    //             0.6,
    //             cv::Scalar(255,255,255),
    //             2);

    //----------------------------------------
    // Image center
    //----------------------------------------

    cv::circle(
        debug_frame_,
        cv::Point(image_center_x_,
                bird_eye_frame_.rows * 3 / 4),
        6,
        cv::Scalar(0,0,255),
        -1);

    //----------------------------------------
    // Lane center
    //----------------------------------------

    cv::circle(
        debug_frame_,
        cv::Point(lane_center_x_,
                bird_eye_frame_.rows * 3 / 4),
        6,
        cv::Scalar(255,255,0),
        -1);

    //----------------------------------------
    // Error line
    //----------------------------------------

    cv::line(
        debug_frame_,
        cv::Point(image_center_x_,
                bird_eye_frame_.rows * 3 / 4),
        cv::Point(lane_center_x_,
                bird_eye_frame_.rows * 3 / 4),
        cv::Scalar(0,255,255),
        2);

    cv::putText(
        debug_frame_,
        "Steering: " +
            std::to_string(
                static_cast<int>(steering_angle_deg_)) +
            " deg",
        cv::Point(20,40),
        cv::FONT_HERSHEY_SIMPLEX,
        0.8,
        cv::Scalar(0,255,255),
        2);

}


void LaneDetectorNode::filterLines()
{
    //----------------------------------------------------------
    // Clear previous frame
    //----------------------------------------------------------

    filtered_lines_.clear();
    left_lines_.clear();
    right_lines_.clear();

    //----------------------------------------------------------
    // Image centre
    //----------------------------------------------------------

    const int image_center = working_frame_.cols / 2;

    //----------------------------------------------------------
    // Parameters
    //----------------------------------------------------------

    constexpr double MIN_LINE_LENGTH = 40.0;

    // Allow ±15 degrees from vertical
    constexpr double MAX_VERTICAL_DEVIATION = 15.0;

    //----------------------------------------------------------
    // Process every Hough line
    //----------------------------------------------------------

    for (const auto &line : detected_lines_)
    {
        LineSegment segment;

        segment.start = cv::Point(line[0], line[1]);
        segment.end   = cv::Point(line[2], line[3]);

        //------------------------------------------------------
        // Length
        //------------------------------------------------------

        const double dx = segment.end.x - segment.start.x;
        const double dy = segment.end.y - segment.start.y;

        segment.length = std::sqrt(dx * dx + dy * dy);

        if (segment.length < MIN_LINE_LENGTH)
            continue;

        //------------------------------------------------------
        // Angle
        //------------------------------------------------------

        segment.angle =
            std::atan2(dy, dx) * 180.0 / CV_PI;

        //------------------------------------------------------
        // Convert angle to deviation from vertical
        //------------------------------------------------------

        double deviation =
            std::abs(std::abs(segment.angle) - 90.0);

        if (deviation > MAX_VERTICAL_DEVIATION)
            continue;

        //------------------------------------------------------
        // Midpoint
        //------------------------------------------------------

        segment.midpoint =
            cv::Point(
                (segment.start.x + segment.end.x) / 2,
                (segment.start.y + segment.end.y) / 2);

        //------------------------------------------------------
        // Store
        //------------------------------------------------------

        filtered_lines_.push_back(segment);

        if (segment.midpoint.x < image_center)
        {
            left_lines_.push_back(segment);
        }
        else
        {
            right_lines_.push_back(segment);
        }
    }

    //----------------------------------------------------------
    // Debug
    //----------------------------------------------------------

    if (ENABLE_DEBUG_LOG)
    {
        RCLCPP_INFO(
            this->get_logger(),
            "Filtered: %zu  Left: %zu  Right: %zu",
            filtered_lines_.size(),
            left_lines_.size(),
            right_lines_.size());
    }        
}

void LaneDetectorNode::splitLeftRightLines()
{
    left_lines_.clear();
    right_lines_.clear();

    const int image_center = original_frame_.cols / 2;

    for (const auto &segment : filtered_lines_)
    {
        //--------------------------------------------------
        // Reject almost vertical lines
        //--------------------------------------------------

        // if (std::abs(segment.slope) > MAX_ABS_SLOPE)
        //     continue;

        //--------------------------------------------------
        // Reject center lane markings
        //--------------------------------------------------

        if (std::abs(segment.midpoint.x - image_center) < IMAGE_CENTER_OFFSET)
            continue;

        //--------------------------------------------------
        // Left side
        //--------------------------------------------------

        if (segment.midpoint.x < image_center)
        {
            left_lines_.push_back(segment);
        }
        //--------------------------------------------------
        // Right side
        //--------------------------------------------------
        else
        {
            right_lines_.push_back(segment);
        }
    }
    
    if (ENABLE_DEBUG_LOG)
    {
        RCLCPP_INFO(
        this->get_logger(),
        "Left: %zu   Right: %zu",
        left_lines_.size(),
        right_lines_.size());
    }    
}

void LaneDetectorNode::fitLaneLines()
{
    //--------------------------------------------------
    // Reset lane models
    //--------------------------------------------------

    left_lane_.valid = false;
    right_lane_.valid = false;

    //--------------------------------------------------
    // Lambda to fit one lane
    //--------------------------------------------------

    auto fitLane =
        [&](const std::vector<LineSegment> &segments,
            LaneModel &lane)
    {
        if (segments.size() < 2)
            return;

        std::vector<cv::Point> points;

        for (const auto &segment : segments)
        {
            points.push_back(segment.start);
            points.push_back(segment.end);
        }

        cv::Vec4f line;

        cv::fitLine(
            points,
            line,
            cv::DIST_L2,
            0,
            0.01,
            0.01);

        lane.direction =
            cv::Point2f(
                line[0],
                line[1]);

        lane.point =
            cv::Point2f(
                line[2],
                line[3]);

        lane.valid = true;
    };

    fitLane(left_lines_, left_lane_);

    fitLane(right_lines_, right_lane_);

    if (ENABLE_DEBUG_LOG)
    {
        if (left_lane_.valid)
        {
            RCLCPP_INFO(
                this->get_logger(),
                "Left lane fitted");
        }

        if (right_lane_.valid)
        {
            RCLCPP_INFO(
                this->get_logger(),
                "Right lane fitted");
        }
    }    
}

void LaneDetectorNode::smoothLaneModels()
{
    //----------------------------------------------------------
    // EMA coefficient
    //----------------------------------------------------------

    constexpr float ALPHA = 0.20f;

    //----------------------------------------------------------
    // Helper lambda
    //----------------------------------------------------------

    auto smoothLane =
        [&](const LaneModel &current,
            LaneModel &smoothed)
    {
        if (!current.valid)
            return;

        //------------------------------------------------------
        // First frame
        //------------------------------------------------------

        if (!smoothed.valid)
        {
            smoothed = current;
            return;
        }

        //------------------------------------------------------
        // Smooth line point
        //------------------------------------------------------

        smoothed.point.x =
            ALPHA * current.point.x +
            (1.0f - ALPHA) * smoothed.point.x;

        smoothed.point.y =
            ALPHA * current.point.y +
            (1.0f - ALPHA) * smoothed.point.y;

        //------------------------------------------------------
        // Smooth direction
        //------------------------------------------------------

        smoothed.direction.x =
            ALPHA * current.direction.x +
            (1.0f - ALPHA) * smoothed.direction.x;

        smoothed.direction.y =
            ALPHA * current.direction.y +
            (1.0f - ALPHA) * smoothed.direction.y;

        //------------------------------------------------------
        // Normalize direction vector
        //------------------------------------------------------

        float norm =
            std::sqrt(
                smoothed.direction.x * smoothed.direction.x +
                smoothed.direction.y * smoothed.direction.y);

        if (norm > 0.0f)
        {
            smoothed.direction.x /= norm;
            smoothed.direction.y /= norm;
        }

        smoothed.valid = true;
    };

    smoothLane(left_lane_, smoothed_left_lane_);
    smoothLane(right_lane_, smoothed_right_lane_);
}

void LaneDetectorNode::calculateLaneCenter()
{
    //----------------------------------------
    // Both lanes required
    //----------------------------------------

    if (!smoothed_left_lane_.valid ||
        !smoothed_right_lane_.valid)
    {
        return;
    }

    //----------------------------------------
    // Look-ahead row
    //----------------------------------------

    const float y =
    bird_eye_frame_.rows * lookahead_ratio_;
    // const float y =
    //     bird_eye_frame_.rows * 0.75f;

    //----------------------------------------
    // Helper
    //----------------------------------------

    auto lineX =
        [&](const LaneModel &lane)
    {
        float t =
            (y - lane.point.y) /
            lane.direction.y;

        return lane.point.x +
               t * lane.direction.x;
    };

    //----------------------------------------
    // Lane positions
    //----------------------------------------

    float leftX = lineX(smoothed_left_lane_);
    float rightX = lineX(smoothed_right_lane_);

    //----------------------------------------
    // Centers
    //----------------------------------------

    lane_center_x_ =
        static_cast<int>((leftX + rightX) * 0.5f);

    image_center_x_ =
        bird_eye_frame_.cols / 2;

    lateral_error_ =
        lane_center_x_ - image_center_x_;
}

void LaneDetectorNode::calculateSteering()
{
    //----------------------------------------
    // Proportional Controller
    //----------------------------------------

    // steering_angle_deg_ =
    //     steering_kp_ *
    //     static_cast<float>(lateral_error_);

    float normalized_error =
        static_cast<float>(lateral_error_) /
        (bird_eye_frame_.cols * 0.5f);

    constexpr float DEAD_BAND = 0.02f;

    if (std::abs(normalized_error) < DEAD_BAND)
    {
        normalized_error = 0.0f;
    }

    steering_angle_deg_ =
        normalized_error *
        max_steering_angle_deg_;



    //----------------------------------------
    // Clamp
    //----------------------------------------

    steering_angle_deg_ =
        std::clamp(
            steering_angle_deg_,
            -max_steering_angle_deg_,
            max_steering_angle_deg_);
        
}

void LaneDetectorNode::publishVehicleControl()
{
    geometry_msgs::msg::Twist msg;

    //----------------------------------------
    // Steering
    //----------------------------------------

    msg.angular.z = steering_angle_deg_;

    //----------------------------------------
    // Constant speed
    //----------------------------------------

    msg.linear.x = 0.20;

    vehicle_control_pub_->publish(msg);
}