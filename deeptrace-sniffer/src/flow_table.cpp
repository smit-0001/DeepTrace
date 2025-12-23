#include "flow_table.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// ---------------------------------------------------------
// 1. Serialization: FlowEntry -> JSON String
// ---------------------------------------------------------
std::string FlowEntry::to_json() const {
    // We map C++ variables to the EXACT Feature Names expected by the Python Model
    // Refer to your "feature_importance.csv" list
    json j;
    
    // Top 20 Features (Must match Python training order/names exactly)
    j["Bwd Packet Length Std"]   = bwd_pkt_len_stats.get_std_dev();
    j["Packet Length Variance"]  = pkt_len_stats.get_variance();
    j["Packet Length Std"]       = pkt_len_stats.get_std_dev();
    j["Total Length of Fwd Packets"] = total_len_fwd_packets;
    j["Average Packet Size"]     = pkt_len_stats.mean;
    j["Max Packet Length"]       = max_packet_len;
    j["Bwd Packet Length Mean"]  = bwd_pkt_len_stats.mean;
    j["Packet Length Mean"]      = pkt_len_stats.mean;
    j["Subflow Fwd Bytes"]       = total_len_fwd_packets; // Usually same as total len
    j["Fwd Packet Length Max"]   = fwd_packet_len_max;
    j["Fwd Packet Length Mean"]  = fwd_pkt_len_stats.mean;
    j["Destination Port"]        = dst_port; 
    j["Fwd IAT Std"]             = fwd_iat_stats.get_std_dev();
    j["Flow IAT Max"]            = flow_iat_max;
    j["Fwd Packet Length Std"]   = fwd_pkt_len_stats.get_std_dev();
    j["Total Fwd Packets"]       = total_fwd_packets;
    j["act_data_pkt_fwd"]        = total_fwd_packets; // Simplified proxy
    j["Init_Win_bytes_backward"] = init_win_bytes_bwd;
    j["Bwd Packet Length Max"]   = bwd_packet_len_max;
    j["Fwd Header Length"]       = fwd_header_len;

    // Metadata for the Alert (Not used by model, but needed for UI)
    j["src_ip"] = src_ip;
    j["dst_ip"] = dst_ip;
    j["src_port"] = src_port;
    j["protocol"] = protocol;
    j["timestamp"] = last_seen;

    return j.dump();
}

// ---------------------------------------------------------
// 2. Flow Manager: Update Logic
// ---------------------------------------------------------
void FlowManager::update_flow(const FlowKey& key, long long timestamp, 
                              int packet_len, int flags, bool is_forward) {
    std::lock_guard<std::mutex> lock(map_mutex);

    // Create entry if it doesn't exist
    if (flow_map.find(key) == flow_map.end()) {
        FlowEntry new_entry(timestamp);
        new_entry.src_ip = key.src_ip;
        new_entry.dst_ip = key.dst_ip;
        new_entry.src_port = key.src_port;
        new_entry.dst_port = key.dst_port;
        new_entry.protocol = key.protocol;
        flow_map[key] = new_entry;
    }

    FlowEntry& f = flow_map[key];

    // -- Calculate Inter-Arrival Times (IAT) --
    if (f.last_seen > 0) {
        double iat = (double)(timestamp - f.last_seen);
        f.flow_iat_stats.update(iat);
        if (iat > f.flow_iat_max) f.flow_iat_max = iat;
        
        if (is_forward) {
             if (f.last_fwd_packet_time > 0) {
                 double fwd_iat = (double)(timestamp - f.last_fwd_packet_time);
                 f.fwd_iat_stats.update(fwd_iat);
             }
             f.last_fwd_packet_time = timestamp;
        }
    }

    // -- Update Timestamps --
    f.last_seen = timestamp;

    // -- Update Packet Stats (Welford) --
    f.pkt_len_stats.update((double)packet_len);
    if (packet_len > f.max_packet_len) f.max_packet_len = (double)packet_len;

    if (is_forward) {
        f.total_fwd_packets++;
        f.total_len_fwd_packets += packet_len;
        f.fwd_pkt_len_stats.update((double)packet_len);
        if (packet_len > f.fwd_packet_len_max) f.fwd_packet_len_max = (double)packet_len;
        // Simple heuristic for header len (20 bytes min IP + 20 TCP)
        f.fwd_header_len += (key.protocol == 6 ? 40 : 28); 
    } else {
        f.total_bwd_packets++;
        f.bwd_pkt_len_stats.update((double)packet_len);
        if (packet_len > f.bwd_packet_len_max) f.bwd_packet_len_max = (double)packet_len;
    }

    // -- Update Flags (Simplified) --
    // flags are usually passed as an integer (e.g. from tcphdr->th_flags)
    if (flags & 0x02) f.syn_count++; // SYN
    if (flags & 0x01) f.fin_count++; // FIN
    if (flags & 0x04) f.rst_count++; // RST
    if (flags & 0x10) f.ack_count++; // ACK
    if (flags & 0x08) f.psh_count++; // PUSH
}

// ---------------------------------------------------------
// 3. Flow Manager: Cleanup & Export
// ---------------------------------------------------------
std::vector<std::string> FlowManager::check_timeouts(long long current_time) {
    std::lock_guard<std::mutex> lock(map_mutex);
    std::vector<std::string> exported_flows;
    
    for (auto it = flow_map.begin(); it != flow_map.end(); ) {
        // If flow hasn't been seen for X seconds
        if ((current_time - it->second.last_seen) > FLOW_TIMEOUT_US) {
            // Serialize to JSON
            exported_flows.push_back(it->second.to_json());
            
            // Remove from map
            it = flow_map.erase(it);
        } else {
            ++it;
        }
    }
    return exported_flows;
}