#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <cmath>

// ---------------------------------------------------------
// Helper: Welford's Algorithm for Online Variance/StdDev
// ---------------------------------------------------------
struct WelfordTracker {
    long count = 0;
    double mean = 0.0;
    double M2 = 0.0; // Sum of squares of differences

    void update(double value) {
        count++;
        double delta = value - mean;
        mean += delta / count;
        double delta2 = value - mean;
        M2 += delta * delta2;
    }

    double get_variance() const {
        return (count > 1) ? M2 / (count - 1) : 0.0;
    }

    double get_std_dev() const {
        return std::sqrt(get_variance());
    }
};

// ---------------------------------------------------------
// The Flow Entry (Matches your Python Model Features)
// ---------------------------------------------------------
struct FlowEntry {
    // Identifiers
    std::string src_ip;
    std::string dst_ip;
    int src_port;
    int dst_port;
    int protocol;

    // Timing
    long long first_seen; // Microseconds
    long long last_seen;
    long long last_fwd_packet_time; // For Fwd IAT

    // Counters
    long total_fwd_packets = 0;
    long total_bwd_packets = 0; // Needed for Bwd Packet features
    long total_len_fwd_packets = 0;
    
    // Feature Trackers (Welford)
    WelfordTracker pkt_len_stats;      // Packet Length (Mean, Std, Var)
    WelfordTracker fwd_pkt_len_stats;  // Fwd Packet Length
    WelfordTracker bwd_pkt_len_stats;  // Bwd Packet Length
    WelfordTracker fwd_iat_stats;      // Fwd Inter-Arrival Time
    WelfordTracker flow_iat_stats;     // Flow Inter-Arrival Time

    // Min/Max Trackers
    double max_packet_len = 0;
    double fwd_packet_len_max = 0;
    double bwd_packet_len_max = 0;
    double flow_iat_max = 0;

    // TCP Flags
    int syn_count = 0;
    int fin_count = 0;
    int rst_count = 0;
    int psh_count = 0;
    int ack_count = 0;
    
    // Window Sizes
    int init_win_bytes_fwd = 0;
    int init_win_bytes_bwd = 0;
    
    // Header Lengths
    int fwd_header_len = 0;

    // Constructor to init timestamps
    FlowEntry(long long timestamp) : first_seen(timestamp), last_seen(timestamp), last_fwd_packet_time(0) {}
    FlowEntry() : first_seen(0), last_seen(0), last_fwd_packet_time(0) {}
    
    // --- Method to convert to JSON string (for Python) ---
    std::string to_json() const;
};

// ---------------------------------------------------------
// The Flow Key (Hash Map Key)
// ---------------------------------------------------------
struct FlowKey {
    std::string src_ip;
    std::string dst_ip;
    int src_port;
    int dst_port;
    int protocol;

    bool operator==(const FlowKey &other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip &&
               src_port == other.src_port && dst_port == other.dst_port &&
               protocol == other.protocol;
    }
};

// Hash function for FlowKey
struct FlowKeyHash {
    std::size_t operator()(const FlowKey &k) const {
        return std::hash<std::string>()(k.src_ip) ^ std::hash<std::string>()(k.dst_ip) ^
               std::hash<int>()(k.src_port) ^ std::hash<int>()(k.dst_port);
    }
};

class FlowManager {
private:
    std::unordered_map<FlowKey, FlowEntry, FlowKeyHash> flow_map;
    std::mutex map_mutex;
    
    // Configuration
    long long FLOW_TIMEOUT_US = 120000000; // 2 minutes

public:
    // Main method called by packet sniffer
    void update_flow(const FlowKey& key, long long timestamp, 
                     int packet_len, int flags, bool is_forward);

    // Returns a list of expired flows to send to Python
    std::vector<std::string> check_timeouts(long long current_time);
};

#endif // FLOW_TABLE_H