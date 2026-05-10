#ifndef MONUTILS_H
#define MONUTILS_H

#include <iostream>
#include <string>
// #include <cstdlib>
// #include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

class MonUtils {
public:
    MonUtils() = default;

    static string find_or_create_mon_iface() {
        string mon_vif;
        if (auto monitors = find_existing_monitors(); monitors.empty()) {
            if (!MonUtils::create_mon_iface()) {
                cerr << "Failed to create monitor interface!" << endl;
                return "";
            } else {
                mon_vif = moniface_;
            }
        } else {
            mon_vif = monitors[0];
        }
        return mon_vif;
    }

    static bool delete_mon_iface(const string &name) {
        string cmd = "sudo iw dev " + name + " del > /dev/null 2>&1";
        (void)system(cmd.c_str());
        if (!base_iface_.empty()) {
            cmd = "sudo nmcli device set " + base_iface_ + " managed yes > /dev/null 2>&1";
            (void)system(cmd.c_str());
        }
        return true;
    }

    static bool create_mon_iface() {
        string phy_dev;
        string cmd;
        if (!detect_phy_dev(phy_dev, base_iface_)) {
            cerr << "No Wi-Fi adapter found!" << endl;
            return false;
        }

        cmd = "sudo ip link set " + base_iface_ + " up";
        (void)system(cmd.c_str());
        cmd = "sudo ip link set " + base_iface_ + " down";
        (void)system(cmd.c_str());

        cmd = "sudo nmcli device disconnect " + base_iface_ + " > /dev/null 2>&1";
        (void)system(cmd.c_str());
        cmd = "sudo nmcli device set " + base_iface_ + " managed no";
        (void)system(cmd.c_str());

        cmd = "sudo iw phy " + phy_dev + " interface add " + moniface_ + " type monitor";
        (void)system(cmd.c_str());
        cmd = "sudo ip link set " + moniface_ + " up";
        (void)system(cmd.c_str());
        cmd = "sudo iw dev " + moniface_ + " set channel 6";
        (void)system(cmd.c_str());
        cout << "Interface " << moniface_ << " created!" << endl;
        return true;
    }

    static vector<string> find_existing_monitors() {
        vector<string> monitors;
        string net_path = "/sys/class/net";
        if (!fs::exists(net_path)) return monitors;
        for (const auto& entry : fs::directory_iterator(net_path)) {
            string iface_name = entry.path().filename().string();
            string type_path = entry.path().string() + "/type";
            ifstream file(type_path);
            if (int hw_type = 0; file.is_open() && (file >> hw_type)) {
                if (hw_type == 801 || hw_type == 802 || hw_type == 803) {
                    monitors.push_back(iface_name);
                }
            }
        }
        return monitors;
    }
private:
    static bool detect_phy_dev(string &out_phy, string &out_base_iface) {
        string fallback_phy, fallback_iface;
        const string net_path = "/sys/class/net";
        if (!fs::exists(net_path)) return false;
        for (const auto& entry : fs::directory_iterator(net_path)) {
            if (string phy_path = entry.path().string() + "/phy80211/name"; fs::exists(phy_path)) {
                string current_phy;
                if (ifstream file(phy_path); file.is_open() && (file >> current_phy)) {
                    string current_iface = entry.path().filename().string();
                    if (string device_path = entry.path().string() + "/device"; fs::exists(device_path)) {
                        if (string real_path = fs::canonical(device_path).string(); real_path.find("usb") != string::npos) {
                            cout << "Found USB Dongle: " << current_phy << " (" << current_iface << ")" << endl;
                            out_phy = current_phy;
                            out_base_iface = current_iface;
                            return true;
                        }
                    }
                    if (fallback_phy.empty()) {
                        fallback_phy = current_phy;
                        fallback_iface = current_iface;
                    }
                }
            }
        }
        if (!fallback_phy.empty()) {
            cout << "No USB dongles found. Using internal adapter: " << fallback_phy << endl;
            out_phy = fallback_phy;
            out_base_iface = fallback_iface;
            return true;
        }
        return false;
    }

    static inline string moniface_ = "mon0";
    static inline string base_iface_;
};

#endif