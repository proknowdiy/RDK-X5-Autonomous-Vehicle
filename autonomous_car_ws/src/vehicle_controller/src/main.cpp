#include "vehicle_controller/vehicle_controller.hpp"

#include <memory>

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    rclcpp::spin(
        std::make_shared<VehicleControllerNode>());

    rclcpp::shutdown();

    return 0;
}