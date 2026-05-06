/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
#include "utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <tlhelp32.h>
#include <powrprof.h>
#include <fstream>
#include <vector>
#include <gdiplus.h>

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

static FILETIME prevSysIdle, prevSysKernel, prevSysUser;
static ULONG_PTR gdiplusToken = 0;

void Utils::init() {
    GetSystemTimes(&prevSysIdle, &prevSysKernel, &prevSysUser);
    init_gdiplus();
}

void Utils::init_gdiplus() {
    Gdiplus::GdiplusStartupInput res;
    Gdiplus::GdiplusStartup(&gdiplusToken, &res, NULL);
}

void Utils::shutdown_gdiplus() {
    if (gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }
}

/*
 * clear_port_9999() - Giết zombie process giữ port 9999
 * Gọi trước bind() để tránh "Address already in use"
 */
void Utils::clear_port_9999() {
    std::cout << "[INIT] Cleaning port 9999..." << std::endl;
    // Tìm PID đang giữ port 9999 qua netstat
    char buffer[256];
    std::string res = "netstat -ano | findstr :9999 | findstr LISTENING";
    FILE* pipe = _popen(res.c_str(), "r");
    if (pipe) {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            // Dòng dạng: TCP 0.0.0.0:9999 0.0.0.0:0 LISTENING 12345
            std::string ans(buffer);
            // PID là số cuối cùng trên dòng
            size_t pos = ans.find_last_not_of(" \t\r\n");
            if (pos != std::string::npos) {
                size_t start = ans.find_last_of(" \t", pos);
                if (start != std::string::npos) {
                    std::string pid = ans.substr(start + 1, pos - start);
                    // Không tự giết mình
                    DWORD myPid = GetCurrentProcessId();
                    try {
                        DWORD targetPid = std::stoul(pid);
                        if (targetPid != myPid && targetPid > 0) {
                            std::string killCmd = "taskkill /f /pid " + pid + " 2>NUL";
                            system(killCmd.c_str());
                            std::cout << "[INIT] Killed zombie PID: " << pid << std::endl;
                        }
                    } catch (...) {}
                }
            }
        }
        _pclose(pipe);
    }
    Sleep(300); // Chờ OS nhả port
    std::cout << "[INIT] Port 9999 clear!" << std::endl;
}

