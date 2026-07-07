#pragma once

#include <cstddef>
#include <functional>
#include <string>

class SseStreamParser
{
public:
    using TokenCallback = std::function<void(const std::string&)>;

    explicit SseStreamParser(TokenCallback onToken = {});

    void append(const char* data, size_t size);
    void finish();

    const std::string& answer() const;
    const std::string& error() const;
    bool done() const;

private:
    void processPending(bool flush);
    void handleDataLine(const std::string& data);

    std::string pending_;
    std::string answer_;
    std::string error_;
    bool done_ = false;
    TokenCallback onToken_;
};
