#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "Transmitter.h"
#include "Receiver.h"
#include "DataStructures.h"
#include "MonUtils.h"

using namespace std;

int main(const int argc, char *argv[]) {
    const string iface = MonUtils::find_or_create_mon_iface();
    const HWAddress<6> src(NetworkInterface(iface).hw_address());
    const HWAddress<6> bssid("12:00:00:00:00:22");
    auto txf = [=]() {
        cout << "Mode: TX on " << iface << endl;
        const HWAddress<6> dst("ff:ff:ff:ff:ff:ff");
        Transmitter tx(iface, src, dst, bssid);
        DataToTransfer dd = {};
        dd.drone_id = 1;
        dd.pos_x = 0;
        dd.timestamp_ms = 0;
        while(true) {
            dd.timestamp_ms += 1;
            dd.pos_x += 0.5f;
            tx.send(dd);
            cout << "Sent packet C" << dd.timestamp_ms << endl;
            this_thread::sleep_for(chrono::milliseconds(100)); // 10Hz
        }
    };
    auto rxf = [=](){
        cout << "Mode: RX on " << iface << endl;
        auto print_data = [](const DataToTransfer &dd) {
            cout << "Received packet: drone_id=" << static_cast<int>(dd.drone_id)
                 << " pos_x=" << dd.pos_x
                 << " timestamp_ms=" << dd.timestamp_ms
                 << endl;
        };
        Receiver rx(iface, bssid, src, print_data);
        rx.start();
    };
    thread tx_thread(txf);
    thread rx_thread(rxf);
    tx_thread.join();
    rx_thread.join();

    return 0;
}
