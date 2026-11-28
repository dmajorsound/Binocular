// Is there a newer Binocular, and getting it (as in OLAAF 0.8.7): asks GitHub once per host session in the
// background; when the release has an installer for this system, the update note in the footer downloads it
// into a folder (default: Downloads) and opens it. Without a connection nothing happens and nothing is shown.
#pragma once
#include <string>

namespace bn {

// starts the check (only the first call does anything)
void updateCheckStart();
// 0: nothing (yet); otherwise the newer version as major*10000 + minor*100 + patch
int updateNewerVersion();
// the GitHub page of that release
std::string updateUrl();
// the installer for this system on GitHub (empty: none, e.g. on Linux; then a click opens the release page)
std::string updateAssetUrl();

// the download folder (remembered in the plug-in settings; default ~/Downloads or %USERPROFILE%\Downloads)
std::string updateFolder();
void setUpdateFolder(const std::string& folder, bool remember = true);

// downloads the installer in the background; installAfter: open it as soon as it is there
void updateDownloadStart(bool installAfter);
// 0 idle, 1 downloading, 2 there (path), 3 failed (err: why, in words for the footer)
// progress: 0..1, or -1 while the size is unknown
int updateDownloadState(double* progress, std::string* path, std::string* err);
// why the last download failed, as a number for the footer: 1 no connection, 2 GitHub answered with an error,
// 3 nothing came for 30 s, 4 it arrived incomplete, 5 the folder cannot be written to
int updateDownloadError();
// called regularly on the message thread: opens the installer once a "download and install" has arrived
void updatePoll();

// opens the downloaded installer: Mac - the .dmg is attached and "Install Binocular.pkg" opened; Windows - the setup
void openInstaller(const std::string& path);
// shows the file in the Finder / Explorer
void revealInFileBrowser(const std::string& path);
// opens a github.com or dmajorsound.com page in the browser
void openWebPage(const std::string& url);

// for the self-test in the GitHub build: download this asset instead of the newest one
void updateSetAssetForTest(const std::string& url);

} // namespace bn
