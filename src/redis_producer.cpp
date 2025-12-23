#include "redis_producer.h"
#include <iostream>

RedisProducer::RedisProducer(std::string h, int p) : host(h), port(p), context(nullptr) {
    connect();
}

RedisProducer::~RedisProducer() {
    if (context) {
        redisFree(context);
    }
}

void RedisProducer::connect() {
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    context = redisConnectWithTimeout(host.c_str(), port, timeout);

    if (context == NULL || context->err) {
        if (context) {
            std::cerr << "❌ Redis Connection Error: " << context->errstr << std::endl;
            redisFree(context);
            context = nullptr;
        } else {
            std::cerr << "❌ Redis Connection Error: Can't allocate redis context" << std::endl;
        }
    } else {
        std::cout << "✅ Connected to Redis at " << host << ":" << port << std::endl;
    }
}

void RedisProducer::publish(const std::string& channel, const std::string& message) {
    std::lock_guard<std::mutex> lock(redis_mutex);

    // Auto-reconnect if connection was lost
    if (context == nullptr) {
        connect();
        if (context == nullptr) return; // Still failed
    }

    // Send PUBLISH command
    // command: PUBLISH channel message
    redisReply *reply = (redisReply *)redisCommand(context, "PUBLISH %s %s", channel.c_str(), message.c_str());

    if (reply == NULL) {
        std::cerr << "⚠️ Redis Publish Failed. Reconnecting..." << std::endl;
        redisFree(context);
        context = nullptr;
        // Optional: Retry logic could go here
        return;
    }

    freeReplyObject(reply);
}