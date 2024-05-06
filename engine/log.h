#pragma once
#include <string>
enum class LogLevel {
    Fatal = 0,
    Error,
    Warning,
    Debug,
    Info,
    All = Info,
};
void log(LogLevel, std::string);
#define DEMO_LOG(LOG_LEVEL, MESSAGE) log(LogLevel::LOG_LEVEL, MESSAGE);