/* Helper: Lấy CLSID của encoder (jpeg, png, ...) cho GDI+ */
static int get_encoder_clsid(const WCHAR* format, CLSID* pClsid) {
    UINT ans = 0;
    UINT res = 0;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&ans, &res);
    if (res == 0) return -1;

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(res));
    if (pImageCodecInfo == NULL) return -1;

    Gdiplus::GetImageEncoders(ans, res, pImageCodecInfo);

    for (UINT j = 0; j < ans; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

double get_overall_cpu_usage() {
    FILETIME sysIdle, sysKernel, sysUser;
    if (GetSystemTimes(&sysIdle, &sysKernel, &sysUser) == 0) return 0.0;

    ULARGE_INTEGER uSysIdle, uSysKernel, uSysUser;
    ULARGE_INTEGER uPrevSysIdle, uPrevSysKernel, uPrevSysUser;

    uSysIdle.LowPart = sysIdle.dwLowDateTime; uSysIdle.HighPart = sysIdle.dwHighDateTime;
    uSysKernel.LowPart = sysKernel.dwLowDateTime; uSysKernel.HighPart = sysKernel.dwHighDateTime;
    uSysUser.LowPart = sysUser.dwLowDateTime; uSysUser.HighPart = sysUser.dwHighDateTime;

    uPrevSysIdle.LowPart = prevSysIdle.dwLowDateTime; uPrevSysIdle.HighPart = prevSysIdle.dwHighDateTime;
    uPrevSysKernel.LowPart = prevSysKernel.dwLowDateTime; uPrevSysKernel.HighPart = prevSysKernel.dwHighDateTime;
    uPrevSysUser.LowPart = prevSysUser.dwLowDateTime; uPrevSysUser.HighPart = prevSysUser.dwHighDateTime;

    ULONGLONG sysIdleDiff = uSysIdle.QuadPart - uPrevSysIdle.QuadPart;
    ULONGLONG sysKernelDiff = uSysKernel.QuadPart - uPrevSysKernel.QuadPart;
    ULONGLONG sysUserDiff = uSysUser.QuadPart - uPrevSysUser.QuadPart;

    ULONGLONG sysTotalDiff = sysKernelDiff + sysUserDiff;
    double ans = 0.0;
    if (sysTotalDiff > 0) {
        ans = (sysTotalDiff - sysIdleDiff) * 100.0 / sysTotalDiff;
    }

    prevSysIdle = sysIdle;
    prevSysKernel = sysKernel;
    prevSysUser = sysUser;

    return ans < 0.0 ? 0.0 : ans;
}

std::string Utils::get_system_stats() {
    std::stringstream res;
    res << "{";

    HKEY hKey;
    char cpuName[256] = "Unknown CPU";
    DWORD bufSize = sizeof(cpuName);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &bufSize);
        RegCloseKey(hKey);
    }

    std::string cpuNameStr(cpuName);
    size_t first = cpuNameStr.find_first_not_of(' ');
    if (first != std::string::npos) {
        size_t last = cpuNameStr.find_last_not_of(' ');
        cpuNameStr = cpuNameStr.substr(first, (last - first + 1));
    }
    res << "\"cpu_name\":\"" << cpuNameStr << "\",";

    double cpu_util = get_overall_cpu_usage();
    res << "\"cpu_util\":" << cpu_util << ",";

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    long long totalRam = memInfo.ullTotalPhys / (1024 * 1024 * 1024);
    long long freeRam = memInfo.ullAvailPhys / (1024 * 1024 * 1024);
    if (totalRam == 0 && memInfo.ullTotalPhys > 0) totalRam = 1;
    res << "\"ram_used\":" << (totalRam - freeRam) << ",";
    res << "\"ram_total\":" << totalRam << ",";

    int processes = 0;
    int threads = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnap, &pe32)) {
            do {
                processes++;
            } while (Process32Next(hSnap, &pe32));
        }

        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(hSnap, &te32)) {
            do {
                threads++;
            } while (Thread32Next(hSnap, &te32));
        }
        CloseHandle(hSnap);
    }
    res << "\"processes\":" << processes << ",";
    res << "\"threads\":" << threads << ",";

    ULONGLONG uptime_sec = GetTickCount64() / 1000;
    char uptimeStr[32];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02llu:%02llu:%02llu", uptime_sec / 3600, (uptime_sec % 3600) / 60, uptime_sec % 60);
    res << "\"uptime\":\"" << uptimeStr << "\",";

    int gpu_util = 0;
    int gpu_temp = 0;
    FILE* pipe = _popen("nvidia-smi --query-gpu=utilization.gpu,temperature.gpu --format=csv,noheader,nounits 2>NUL", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            sscanf(buffer, "%d, %d", &gpu_util, &gpu_temp);
        }
        _pclose(pipe);
    }
    res << "\"gpu_util\":" << gpu_util << ",";
    res << "\"gpu_temp\":" << gpu_temp << ",";

    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    res << "\"pc_time\":\"" << timeStr << "\",";

    SYSTEM_POWER_STATUS powerStatus;
    bool is_charging = true;
    int battery_pct = 100;
    if (GetSystemPowerStatus(&powerStatus)) {
        is_charging = (powerStatus.ACLineStatus == 1);
        if (powerStatus.BatteryLifePercent != 255) {
            battery_pct = powerStatus.BatteryLifePercent;
        }
    }
    res << "\"is_charging\":" << (is_charging ? "true" : "false") << ",";
    res << "\"battery_pct\":" << battery_pct;

    res << "}";
    return res.str();
}

