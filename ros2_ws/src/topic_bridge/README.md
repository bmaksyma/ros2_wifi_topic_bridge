# ROS2 WiFi Topic Bridge

This ROS2 application provides a bridge for topics over raw WiFi frames using a monitor mode interface. It enables direct communication bypassing standard network layers, utilizing `libtins` to inject and sniff raw 802.11 frames (specifically `Dot11Action` frames).

## Prerequisites

1. **ROS2** (Tested on Jazzy, but should be compatible with modern ROS2 distros).
2. **libtins** (Custom branch):
   **Important:** The standard `libtins v4.5` installation will fail to compile this project because `Dot11Action` support is missing. 
   You must use the provided script to install the specific `libtins` branch that includes `Dot11Action` frame support.

### Installing the Custom Libtins Branch

Run the included install script from the package directory:
```bash
cd src/topic_bridge
./startup_tool.sh
```
*(Note: If the script is missing execute permissions, run `chmod +x startup_tool.sh` first).*

## Building

This is a standard ROS2 `ament_cmake` package. You can build it using `colcon`:

```bash
# Navigate to your workspace root
cd ~/ros2_ws 

# Build the package
colcon build --packages-select topic_bridge

# Source the workspace
source install/setup.bash
```

## Running the Application

To intercept and send raw WiFi frames, a wireless interface must be placed into **monitor mode**. We provide a convenient `start.sh` script that automates the environment setup, interface configuration, and node execution.

### Using `start.sh` (Recommended)

The script performs the following actions:
- Identifies your WiFi adapter.
- Temporarily disables `wpa_supplicant` and `NetworkManager` interference.
- Creates a virtual monitor interface (`mon0`) on channel 6.
- Sets up the ROS2 environment (using `rmw_zenoh_cpp` by default).
- Runs the `wifi_node` as root (required for raw packet injection/sniffing).
- Automatically cleans up and restores your normal WiFi connection when you exit (via `Ctrl+C`).

```bash
# Run the startup script from the workspace root or the package dir
sudo ./src/topic_bridge/start.sh
```

### Running Manually

If you prefer to configure your interface and run the node manually:

1. **Setup Monitor Mode:**
   ```bash
   sudo ip link set wlan0 down
   sudo iw phy phy0 interface add mon0 type monitor
   sudo ip link set mon0 up
   sudo iw dev mon0 set channel 6
   ```

2. **Run the Node:**
   *(Ensure you run as root/sudo since raw socket access is required)*
   ```bash
   source install/setup.bash
   sudo -E ros2 run topic_bridge wifi_node --ros-args --params-file install/topic_bridge/share/topic_bridge/config/params.yaml
   ```

## Project Structure

* `start.sh`: The main launcher script that sets up monitor mode and runs the node.
* `startup_tool.sh`: Script to compile and install the patched `libtins` dependency.
* `config/params.yaml`: Configuration file for the base parameters (interface, target topic strings, addresses).
* `src/wifi_bridge/`: Contains the core C++ implementations for the `Receiver`, `Transmitter`, and the `WifiBridgeNode`.

### Configuration file (params.yaml)
You can configure standard node behaviors in `config/params.yaml` directly:
- `interface`: The monitor interface name (default: `mon0`).
- `bssid` & `dst_mac`: The MAC addresses configured for injected WiFi frames.
- `topic_suffix`: The topic path that will be appended to the UAV namespace. For example, if configured to `control_manager/mpc_tracker/predicted_trajectory` with `uav_name` resolved to `uav1`, the node subscribes to `/uav1/control_manager/mpc_tracker/predicted_trajectory`.

## Changing the Transferred Message Type

Currently, the bridging node parses messages based on a fixed template argument. To change the overall bridged message *type*, modify the source code as follows:

1. **Change the Message Type (`src/wifi_bridge/wifi_node.cpp`)**:
   Adjust the included headers and template parameter.
   ```cpp
   // Remove mrs_msgs include and add your message type, e.g.:
   #include <nav_msgs/msg/odometry.hpp>

   // Update the template argument in main():
   auto node = make_shared<WifiBridgeNode<nav_msgs::msg::Odometry>>();
   ```

2. **Handle Publisher Discovery Identification**:
   In `WifiBridgeNode.h` inside `rx_task()`, the code expects the message object to have a `uav_name` field string to associate it with a sender (`string sender_name = ros_msg.uav_name;`). If your new custom message *does not* have a `uav_name` field, you will need to adapt this logic to uniquely identify senders or hardcode a single publisher behavior.

3. **Rebuild the Package**:
   After making these source changes, recompile:
   ```bash
   colcon build --packages-select topic_bridge
   source install/setup.bash
   ```
