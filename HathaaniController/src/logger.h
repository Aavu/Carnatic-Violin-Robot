//
// Created by Raghavasimhan Sankaranarayanan on 11/26/21.
// 

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "stdarg.h"

class Logger {
public:
    enum LogLevel {
        None = 0,
        Error = 1,
        Warn = 2,
        Log = 3,
        Trace = 4
    };

    Logger(LogLevel logLevel = LogLevel::Log);

    ~Logger();

    static Logger* getDefaultLogger(LogLevel logLevel = LogLevel::Log);

    void error(const char* func, const char* msg...);

    void warn(const char* func, const char* msg...);

    void log(const char* func, const char* msg...);

    void trace(const char* func, const char* msg...);

private:
    int m_iLevel = LogLevel::Error;
    static Logger* pLogger;

    void vaPrint(const char* fmt, va_list args);
};

#define LOG_ERROR(...) Logger::getDefaultLogger(Logger::Log)->error(__func__, __VA_ARGS__)
#define LOG_WARN(...) Logger::getDefaultLogger(Logger::Log)->warn(__func__, __VA_ARGS__)
#define LOG_LOG(...) Logger::getDefaultLogger(Logger::Log)->log(__func__, __VA_ARGS__)
#define LOG_TRACE(...) Logger::getDefaultLogger(Logger::Log)->trace(__func__, __VA_ARGS__)

#endif // LOGGER_H