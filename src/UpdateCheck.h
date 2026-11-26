// Is there a newer Binocular? Asks GitHub once per host session, in the background (as in OLAAF), and gives up
// quietly when there is no connection or no answer within a few seconds.
#pragma once
#include <string>

namespace bn {

// starts the check (only the first call does anything)
void updateCheckStart();
// 0: nothing (yet); otherwise the newer version as major*10000 + minor*100 + patch
int updateNewerVersion();
// the GitHub page of that release
std::string updateUrl();
// opens a github.com or dmajorsound.com page in the browser
void openWebPage(const std::string& url);

} // namespace bn
