#include "UpdateCheck.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace bn {

static const char* kApi = "https://api.github.com/repos/dmajorsound/Binocular/releases?per_page=1";   // newest, betas included

static std::atomic<bool> g_started{false};
static std::atomic<int> g_newer{0};
static std::mutex g_mu;
static std::string g_url;

// the background request; joined when the plug-in is unloaded, so it never runs on in an unloaded module
static struct Worker {
    std::thread t;
    ~Worker() { if (t.joinable()) t.join(); }
} g_worker;

// the first "key": "value" in the JSON text
static std::string jsonString(const std::string& js, const char* key) {
    const std::string k = std::string("\"") + key + "\"";
    size_t p = js.find(k);
    if (p == std::string::npos) return {};
    p = js.find(':', p + k.size());
    if (p == std::string::npos) return {};
    p = js.find('"', p);
    if (p == std::string::npos) return {};
    const size_t e = js.find('"', p + 1);
    if (e == std::string::npos || e - p > 300) return {};
    return js.substr(p + 1, e - p - 1);
}

// "v1.0.4", "1.0.4-beta" -> 10004
static int versionCode(const std::string& v) {
    std::vector<int> r;
    int cur = -1;
    for (char ch : v) {
        if (ch >= '0' && ch <= '9') cur = (cur < 0 ? 0 : cur * 10) + (ch - '0');
        else if (cur >= 0) {
            r.push_back(cur);
            cur = -1;
            if (ch != '.') break;   // stop at a suffix
        }
    }
    if (cur >= 0) r.push_back(cur);
    while (r.size() < 3) r.push_back(0);
    if (r[0] > 99 || r[1] > 99 || r[2] > 99) return 0;
    return r[0] * 10000 + r[1] * 100 + r[2];
}

void updateCheckStart() {
    if (g_started.exchange(true)) return;   // once per host session
    g_worker.t = std::thread([] {
        juce::URL url(kApi);
        int status = 0;
        std::unique_ptr<juce::InputStream> in = url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withExtraHeaders("Accept: application/vnd.github+json\r\nUser-Agent: Binocular/" JucePlugin_VersionString)
                .withConnectionTimeoutMs(4000)
                .withStatusCode(&status));
        if (in == nullptr || status != 200) return;
        const std::string js = in->readEntireStreamAsString().substring(0, 2000000).toStdString();
        const std::string tag = jsonString(js, "tag_name"), page = jsonString(js, "html_url");
        const int code = versionCode(tag);
        if (code <= versionCode(JucePlugin_VersionString) || page.compare(0, 19, "https://github.com/") != 0) return;
        {
            std::lock_guard<std::mutex> l(g_mu);
            g_url = page;
        }
        g_newer = code;
    });
}

int updateNewerVersion() { return g_newer.load(); }

std::string updateUrl() {
    std::lock_guard<std::mutex> l(g_mu);
    return g_url;
}

void openWebPage(const std::string& url) {
    if (url.compare(0, 19, "https://github.com/") != 0) return;
    for (char ch : url)
        if (ch <= ' ' || ch == '"' || ch == '\'' || ch == '`') return;
    juce::URL(juce::String(url)).launchInDefaultBrowser();
}

} // namespace bn