void Utils::change_volume(int vk_code) {
    keybd_event(vk_code, MapVirtualKey(vk_code, 0), KEYEVENTF_EXTENDEDKEY, 0);
    keybd_event(vk_code, MapVirtualKey(vk_code, 0), KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

void Utils::set_volume(int targetVol) {
    int upPresses = targetVol / 2;
    for (int i = 0; i < 50; ++i) {
        change_volume(VK_VOLUME_DOWN);
    }
    for (int i = 0; i < upPresses; ++i) {
        change_volume(VK_VOLUME_UP);
    }
}

void Utils::execute_command(const std::string& command) {
    if (command == "LOCK") {
        LockWorkStation();
    } else if (command == "OFF") {
        system("shutdown /s /t 0");
    } else if (command == "RESTART") {
        system("shutdown /r /t 0");
    } else if (command == "SLEEP") {
        SetSuspendState(FALSE, FALSE, FALSE);
    } else if (command == "HIBERNATE") {
        system("shutdown /h");
    } else if (command == "ABORT") {
        system("shutdown /a");
    } else if (command.find("TIMER_") == 0) {
        std::string timeStr = command.substr(6);
        std::string res = "shutdown /s /t " + timeStr;
        system(res.c_str());
    } else if (command.find("SLEEPTIMER_") == 0) {
        std::string timeStr = command.substr(11);
        std::string res = "start /min cmd.exe /c \"timeout /t " + timeStr + " /nobreak & rundll32.exe powrprof.dll,SetSuspendState 0,1,0\"";
        system(res.c_str());
    } else if (command == "VOL_UP") {
        change_volume(VK_VOLUME_UP);
    } else if (command == "VOL_DOWN") {
        change_volume(VK_VOLUME_DOWN);
    } else if (command == "VOL_MUTE") {
        change_volume(VK_VOLUME_MUTE);
    } else if (command.find("VOL_SET_") == 0) {
        try {
            int targetVol = std::stoi(command.substr(8));
            set_volume(targetVol);
        } catch (...) {}
    }
}

std::string Utils::execute_shell(const std::string& cmd) {
    std::string ans;
    char buffer[128];
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR_PIPE";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        ans += buffer;
    }
    _pclose(pipe);
    if (ans.empty()) ans = "SUCCESS_NO_OUTPUT";
    return ans;
}

std::string Utils::kill_process(const std::string& process_name) {
    std::string ans = "PROCESS_NOT_FOUND";
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnap, &pe32)) {
            do {
                if (process_name == pe32.szExeFile) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProc != NULL) {
                        if (TerminateProcess(hProc, 0)) {
                            ans = "KILLED";
                        } else {
                            ans = "ACCESS_DENIED";
                        }
                        CloseHandle(hProc);
                        break;
                    }
                }
            } while (Process32Next(hSnap, &pe32));
        }
        CloseHandle(hSnap);
    }
    return ans;
}

/*
 * capture_screenshot() - Native GDI+ Screen Capture
 * Dùng BitBlt + GDI+ Bitmap::Save để chụp toàn bộ màn hình.
 * Tránh PowerShell hoàn toàn -> nhanh hơn, đáng tin cậy hơn.
 */
