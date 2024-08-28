#pragma once

#include "Singleton.h"
#include <sstream>
#include <iomanip>

enum class LogLevel {
    LEVEL_DEBUG, 
    LEVEL_INFO, 
    LEVEL_WARN, 
    LEVEL_ERROR 
};

class LogManager : public SingletonBase<LogManager> {
private:
    // 생성자는 protected로 설정하여 외부에서 인스턴스를 생성할 수 없게 함
    friend class SingletonBase<LogManager>;

public:
    explicit LogManager() noexcept : log_level(LogLevel::LEVEL_DEBUG) {}
    ~LogManager() noexcept {}

    // 복사 생성자와 대입 연산자를 삭제하여 복사 방지
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

public:
    // 로그 레벨을 설정하는 메서드
    void set_log_level(LogLevel level) {
        log_level = level;
    }

    // 로그 메시지를 출력하는 메서드
    void log(LogLevel level, const std::string& message) {
        if (level < log_level) return; // 현재 로그 레벨보다 낮은 메시지는 무시

        std::string formatted_message = format_message(level, message);
        std::cout << formatted_message << std::endl;
    }

public:
    // 로그 메시지를 포맷팅하는 메서드
    std::string format_message(LogLevel level, const std::string& message) {
        std::stringstream ss;
        ss << get_current_time() << " ["
            << (level == LogLevel::LEVEL_DEBUG ? "DEBUG" :
                level == LogLevel::LEVEL_INFO ? "INFO" :
                level == LogLevel::LEVEL_WARN ? "WARN" : "ERROR")
            << "] " << message;
        return ss.str();
    }

    // 현재 시간을 문자열 형식으로 반환하는 메서드
    std::string get_current_time() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    LogLevel log_level; // 현재 로그 레벨
}


/*
    // 싱글톤 인스턴스를 가져와 로그 레벨을 설정
    LogManager::get_instance().set_log_level(LogLevel::LEVEL_DEBUG);

    // 로그 메시지 기록
    LogManager::get_instance().log(LogLevel::LEVEL_INFO, "Server started");
    LogManager::get_instance().log(LogLevel::LEVEL_DEBUG, "Debugging details");
    LogManager::get_instance().log(LogLevel::LEVEL_WARN, "Warning message");
    LogManager::get_instance().log(LogLevel::LEVEL_ERROR, "Error occurred");
*/