#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <string>
#include <vector>
#include <cstring>
#include <tins/tins.h>

using namespace std;
using namespace Tins;

class Transmitter {
public:
    Transmitter(const string &iface, const HWAddress<6> &src, const HWAddress<6> &dst, const HWAddress<6> &bssid)
                        : iface_(iface), src_(src), dst_(dst), bssid_(bssid), sender_(iface) {}

    template <typename T>
    void send(const T &data) {
        RadioTap rtap;
        rtap.tx_flags(0x0008);
        rtap.rate(2);
        rtap.channel(2437, 0x00a0);
        Dot11Action dot11(dst_, src_);
        dot11.addr3(bssid_);
        dot11.category(Dot11Action::VENDORSPECIFIC);
        const auto ptr = reinterpret_cast<const uint8_t*>(&data);
        const auto raw = RawPDU(ptr, sizeof(T));
        auto pkt = rtap / dot11 / raw;
        sender_.send(pkt, iface_);
    }

    ~Transmitter() = default; // TODO: destructor

private:
    string iface_;
    HWAddress<6> src_;
    HWAddress<6> dst_;
    HWAddress<6> bssid_;
    PacketSender sender_;
};

#endif