std::string Utils::capture_screenshot() {
    // Lấy kích thước toàn bộ virtual screen (hỗ trợ multi-monitor)
    int res = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int ans = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);

    if (res <= 0 || ans <= 0) return "";

    // Bước 1: BitBlt toàn bộ screen vào HBITMAP
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, res, ans);
    HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, res, ans, hScreenDC, screenX, screenY, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);

    // Bước 2: Tạo GDI+ Bitmap từ HBITMAP, save sang JPEG trong memory (IStream)
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);

    // Cleanup GDI objects sớm
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
    DeleteObject(hBitmap);

    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        if (bmp) delete bmp;
        return "";
    }

    // Bước 3: Encode JPEG vào IStream (in-memory, không ghi file)
    CLSID jpegClsid;
    if (get_encoder_clsid(L"image/jpeg", &jpegClsid) < 0) {
        delete bmp;
        return "";
    }

    // Chất lượng JPEG 85% (cân bằng dung lượng vs chất lượng)
    Gdiplus::EncoderParameters encoderParams;
    encoderParams.Count = 1;
    encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
    encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    encoderParams.Parameter[0].NumberOfValues = 1;
    ULONG quality = 85;
    encoderParams.Parameter[0].Value = &quality;

    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (!pStream) {
        delete bmp;
        return "";
    }

    Gdiplus::Status status = bmp->Save(pStream, &jpegClsid, &encoderParams);
    delete bmp;

    if (status != Gdiplus::Ok) {
        pStream->Release();
        return "";
    }

    // Bước 4: Đọc dữ liệu JPEG từ IStream -> vector<unsigned char>
    HGLOBAL hGlobal = NULL;
    GetHGlobalFromStream(pStream, &hGlobal);
    SIZE_T dataSize = GlobalSize(hGlobal);
    void* pData = GlobalLock(hGlobal);

    std::vector<unsigned char> buf((unsigned char*)pData, (unsigned char*)pData + dataSize);

    GlobalUnlock(hGlobal);
    pStream->Release();

    // Bước 5: Base64 encode và trả về
    return base64_encode(buf);
}

/*
 * capture_webcam() - PowerShell WinRT Webcam Capture
 * CRITICAL FIX: Tăng WaitForSingleObject lên 12s và Sleep lên 1500ms
 * để PowerShell kịp nhả file handle trước khi C++ đọc.
 */
std::string Utils::capture_webcam() {
    // Xoá file cũ trước khi chụp mới
    std::remove("shot.jpg");

    std::string psCommand = "powershell -WindowStyle Hidden -Command \"[Windows.Media.Capture.MediaCapture,Windows.Media.Capture,ContentType=WindowsRuntime] | Out-Null; [Windows.Storage.StorageFolder,Windows.Storage,ContentType=WindowsRuntime] | Out-Null; $folder = [Windows.Storage.StorageFolder]::GetFolderFromPathAsync((Get-Location).Path).AsTask().Result; $file = $folder.CreateFileAsync('shot.jpg', [Windows.Storage.CreationCollisionOption]::ReplaceExisting).AsTask().Result; $cap = New-Object Windows.Media.Capture.MediaCapture; $cap.InitializeAsync().AsTask().Wait(); Start-Sleep -Milliseconds 500; $bmp = [Windows.Media.MediaProperties.ImageEncodingProperties]::CreateJpeg(); $cap.CapturePhotoToStorageFileAsync($bmp, $file).AsTask().Wait(); $cap.Dispose();\"";

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)psCommand.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        // Chờ 12s - webcam cần thời gian init + capture + flush
        WaitForSingleObject(pi.hProcess, 12000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // CRITICAL: Chờ 2000ms cho webcam hardware ghi file xong hoàn toàn
    Sleep(2000);

    // Retry mở file 3 lần, mỗi lần cách 500ms
    // Kiểm tra file tồn tại VÀ size > 0 trước khi encode
    for (int retry = 0; retry < 3; retry++) {
        std::ifstream file("shot.jpg", std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size > 0) {
                std::vector<unsigned char> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                std::remove("shot.jpg");
                return base64_encode(buf);
            }
            file.close();
        }
        Sleep(500);
    }
    // File không tồn tại hoặc size == 0 sau 3 lần thử -> ERROR_CAM
    std::remove("shot.jpg");
    return "";
}

