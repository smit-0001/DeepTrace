#include <iostream>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

#include "flow_table.h"
#include "redis_producer.h" // We will create this next

// Global Flow Manager
FlowManager flow_manager;
RedisProducer redis_producer("localhost", 6379); // Adjust host/port if needed

// ---------------------------------------------------------
// Packet Handler Callback
// ---------------------------------------------------------
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    struct ip *ip_header;
    struct tcphdr *tcp_header;
    struct udphdr *udp_header;
    
    // 1. Parse Ethernet Header (skip 14 bytes)
    ip_header = (struct ip *)(packet + 14);
    
    // Basic IP Validation
    if (ip_header->ip_v != 4) return; // Only IPv4 for now

    // 2. Extract Key Info
    FlowKey key;
    key.src_ip = inet_ntoa(ip_header->ip_src);
    key.dst_ip = inet_ntoa(ip_header->ip_dst);
    key.protocol = ip_header->ip_p;
    
    int src_port = 0;
    int dst_port = 0;
    int flags = 0;
    int header_size = 0;

    // 3. Protocol Specific Parsing
    if (key.protocol == IPPROTO_TCP) {
        tcp_header = (struct tcphdr *)(packet + 14 + (ip_header->ip_hl * 4));
        src_port = ntohs(tcp_header->th_sport);
        dst_port = ntohs(tcp_header->th_dport);
        flags = tcp_header->th_flags;
        header_size = (ip_header->ip_hl * 4) + (tcp_header->th_off * 4);
    } 
    else if (key.protocol == IPPROTO_UDP) {
        udp_header = (struct udphdr *)(packet + 14 + (ip_header->ip_hl * 4));
        src_port = ntohs(udp_header->uh_sport);
        dst_port = ntohs(udp_header->uh_dport);
        header_size = (ip_header->ip_hl * 4) + 8;
    } 
    else {
        return; // Skip ICMP/others for this demo
    }

    key.src_port = src_port;
    key.dst_port = dst_port;

    // 4. Calculate Features
    long long timestamp = pkthdr->ts.tv_sec * 1000000 + pkthdr->ts.tv_usec;
    int packet_len = pkthdr->len;
    
    // Determine direction (very simplified: assume if src port is high, it's forward)
    // In production, we track who started the connection.
    bool is_forward = true; 

    // 5. Update Flow
    flow_manager.update_flow(key, timestamp, packet_len, flags, is_forward);
}

// ---------------------------------------------------------
// Timeout Checker Thread
// ---------------------------------------------------------
void timeout_loop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Use current system time for timeouts
        auto now = std::chrono::system_clock::now();
        auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();

        std::vector<std::string> expired_flows = flow_manager.check_timeouts(now_us);
        
        for (const auto& json_str : expired_flows) {
            // Push to Redis!
            redis_producer.publish("network_traffic", json_str);
            std::cout << "🚀 Exported Flow: " << json_str << std::endl;
        }
    }
}

// ---------------------------------------------------------
// Main Entry
// ---------------------------------------------------------
int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    
    // Get device from env or default
    const char *dev = getenv("INTERFACE_NAME");
    if (!dev) dev = "eth0";

    std::cout << "🔌 DeepTrace Sniffer starting on: " << dev << std::endl;

    // Open interface
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        std::cerr << "❌ Could not open device " << dev << ": " << errbuf << std::endl;
        return 2;
    }

    // Start Timeout Thread
    std::thread t1(timeout_loop);
    t1.detach();

    // Start Sniffing Loop
    pcap_loop(handle, 0, packet_handler, NULL);

    pcap_close(handle);
    return 0;
}