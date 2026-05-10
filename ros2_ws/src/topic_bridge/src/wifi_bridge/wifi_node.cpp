#include "WifiBridgeNode.h"
#include "DataStructures.h"
// #include <nav_msgs/msg/odometry.hpp>
#include <mrs_msgs/msg/future_trajectory.hpp>

using namespace std;

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);

    auto node = make_shared<WifiBridgeNode<mrs_msgs::msg::FutureTrajectory>>();
    node->run();

    rclcpp::spin(node);
    rclcpp::shutdown();

    return 0;
}