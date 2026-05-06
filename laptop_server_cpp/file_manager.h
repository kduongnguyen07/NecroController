#ifndef NECROCONTROLLER_FILE_MANAGER_H
#define NECROCONTROLLER_FILE_MANAGER_H

#include <winsock2.h>
#include <string>

void handle_file_receive(const std::string& header, SOCKET client_socket);

#endif // NECROCONTROLLER_FILE_MANAGER_H
