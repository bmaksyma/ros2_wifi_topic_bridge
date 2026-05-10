#ifndef RECEIVER_H
#define RECEIVER_H

#include <string>
#include <functional>
#include <iostream>
#include <cstring>
#include <utility>
#include <tins/tins.h>
#include "DataStructures.h"

using namespace std;
using namespace Tins;

class Receiver {
public:
    Receiver(string iface, const HWAddress<6> &bssid, const HWAddress<6> &src, const function<void(DataToTransfer&)>& data_func)
                        : iface_(std::move(iface)), src_(src), bssid_(bssid), process_raw(data_func) {}

    void start() {
        SnifferConfiguration config;
        config.set_rfmon(true);
        Sniffer sniffer(iface_, config);
        sniffer.sniff_loop([this](PDU &pdu) {
            return handle(pdu);
        });
    }

private:
    bool handle(PDU &pdu) {
        auto *action = pdu.find_pdu<Dot11Action>();
        if (!action || action->category() != Dot11Action::VENDORSPECIFIC) {
            return true;
        }
        if (action->addr2() == src_) {
            return true;
        }
        if (action->addr3() != bssid_) {
            return true;
        }
        auto *raw = pdu.find_pdu<RawPDU>();
        if (!raw || raw->payload().size() < sizeof(DataToTransfer)) {
            return true;
        }
        DataToTransfer dd{};
        memcpy(&dd, raw->payload().data(), sizeof(DataToTransfer));
        process_raw(dd);
        return true;
    }

    string iface_;
    HWAddress<6> src_;
    HWAddress<6> bssid_;
    function<void(DataToTransfer&)> process_raw;
};

#endif