#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "UpdateChecker.hpp"
#include <dlfcn.h>
#include <mutex>
#include <thread>

namespace gui {

std::mutex UpdateChecker::mutex_;
UpdateChecker::UpdateInfo UpdateChecker::info_;
std::atomic<bool> UpdateChecker::checked_{false};

std::string UpdateChecker::httpGet(const char* url) {
    @autoreleasepool {
        NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
        if (!nsUrl) return "";
        NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:nsUrl];
        [request setTimeoutInterval:2.0];

        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        __block NSData* responseData = nil;
        __block NSError* responseError = nil;

        NSURLSessionDataTask* task =
            [[NSURLSession sharedSession] dataTaskWithRequest:request
                completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
                    responseData = data;
                    responseError = error;
                    dispatch_semaphore_signal(sem);
                }];
        [task resume];
        dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC));

        if (responseError || !responseData || responseData.length == 0)
            return "";

        return std::string(static_cast<const char*>(responseData.bytes), responseData.length);
    }
}

void UpdateChecker::openInBrowser(const char* url) {
    @autoreleasepool {
        NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
        [[NSWorkspace sharedWorkspace] openURL:nsUrl];
    }
}

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

    static std::once_flag pinOnce;
    std::call_once(pinOnce, [] {
        Dl_info di{};
        if (dladdr(reinterpret_cast<void*>(&UpdateChecker::checkForUpdate), &di) && di.dli_fname) {
            dlopen(di.dli_fname, RTLD_NOW | RTLD_NODELETE);
        }
    });

    std::string ver(currentVersion);
    try {
        std::thread([ver]() {
            try {
                std::string body = httpGet(kApiUrl);
                if (body.empty()) return;

                std::string latest = jsonValue(body, "version");
                std::string urlMac = jsonValue(body, "url_mac");
                std::string changelog = jsonValue(body, "changelog");

                if (latest.empty()) return;

                bool newer = isNewer(latest.c_str(), ver.c_str());

                std::lock_guard<std::mutex> lock(mutex_);
                info_.latestVersion = latest;
                info_.downloadUrl = urlMac;
                info_.changelog = changelog;
                info_.hasUpdate = newer;
            } catch (...) {
                // не даём исключению упасть в хост
            }
        }).detach();
    } catch (...) {}
}

UpdateChecker::UpdateInfo UpdateChecker::getUpdateInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

} // namespace gui
