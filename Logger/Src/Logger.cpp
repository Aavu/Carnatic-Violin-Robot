//
// Created by Raghavasimhan Sankaranarayanan on 11/7/21.
//

#include "Logger.h"

void Logger::init(Level level) {
    spdlog::set_pattern("%^[%r]\t[%s]\t[line %#]\t[---%l---]\t%v%$");
    spdlog::set_level((spdlog::level::level_enum)level);
    LOG_INFO("Logger initialized");
}
