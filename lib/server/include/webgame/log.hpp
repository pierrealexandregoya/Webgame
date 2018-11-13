#pragma once

#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

extern bool io_log;
extern bool data_log;
extern std::recursive_mutex log_mutex;
extern unsigned int title_max_size;

#define _MY_LOG(to_log)\
do {\
std::lock_guard<decltype(log_mutex)> lock(log_mutex);\
std::cout << to_log << std::endl;\
} while (0)

#define LOG(title, to_log) _MY_LOG("[" << std::setw(5) << std::this_thread::get_id() << "] [" << std::setw(title_max_size) << title << "] " << to_log)
