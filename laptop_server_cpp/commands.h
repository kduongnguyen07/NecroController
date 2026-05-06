/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
#ifndef NECROCONTROLLER_COMMANDS_H
#define NECROCONTROLLER_COMMANDS_H

#include <string>

namespace Commands {
    const std::string GET_STATS = "GET_STATS";
    const std::string OFF = "OFF";
    const std::string RESTART = "RESTART";
    const std::string LOCK = "LOCK";
    const std::string SLEEP = "SLEEP";
    const std::string HIBERNATE = "HIBERNATE";
    const std::string ABORT = "ABORT";
    const std::string VOL_UP = "VOL_UP";
    const std::string VOL_DOWN = "VOL_DOWN";
    const std::string VOL_MUTE = "VOL_MUTE";
    const std::string VOL_SET = "VOL_SET_";
    const std::string TIMER = "TIMER_";
    const std::string SLEEPTIMER = "SLEEPTIMER_";
    const std::string WEBCAM = "WEBCAM";
    const std::string SCREENSHOT = "SCREENSHOT";
    const std::string FILE_SEND = "FILE_SEND_";
    const std::string CMD_SHELL = "CMD_SHELL_";
    const std::string CMD_KILL = "CMD_KILL_";
    const std::string CMD_SEARCH = "CMD_SEARCH_";
    const std::string FILE_DOWNLOAD = "FILE_DOWNLOAD_";
    const std::string VOICE_CMD = "VOICE_CMD_";
    const std::string GET_CLIPBOARD = "GET_CLIPBOARD";
    const std::string LIVE_STREAM = "LIVE_STREAM_";
}

#endif // NECROCONTROLLER_COMMANDS_H
