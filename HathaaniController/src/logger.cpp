//
// Created by Raghavasimhan Sankaranarayanan on 11/26/21.
// 

#include "logger.h"

Logger* Logger::pLogger = nullptr;

Logger::Logger(Logger::LogLevel logLevel) : m_iLevel(logLevel) {
    Serial.begin(115200);
    while (!Serial) {}
}

Logger::~Logger() {
    delete pLogger;
}

Logger* Logger::getDefaultLogger(LogLevel logLevel) {
    pLogger = new Logger(logLevel);
    return pLogger;
}

void Logger::trace(const char* func, const char* msg...) {
    if (m_iLevel < LogLevel::Trace)
        return;
    Serial.print("TRACE: function - ");
    Serial.print(func);
    Serial.print(" | ");
    va_list args;
    va_start(args, msg);
    vaPrint(msg, args);
    va_end(args);
    Serial.println();
}

void Logger::log(const char* func, const char* msg...) {
    if (m_iLevel < LogLevel::Log)
        return;
    Serial.print("LOG: function - ");
    Serial.print(func);
    Serial.print(" | ");
    va_list args;
    va_start(args, msg);
    vaPrint(msg, args);
    va_end(args);
    Serial.println();
}

void Logger::warn(const char* func, const char* msg...) {
    if (m_iLevel < LogLevel::Warn)
        return;
    Serial.print("WARN: function - ");
    Serial.print(func);
    Serial.print(" | ");
    va_list args;
    va_start(args, msg);
    vaPrint(msg, args);
    va_end(args);
    Serial.println();
}

void Logger::error(const char* func, const char* msg...) {
    if (m_iLevel < LogLevel::Error)
        return;
    Serial.print("ERROR: function - ");
    Serial.print(func);
    Serial.print(" | ");
    va_list args;
    va_start(args, msg);
    vaPrint(msg, args);
    va_end(args);
    Serial.println();
}

void Logger::vaPrint(const char* fmt, va_list args) {
    while (*fmt != '\0') {
        if (*fmt == '%') {
            ++fmt;
            if (*fmt == 'i') {
                int val = va_arg(args, int);
                Serial.print(val);
            } else if (*fmt == 'c') {
                char val = (char) va_arg(args, int);
                Serial.print(val);
            } else if (*fmt == 'f') {
                float val = va_arg(args, float);  // Not working!!!
                // char buff[32];
                // sprintf(buff, "%.*f", 3, val);
                Serial.print(val);
            } else if (*fmt == 's') {
                const char* val = va_arg(args, const char*);
                Serial.print(val);
            } else if (*fmt == 'h') {
                int val = va_arg(args, int);
                Serial.print("0x");
                Serial.print(val, HEX);
            }
        } else {
            Serial.print(*fmt);
        }
        ++fmt;
    }

}
