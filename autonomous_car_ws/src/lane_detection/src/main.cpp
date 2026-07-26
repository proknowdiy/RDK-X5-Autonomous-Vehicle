#include "lane_detection/lane_detector.hpp"

#include <memory>

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<LaneDetectorNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}
