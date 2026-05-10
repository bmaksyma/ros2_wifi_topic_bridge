#!/bin/bash

WS_DIR="$(pwd)"
MON_IFACE="mon0"
BASE_IFACE=""
PHY_DEV=""

cleanup() {
    echo -e "\n[dbg] Cleanup"
    sudo iw dev ${MON_IFACE} del 2>/dev/null || true

    if [ -n "${BASE_IFACE}" ]; then
        sudo nmcli device set ${BASE_IFACE} managed yes 2>/dev/null || true
        sudo nmcli device connect ${BASE_IFACE} 2>/dev/null || true
	      sudo ip link set ${BASE_IFACE} up 2>/dev/null || true
    fi
    sudo systemctl restart NetworkManager
    sudo systemctl start wpa_supplicant 2>/dev/null || true
    sudo systemctl start avahi-daemon 2>/dev/null || true
    echo "[dbs] Cleanup finishd."
    exit 0
}

trap cleanup EXIT INT TERM

echo "[dbg] Searching for wifi adapters."
for iface in /sys/class/net/*; do
    if [ -d "${iface}/wireless" ]; then
        name=$(basename "${iface}")
        [ -z "${BASE_IFACE}" ] && BASE_IFACE=${name}
    fi
done

if [ -z "${BASE_IFACE}" ]; then
    echo "[err] No wifi adapter found."
    exit 1
fi

PHY_DEV=$(cat /sys/class/net/${BASE_IFACE}/phy80211/name 2>/dev/null)
echo "[dbg] Found adapter: ${BASE_IFACE} (Phy: ${PHY_DEV})"

sudo killall wpa_supplicant 2>/dev/null || true
sudo killall avahi-daemon 2>/dev/null || true

echo "[dbg] Creating monitor interface ${MON_IFACE}."
sudo ip link set ${BASE_IFACE} down
sudo nmcli device disconnect ${BASE_IFACE} 2>/dev/null || true
sudo nmcli device set ${BASE_IFACE} managed no

sudo iw phy ${PHY_DEV} interface add ${MON_IFACE} type monitor 2>/dev/null || true
sudo ip link set ${MON_IFACE} up
sudo iw dev ${MON_IFACE} set channel 6

echo "[dbg] Running node."

SETUP_CMD="export RMW_IMPLEMENTATION=rmw_zenoh_cpp && source /opt/ros/jazzy/setup.bash && source ${WS_DIR}/install/setup.bash"

sudo bash -c "${SETUP_CMD} && ros2 run topic_bridge wifi_node"
