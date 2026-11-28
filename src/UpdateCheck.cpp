#include "UpdateCheck.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "0.0.0"   // the self-test program
#endif

namespace bn {

static const char* kApi = "https://api.github.com/repos/dmajorsound/Binocular/releases?per_page=1";   // newest, betas included
static const char* kAssetBase = "https://github.com/dmajorsound/Binocular/releases/download/";

static std::atomic<bool> g_started{false};
static std::atomic<int> g_newer{0};
static std::mutex g_mu;
static std::string g_url, g_asset;

// download state (guarded by g_mu): 0 idle, 1 downloading, 2 there, 3 failed
static int g_dl = 0, g_dlErr = 0;
static double g_prog = 0;
static std::string g_path, g_err, g_folder;
static bool g_installAfter = false, g_installDone = false;
static std::atomic<bool> g_quit{false};

// the background threads; joined when the plug-in is unloaded, so they never run on in an unloaded module
static struct Worker {
    std::thread check, download;
    ~Worker() {
        g_quit = true;
        if (check.joinable()) check.join();
        if (download.joinable()) download.join();
    }
} g_worker;

// the first "key": "value" in the JSON text, starting at `from` (*next: just after it)
static std::string jsonString(const std::string& js, const char* key, size_t from = 0, size_t* next = nullptr) {
    const std::string k = std::string("\"") + key + "\"";
    size_t p = js.find(k, from);
    if (p == std::string::npos) return {};
    p = js.find(':', p + k.size());
    if (p == std::string::npos) return {};
    p = js.find('"', p);
    if (p == std::string::npos) return {};
    const size_t e = js.find('"', p + 1);
    if (e == std::string::npos || e - p > 400) return {};
    if (next) *next = e + 1;
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

[[maybe_unused]] static bool endsWith(const std::string& s, const char* e) {
    const size_t n = strlen(e);
    return s.size() >= n && s.compare(s.size() - n, n, e) == 0;
}

// the installer for this system among the release's files: Mac "...-mac.dmg", Windows "...-win64.exe", never an uninstaller
static std::string pickAsset(const std::string& js) {
    size_t at = 0;
    for (;;) {
        size_t next = 0;
        const std::string u = jsonString(js, "browser_download_url", at, &next);
        if (u.empty()) return {};
        at = next;
        if (u.compare(0, strlen(kAssetBase), kAssetBase) != 0 || u.find("uninstall") != std::string::npos) continue;
       #if JUCE_MAC
        if (endsWith(u, "-mac.dmg")) return u;
       #elif JUCE_WINDOWS
        if (endsWith(u, "-win64.exe")) return u;
       #endif
    }
}

void updateCheckStart() {
    if (g_started.exchange(true)) return;   // once per host session
    g_worker.check = std::thread([] {
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
            g_asset = pickAsset(js);
        }
        g_newer = code;
    });
}

int updateNewerVersion() { return g_newer.load(); }

std::string updateUrl() {
    std::lock_guard<std::mutex> l(g_mu);
    return g_url;
}

std::string updateAssetUrl() {
    std::lock_guard<std::mutex> l(g_mu);
    return g_asset;
}

void updateSetAssetForTest(const std::string& url) {
    std::lock_guard<std::mutex> l(g_mu);
    g_asset = url;
}

// ---- the download folder, kept in the plug-in settings (~/Library/Application Support/dmajorsound, %APPDATA%\dmajorsound)
static std::unique_ptr<juce::PropertiesFile> settings() {
    juce::PropertiesFile::Options o;
    o.applicationName = "Binocular";
    o.folderName = "dmajorsound";
    o.filenameSuffix = "settings";
    o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<juce::PropertiesFile>(o);
}

std::string updateFolder() {
    {
        std::lock_guard<std::mutex> l(g_mu);
        if (!g_folder.empty()) return g_folder;
    }
    juce::String f = settings()->getValue("updateFolder");
    if (f.isEmpty() || !juce::File::isAbsolutePath(f) || !juce::File(f).isDirectory())
        f = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads").getFullPathName();
    std::lock_guard<std::mutex> l(g_mu);
    g_folder = f.toStdString();
    return g_folder;
}

void setUpdateFolder(const std::string& folder, bool remember) {
    {
        std::lock_guard<std::mutex> l(g_mu);
        g_folder = folder;
    }
    if (remember) {
        auto s = settings();
        s->setValue("updateFolder", juce::String(folder));
        s->saveIfNeeded();
    }
}

// ---- the download: "<file>.part" first, renamed only when it is whole
static void fail(int code, const std::string& why) {
    std::lock_guard<std::mutex> l(g_mu);
    g_dl = 3;
    g_dlErr = code;
    g_err = why;
}

static void downloadNow(std::string url, std::string folder) {
    const juce::File dir(folder);
    if (!dir.isDirectory() && !dir.createDirectory()) return fail(5, "the folder " + folder + " cannot be written to");
    const juce::String name = juce::URL::removeEscapeChars(juce::String(url).fromLastOccurrenceOf("/", false, false));
    if (name.isEmpty() || name.containsAnyOf("/\\:")) return fail(2, "GitHub gave no file name");
    const juce::File target = dir.getChildFile(name), part = dir.getChildFile(name + ".part");
    part.deleteFile();

    int status = 0;
    auto opts = juce::URL::DownloadTaskOptions()
                    .withExtraHeaders("User-Agent: Binocular/" JucePlugin_VersionString)
                    .withUsePost(false);
    // redirects are followed (GitHub hands the file over from objects.githubusercontent.com)
    std::unique_ptr<juce::URL::DownloadTask> task = juce::URL(juce::String(url)).downloadToFile(part, opts);
    if (task == nullptr) return fail(1, "no connection to GitHub");

    juce::int64 last = -1;
    auto lastChange = std::chrono::steady_clock::now();
    while (!task->isFinished()) {
        if (g_quit) { task.reset(); part.deleteFile(); return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        const juce::int64 got = task->getLengthDownloaded(), total = task->getTotalLength();
        {
            std::lock_guard<std::mutex> l(g_mu);
            g_prog = total > 0 ? juce::jlimit(0.0, 1.0, (double)got / (double)total) : -1.0;
        }
        const auto now = std::chrono::steady_clock::now();
        if (got != last) { last = got; lastChange = now; }
        else if (now - lastChange > std::chrono::seconds(30)) {   // nothing for 30 s: give up
            task.reset();
            part.deleteFile();
            return fail(3, "nothing came for 30 seconds");
        }
    }
    status = task->statusCode();
    const bool err = task->hadError();
    const juce::int64 total = task->getTotalLength(), got = task->getLengthDownloaded();
    task.reset();   // closes the file
    if (status != 0 && (status < 200 || status >= 300)) {
        part.deleteFile();
        return fail(2, "GitHub answered with error " + std::to_string(status));
    }
    if (err || got <= 0) {
        part.deleteFile();
        return fail(1, "the connection broke off");
    }
    if ((total > 0 && part.getSize() != total) || part.getSize() < 100000) {   // incomplete, or an error page
        part.deleteFile();
        return fail(4, "it arrived incomplete");
    }
    target.deleteFile();
    if (!part.moveFileTo(target)) {
        part.deleteFile();
        return fail(5, "the folder " + folder + " cannot be written to");
    }
    std::lock_guard<std::mutex> l(g_mu);
    g_dl = 2;
    g_prog = 1;
    g_path = target.getFullPathName().toStdString();
}

void updateDownloadStart(bool installAfter) {
    const std::string url = updateAssetUrl(), folder = updateFolder();
    if (url.empty()) return;
    {
        std::lock_guard<std::mutex> l(g_mu);
        if (g_dl == 1) return;
        g_dl = 1;
        g_prog = -1;
        g_err.clear();
        g_dlErr = 0;
        g_installAfter = installAfter;
        g_installDone = false;
    }
    if (g_worker.download.joinable()) g_worker.download.join();   // a finished earlier attempt
    g_worker.download = std::thread([url, folder] { downloadNow(url, folder); });
}

int updateDownloadState(double* progress, std::string* path, std::string* err) {
    std::lock_guard<std::mutex> l(g_mu);
    if (progress) *progress = g_prog;
    if (path) *path = g_path;
    if (err) *err = g_err;
    return g_dl;
}

int updateDownloadError() {
    std::lock_guard<std::mutex> l(g_mu);
    return g_dl == 3 ? g_dlErr : 0;
}

void updatePoll() {
    std::string open;
    {
        std::lock_guard<std::mutex> l(g_mu);
        if (g_dl == 2 && g_installAfter && !g_installDone) {
            g_installDone = true;
            open = g_path;
        }
    }
    if (!open.empty()) openInstaller(open);
}

// ---- opening it: no shell, every argument on its own
void openInstaller(const std::string& path) {
    const juce::File f(path);
    if (!f.existsAsFile()) return;
   #if JUCE_MAC
    std::thread([path] {
        const juce::String mnt = "/tmp/binocular-update";
        {
            juce::ChildProcess d;   // an earlier attempt may still be attached there
            if (d.start(juce::StringArray { "/usr/bin/hdiutil", "detach", "-quiet", mnt }, 0)) d.waitForProcessToFinish(15000);
        }
        juce::ChildProcess a;
        bool ok = a.start(juce::StringArray { "/usr/bin/hdiutil", "attach", "-nobrowse", "-noautoopen", "-quiet",
                                              "-mountpoint", mnt, juce::String(path) }, 0)
                  && a.waitForProcessToFinish(60000) && a.getExitCode() == 0;
        const juce::File pkg(mnt + "/Install Binocular.pkg");
        juce::ChildProcess o;
        if (ok && pkg.existsAsFile()) ok = o.start(juce::StringArray { "/usr/bin/open", pkg.getFullPathName() }, 0);
        else ok = false;
        if (!ok) {   // fall back: open the disk image in the Finder
            juce::ChildProcess o2;
            o2.start(juce::StringArray { "/usr/bin/open", juce::String(path) }, 0);
            o2.waitForProcessToFinish(10000);
        } else
            o.waitForProcessToFinish(10000);
    }).detach();
   #elif JUCE_WINDOWS
    f.startAsProcess();
   #else
    f.revealToUser();
   #endif
}

void revealInFileBrowser(const std::string& path) {
    const juce::File f(path);
    if (f.exists()) f.revealToUser();
}

void openWebPage(const std::string& url) {
    if (url.compare(0, 19, "https://github.com/") != 0 && url.compare(0, 24, "https://dmajorsound.com/") != 0) return;
    for (char ch : url)
        if (ch <= ' ' || ch == '"' || ch == '\'' || ch == '`') return;
    juce::URL(juce::String(url)).launchInDefaultBrowser();
}

} // namespace bn
