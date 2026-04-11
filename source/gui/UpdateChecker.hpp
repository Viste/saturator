#pragma once

#include <string>
#include <mutex>
#include <atomic>

namespace gui {

class UpdateChecker {
public:
    struct UpdateInfo {
        std::string latestVersion;
        std::string downloadUrl;  // платформо-зависимая ссылка
        std::string changelog;
        bool hasUpdate = false;
    };

    static void checkForUpdate(const char* currentVersion);

    static UpdateInfo getUpdateInfo();

    static void openInBrowser(const char* url);

    static constexpr const char* kApiUrl = "https://dev-vlab.ru/api/saturator/version";

private:
    static std::mutex mutex_;
    static UpdateInfo info_;
    static std::atomic<bool> checked_;

    static bool isNewer(const char* latest, const char* current);

    static std::string httpGet(const char* url);
};

} // namespace gui
