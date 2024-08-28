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
    // �����ڴ� protected�� �����Ͽ� �ܺο��� �ν��Ͻ��� ������ �� ���� ��
    friend class SingletonBase<LogManager>;

public:
    explicit LogManager() noexcept : log_level(LogLevel::LEVEL_DEBUG) {}
    ~LogManager() noexcept {}

    // ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���� ����
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

public:
    // �α� ������ �����ϴ� �޼���
    void set_log_level(LogLevel level) {
        log_level = level;
    }

    // �α� �޽����� ����ϴ� �޼���
    void log(LogLevel level, const std::string& message) {
        if (level < log_level) return; // ���� �α� �������� ���� �޽����� ����

        std::string formatted_message = format_message(level, message);
        std::cout << formatted_message << std::endl;
    }

public:
    // �α� �޽����� �������ϴ� �޼���
    std::string format_message(LogLevel level, const std::string& message) {
        std::stringstream ss;
        ss << get_current_time() << " ["
            << (level == LogLevel::LEVEL_DEBUG ? "DEBUG" :
                level == LogLevel::LEVEL_INFO ? "INFO" :
                level == LogLevel::LEVEL_WARN ? "WARN" : "ERROR")
            << "] " << message;
        return ss.str();
    }

    // ���� �ð��� ���ڿ� �������� ��ȯ�ϴ� �޼���
    std::string get_current_time() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    LogLevel log_level; // ���� �α� ����
}


/*
    // �̱��� �ν��Ͻ��� ������ �α� ������ ����
    LogManager::get_instance().set_log_level(LogLevel::LEVEL_DEBUG);

    // �α� �޽��� ���
    LogManager::get_instance().log(LogLevel::LEVEL_INFO, "Server started");
    LogManager::get_instance().log(LogLevel::LEVEL_DEBUG, "Debugging details");
    LogManager::get_instance().log(LogLevel::LEVEL_WARN, "Warning message");
    LogManager::get_instance().log(LogLevel::LEVEL_ERROR, "Error occurred");
*/