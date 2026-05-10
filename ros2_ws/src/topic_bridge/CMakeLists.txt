cmake_minimum_required(VERSION 3.8)
project(topic_bridge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(libtins REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(mrs_msgs REQUIRED)

include_directories(src/wifi_bridge)

add_executable(wifi_node
        src/wifi_bridge/wifi_node.cpp
)

ament_target_dependencies(wifi_node mrs_msgs nav_msgs rclcpp geometry_msgs)
target_link_libraries(wifi_node tins)

install(TARGETS
        wifi_node
        DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
        config
        DESTINATION share/${PROJECT_NAME}
)

ament_package()