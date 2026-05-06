/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
#ifndef NECROCONTROLLER_UTILS_H
#define NECROCONTROLLER_UTILS_H

#include <string>
#include <vector>
#include <windows.h>

namespace Utils {
    void init();
    void init_gdiplus();
    void shutdown_gdiplus();
    void clear_port_9999();
    std::string get_system_stats();
    void execute_command(const std::string& command);
    void set_volume(int targetVol);
    void change_volume(int vk_code);
    std::string base64_encode(const std::vector<unsigned char>& data); 
    std::string capture_webcam();
    std::string capture_screenshot();
    std::string execute_shell(const std::string& cmd);
    std::string kill_process(const std::string& process_name);
    std::string search_file(const std::string& query);
    std::string get_clipboard_text();
    void start_live_stream(const std::string& res);
    std::string handle_voice_command(const std::string& res);
}

#endif