std::string Utils::search_file(const std::string& query) {
    std::string ans;
    char buffer[512];
    std::string cmd = "where /r \"%USERPROFILE%\" *" + query + "* 2>NUL";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR_PIPE\n";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        ans += buffer;
    }
    _pclose(pipe);
    if (ans.empty()) ans = "NOT_FOUND\n";
    return ans;
}

/*
 * get_clipboard_text() - Đọc clipboard Windows, trả về CLIP_DATA_ + nội dung UTF-8
 * Dùng CF_UNICODETEXT + WideCharToMultiByte để convert sang UTF-8
 */
std::string Utils::get_clipboard_text() {
    std::string ans = "CLIP_DATA_";
    if (!OpenClipboard(NULL)) return "CLIP_DATA_ERROR_OPEN";

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        CloseClipboard();
        return "CLIP_DATA_ERROR_EMPTY";
    }

    wchar_t* res = static_cast<wchar_t*>(GlobalLock(hData));
    if (res == NULL) {
        CloseClipboard();
        return "CLIP_DATA_ERROR_LOCK";
    }

    // Convert wchar_t (UTF-16) -> UTF-8
    int bufSize = WideCharToMultiByte(CP_UTF8, 0, res, -1, NULL, 0, NULL, NULL);
    if (bufSize > 0) {
        std::vector<char> utf8buf(bufSize);
        WideCharToMultiByte(CP_UTF8, 0, res, -1, utf8buf.data(), bufSize, NULL, NULL);
        ans += std::string(utf8buf.data());
    }

    GlobalUnlock(hData);
    CloseClipboard();
    return ans;
}

/*
 * start_live_stream() - Bắn FFmpeg gdigrab stream qua UDP tới điện thoại
 * Stream 1280x720 @ 60fps, preset ultrafast, tune zerolatency
 * Chạy ẩn hoàn toàn (SW_HIDE) để không chớp cửa sổ
 */
void Utils::start_live_stream(const std::string& res) {
    // Giết ffmpeg cũ nếu đang chạy
    system("taskkill /f /im ffmpeg.exe 2>NUL");
    Sleep(300);

    std::string cmd = "ffmpeg -f gdigrab -framerate 60 -i desktop -s 1280x720 -vcodec libx264 -preset ultrafast -tune zerolatency -b:v 2000k -f mpegts udp://" + res + ":1234";
    WinExec(cmd.c_str(), SW_HIDE);
}

/*
 * handle_voice_command() - Xử lý lệnh giọng nói từ điện thoại
 * Nhận string res đã lowercase, dispatch tới đúng chức năng
 */
std::string Utils::handle_voice_command(const std::string& res) {
    std::string ans = "VOICE_OK";

    if (res.find("restart") != std::string::npos || res.find("khởi động lại") != std::string::npos) {
        system("shutdown /r /t 0");
        ans = "VOICE_RESTART";
    } else if (res.find("copy") != std::string::npos || res.find("sao chép") != std::string::npos || res.find("clipboard") != std::string::npos) {
        ans = get_clipboard_text();
    } else if (res.find("tắt") != std::string::npos || res.find("shutdown") != std::string::npos || res.find("off") != std::string::npos) {
        system("shutdown /s /t 0");
        ans = "VOICE_SHUTDOWN";
    } else if (res.find("khóa") != std::string::npos || res.find("lock") != std::string::npos) {
        LockWorkStation();
        ans = "VOICE_LOCK";
    } else if (res.find("ngủ") != std::string::npos || res.find("sleep") != std::string::npos) {
        SetSuspendState(FALSE, FALSE, FALSE);
        ans = "VOICE_SLEEP";
    } else {
        ans = "VOICE_UNKNOWN";
    }

    return ans;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Utils::base64_encode(const std::vector<unsigned char>& data) {
    std::string res;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    const unsigned char* bytes_to_encode = data.data();
    size_t in_len = data.size();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++) res += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) res += base64_chars[char_array_4[j]];
        while ((i++ < 3)) res += '=';
    }
    return res;
}