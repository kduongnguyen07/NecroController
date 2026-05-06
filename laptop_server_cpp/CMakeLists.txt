cmake_minimum_required(VERSION 3.20)
project(NecroServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
add_executable(NecroServer
        main.cpp
        utils.cpp
        utils.h
        file_manager.cpp
        file_manager.h
        commands.h
)

# NÃ ĐẠI BÁC: Ép tĩnh toàn bộ thư viện ruột của MinGW/GCC vào thẳng file EXE
target_link_options(NecroServer PRIVATE -static -static-libgcc -static-libstdc++)

# Native libs only - ws2_32(socket), gdi32+gdiplus(screenshot), Ole32(IStream), PowrProf(sleep)
target_link_libraries(NecroServer PRIVATE ws2_32 iphlpapi PowrProf gdi32 gdiplus Ole32)