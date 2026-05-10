#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <cstdint>
#include <nav_msgs/msg/odometry.hpp>

struct __attribute__((packed)) DataToTransfer {
    uint8_t drone_id;
    float pos_x;
    float pos_y;
    float pos_z;
    float vel_x;
    float vel_y;
    float vel_z;
    uint32_t timestamp_ms;
};

inline void pack_msg(const nav_msgs::msg::Odometry& msg, DataToTransfer& data) {
    data.drone_id = 1;
    data.pos_x = msg.pose.pose.position.x;
    data.pos_y = msg.pose.pose.position.y;
    data.pos_z = msg.pose.pose.position.z;
    data.vel_x = msg.twist.twist.linear.x;
    data.vel_y = msg.twist.twist.linear.y;
    data.vel_z = msg.twist.twist.linear.z;
    data.timestamp_ms = (msg.header.stamp.sec * 1000) + (msg.header.stamp.nanosec / 1000000);
}

inline void unpack_msg(const DataToTransfer& data, nav_msgs::msg::Odometry& msg) {
    msg.pose.pose.position.x = data.pos_x;
    msg.pose.pose.position.y = data.pos_y;
    msg.pose.pose.position.z = data.pos_z;
    msg.twist.twist.linear.x = data.vel_x;
    msg.twist.twist.linear.y = data.vel_y;
    msg.twist.twist.linear.z = data.vel_z;
    msg.header.stamp.sec = data.timestamp_ms / 1000;
    msg.header.stamp.nanosec = (data.timestamp_ms % 1000) * 1000000;
    msg.header.frame_id = "wifi_bridge";
}

#endif