/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
#include "file_manager.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdlib>

void handle_file_receive(const std::string& header, SOCKET client_socket) {
    std::stringstream ss(header);
    std::string item;
    std::vector<std::string> parts;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }

    if (parts.size() < 3) return;

    std::string filename = parts[1];
    long long filesize = std::stoll(parts[2]);

    const char* userProfile = getenv("USERPROFILE");
    if (!userProfile) return;
    
    std::string res = std::string(userProfile) + "\\Downloads\\" + filename;

    std::ofstream ans(res, std::ios::binary);
    if (!ans.is_open()) {
        std::cerr << "Loi mo file tai: " << res << std::endl;
        return;
    }

    char buffer[4096];
    long long total_received = 0;
    while (total_received < filesize) {
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) break;

        ans.write(buffer, bytes_read);
        total_received += bytes_read;
    }

    ans.close();

    if (total_received == filesize) {
        std::cout << "Da nhan xong file: " << res << std::endl;
        send(client_socket, "DONE\n", 5, 0);
    }
}