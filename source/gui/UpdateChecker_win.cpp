#ifdef _WIN32

#include "UpdateChecker.hpp"
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <thread>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

namespace gui {

std::mutex UpdateChecker::mutex_;
UpdateChecker::UpdateInfo UpdateChecker::info_;
std::atomic<bool> UpdateChecker::checked_{false};

std::string UpdateChecker::httpGet(const char* url) {
    std::string sUrl(url);
    std::wstring wUrl(sUrl.begin(), sUrl.end());

    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{}, path[1024]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &uc))
        return "";

    HINTERNET session = WinHttpOpen(L"Saturator/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return "";

    DWORD timeout = 2000;
    WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);

    HINTERNET connect = WinHttpConnect(session, host, uc.nPort, 0);
    if (!connect) { WinHttpCloseHandle(session); return ""; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::string result;
    DWORD bytesRead = 0;
    char buf[4096];
    while (WinHttpReadData(request, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
        result.append(buf, bytesRead);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return result;
}

void UpdateChecker::openInBrowser(const char* url) {
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

// простой парсер JSON — ищем "key": "value"
static std::string jsonValue(const std::string& json, const char* key) {
    std::string needle = std::string("\"") + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";

    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";

    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}

bool UpdateChecker::isNewer(const char* latest, const char* current) {
    int lMaj = 0, lMin = 0, lPat = 0;
    int cMaj = 0, cMin = 0, cPat = 0;
    std::sscanf(latest, "%d.%d.%d", &lMaj, &lMin, &lPat);
    std::sscanf(current, "%d.%d.%d", &cMaj, &cMin, &cPat);

    if (lMaj != cMaj) return lMaj > cMaj;
    if (lMin != cMin) return lMin > cMin;
    return lPat > cPat;
}

void UpdateChecker::checkForUpdate(const char* currentVersion) {
    if (checked_.load()) return;
    checked_.store(true);

    HMODULE thisModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCWSTR>(&UpdateChecker::checkForUpdate),
        &thisModule);

    std::string ver(currentVersion);
    try {
        std::thread([ver]() {
            try {
                std::string body = httpGet(kApiUrl);
                if (body.empty()) return;

                std::string latest = jsonValue(body, "version");
                std::string urlWin = jsonValue(body, "url_win");
                std::string changelog = jsonValue(body, "changelog");

                if (latest.empty()) return;

                bool newer = isNewer(latest.c_str(), ver.c_str());

                std::lock_guard<std::mutex> lock(mutex_);
                info_.latestVersion = latest;
                info_.downloadUrl = urlWin;
                info_.changelog = changelog;
                info_.hasUpdate = newer;
            } catch (...) {}
        }).detach();
    } catch (...) {}
}

UpdateChecker::UpdateInfo UpdateChecker::getUpdateInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

} // namespace gui

#endif // _WIN32
