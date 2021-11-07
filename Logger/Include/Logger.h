//
// Created by Raghavasimhan Sankaranarayanan on 11/7/21.
//

#ifndef HATHAANI_LOGGER_H
#define HATHAANI_LOGGER_H

#include "pch.h"
#include "spdlog_pch.h"

class Logger {
public:
    enum Level
    {
        trace = SPDLOG_LEVEL_TRACE,
        debug = SPDLOG_LEVEL_DEBUG,
        info = SPDLOG_LEVEL_INFO,
        warn = SPDLOG_LEVEL_WARN,
        err = SPDLOG_LEVEL_ERROR,
        critical = SPDLOG_LEVEL_CRITICAL,
        off = SPDLOG_LEVEL_OFF,

        n_levels
    };

    static void init(Level level = trace);

    template<typename T>
    static void pprintArray(T* array, int size) {
        for (int i=0; i<size; i++) {
            std::cout << array[i] << " ";
        }
        std::cout << std::endl;
    }

};

#define LOG_TRACE(...)      SPDLOG_TRACE    (__VA_ARGS__)
#define LOG_DEBUG(...)      SPDLOG_DEBUG    (__VA_ARGS__)
#define LOG_INFO(...)       SPDLOG_INFO     (__VA_ARGS__)
#define LOG_WARN(...)       SPDLOG_WARN     (__VA_ARGS__)
#define LOG_ERROR(...)      SPDLOG_ERROR    (__VA_ARGS__)
#define LOG_CRITICAL(...)   SPDLOG_CRITICAL (__VA_ARGS__)

#endif //HATHAANI_LOGGER_H
