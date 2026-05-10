#ifndef WIFIBRIDGENODE_H
#define WIFIBRIDGENODE_H

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "Transmitter.h"
#include "Receiver.h"
#include "MonUtils.h"
#include "DataStructures.h"

#include "rclcpp/rclcpp.hpp"

using namespace std;

template <typename RosMsgT>
class WifiBridgeNode : public rclcpp::Node {
public:
    WifiBridgeNode() : Node("wifi_bridge") {
        this->declare_parameter<string>("interface", "mon0");
        this->declare_parameter<string>("bssid", "12:00:00:00:00:22");
        this->declare_parameter<string>("dst_mac", "ff:ff:ff:ff:ff:ff");
        this->declare_parameter<string>("topic_suffix", "control_manager/mpc_tracker/predicted_trajectory");

        iface_ = this->get_parameter("interface").as_string();
        string bssid_str = this->get_parameter("bssid").as_string();
        string dst_str = this->get_parameter("dst_mac").as_string();
        topic_suffix_ = this->get_parameter("topic_suffix").as_string();

        src_ = HWAddress<6>(NetworkInterface(iface_).hw_address());
        bssid_ = HWAddress<6>(bssid_str);
        dst_ = HWAddress<6>(dst_str);

        string uav_name = get_drone_name();
        cout << "[code dbg] UAV name: " << uav_name << endl;

        string my_topic = "/" + uav_name + "/" + topic_suffix_;
        sub_ = this->create_subscription<RosMsgT>(
            my_topic,
            rclcpp::SensorDataQoS(), // quos
            std::bind(&WifiBridgeNode::get_data_from_topic, this, std::placeholders::_1)
        );
    }

    void run() {
        running_ = true;
        tx_thread_ = thread(&WifiBridgeNode::tx_task, this);
        rx_thread_ = thread(&WifiBridgeNode::rx_task, this);
    }

    ~WifiBridgeNode() override = default;

private:
    string get_drone_name() {
        if (const char* env_uav_name = std::getenv("UAV_NAME"); !env_uav_name) {
            auto topics = this->get_topic_names_and_types();
            for (const auto& topic : topics) {
                // Unpredicted behavior if there are other drones topics before the node started.
                if (size_t start_pos = topic.first.find("uav"); start_pos != string::npos) {
                    if (const size_t end_pos = topic.first.find('/', start_pos); end_pos != string::npos) {
                        return topic.first.substr(start_pos, end_pos - start_pos);
                    } else {
                        return topic.first.substr(start_pos);
                    }
                }
            }
            return "uav999";
        } else {
            return env_uav_name;
        }
    }

    void get_data_from_topic(const shared_ptr<RosMsgT> msg) {
        {
            lock_guard<mutex> lock(tx_mutex_);
            tx_queue_.push(*msg);
        }
        tx_cv_.notify_one();
    }

    void tx_task() {
        Transmitter tx(iface_, src_, dst_, bssid_);
        rclcpp::Serialization<RosMsgT> serializer;
        while (true) {
            RosMsgT msg;
            {
                unique_lock<mutex> lock(tx_mutex_);
                tx_cv_.wait(lock, [this] { return !tx_queue_.empty() || !running_; });

                if (!running_ && tx_queue_.empty()) {
                    break;
                }

                msg = tx_queue_.front();
                tx_queue_.pop();
            }
            rclcpp::SerializedMessage serialized_msg;
            try {
                serializer.serialize_message(&msg, &serialized_msg);
                const uint8_t* raw_bytes = serialized_msg.get_rcl_serialized_message().buffer;
                const size_t length = serialized_msg.get_rcl_serialized_message().buffer_length;
                tx.send(raw_bytes, length);
            } catch (const std::exception& e) {
                RCLCPP_ERROR(this->get_logger(), "Error serializing message: %s", e.what());
            }
        }
    }

    void rx_task() {
        rclcpp::Serialization<RosMsgT> serializer;
        auto rx_callback = [this, &serializer](const vector<uint8_t> &payload) {
            if (payload.empty()) return;
            rclcpp::SerializedMessage serialized_msg;
            serialized_msg.reserve(payload.size());
            auto& msg_ref = serialized_msg.get_rcl_serialized_message();
            memcpy(msg_ref.buffer, payload.data(), payload.size());
            msg_ref.buffer_length = payload.size();
            RosMsgT ros_msg;
            try {
                serializer.deserialize_message(&serialized_msg, &ros_msg);
                string sender_name = ros_msg.uav_name;
                if (sender_name.empty()) {
                    RCLCPP_WARN(this->get_logger(), "Received message with empty uav_name, dropping.");
                }
                auto iter = traj_publishers_.find(sender_name);
                if (iter == traj_publishers_.end()) {
                    string topic_name = "/" + sender_name + "/" + topic_suffix_;
                    auto pub = this->create_publisher<RosMsgT>(topic_name, rclcpp::SensorDataQoS());
                    traj_publishers_[sender_name] = pub;
                    RCLCPP_INFO(this->get_logger(), "Created new publisher for topic: %s", topic_name.c_str());
                    pub->publish(ros_msg);
                } else {
                    iter->second->publish(ros_msg);
                }
            } catch (const std::exception& e) {
                RCLCPP_DEBUG(this->get_logger(), "Failed to deserialize: %s", e.what());
                return;
            }
        };

        Receiver rx(iface_, bssid_, src_, rx_callback);
        rx.start();
    }

    bool running_ = false;

    string iface_;
    string topic_suffix_;
    HWAddress<6> src_;
    HWAddress<6> bssid_;
    HWAddress<6> dst_;

    thread tx_thread_;
    thread rx_thread_;

    queue<RosMsgT> tx_queue_;
    mutex tx_mutex_;
    condition_variable tx_cv_;

    std::map<std::string, typename rclcpp::Publisher<RosMsgT>::SharedPtr> traj_publishers_;
    typename rclcpp::Subscription<RosMsgT>::SharedPtr sub_;
};

#endif