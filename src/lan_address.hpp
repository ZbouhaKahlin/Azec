#pragma once
#include <QtNetwork>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <vector>
#include <limits>

namespace lan {
struct Adapter {
    QHostAddress address;
    bool physical=false, gateway=false, wifi=false;
    quint32 metric=std::numeric_limits<quint32>::max();
};

// Prefer a real LAN with a gateway, then an isolated physical LAN. Internal
// virtual switches without a gateway are never useful pairing destinations.
inline QString selectAddress(const QList<Adapter>& adapters){
    QString best="127.0.0.1";int bestScore=-1;quint32 bestMetric=std::numeric_limits<quint32>::max();
    for(const auto& adapter:adapters){
        const auto ip=adapter.address;
        if(ip.protocol()!=QAbstractSocket::IPv4Protocol||ip.isLoopback()||ip.isLinkLocal()||ip.isNull()||(!adapter.physical&&!adapter.gateway))continue;
        const int score=(adapter.gateway?200:0)+(adapter.physical?100:0)+(adapter.wifi?10:0);
        if(score>bestScore||(score==bestScore&&adapter.metric<bestMetric)){
            best=ip.toString();bestScore=score;bestMetric=adapter.metric;
        }
    }
    return best;
}

inline QString currentAddress(){
    ULONG size=16384;std::vector<unsigned char> buffer(size);ULONG result=ERROR_BUFFER_OVERFLOW;
    for(int attempt=0;attempt<3&&result==ERROR_BUFFER_OVERFLOW;++attempt){
        buffer.resize(size);
        result=GetAdaptersAddresses(AF_INET,GAA_FLAG_INCLUDE_GATEWAYS|GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&size);
    }
    QList<Adapter> adapters;
    if(result==NO_ERROR)for(auto* adapter=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());adapter;adapter=adapter->Next){
        if(adapter->OperStatus!=IfOperStatusUp||(adapter->IfType!=IF_TYPE_ETHERNET_CSMACD&&adapter->IfType!=IF_TYPE_IEEE80211))continue;
        MIB_IF_ROW2 row{};row.InterfaceIndex=adapter->IfIndex;
        const bool physical=GetIfEntry2(&row)==NO_ERROR&&row.InterfaceAndOperStatusFlags.HardwareInterface;
        bool gateway=false;
        for(auto* entry=adapter->FirstGatewayAddress;entry;entry=entry->Next)
            if(entry->Address.lpSockaddr&&entry->Address.lpSockaddr->sa_family==AF_INET&&reinterpret_cast<sockaddr_in*>(entry->Address.lpSockaddr)->sin_addr.s_addr!=0)gateway=true;
        for(auto* entry=adapter->FirstUnicastAddress;entry;entry=entry->Next){
            if(!entry->Address.lpSockaddr||entry->Address.lpSockaddr->sa_family!=AF_INET||entry->DadState!=IpDadStatePreferred)continue;
            const auto address=QHostAddress(ntohl(reinterpret_cast<sockaddr_in*>(entry->Address.lpSockaddr)->sin_addr.s_addr));
            adapters.append({address,physical,gateway,adapter->IfType==IF_TYPE_IEEE80211,adapter->Ipv4Metric});
        }
    }
    return selectAddress(adapters);
}
}
