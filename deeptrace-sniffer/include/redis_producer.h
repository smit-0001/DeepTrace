#ifndef REDIS_PRODUCER_H
#define REDIS_PRODUCER_H

#include <string>
#include <hiredis/hiredis.h>
#include <mutex>

class RedisProducer {
private:
    std::string host;
    int port;
    redisContext *context;
    std::mutex redis_mutex; // Thread safety is crucial

    void connect();

public:
    RedisProducer(std::string host, int port);
    ~RedisProducer();

    // Publishes a JSON string to a specific Redis channel
    void publish(const std::string& channel, const std::string& message);
};

#endif