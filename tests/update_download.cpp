// Binocular - run by the GitHub build on macOS and Windows: downloads an already published installer the way the
// update note in the footer does (the same code), and checks that it arrived whole. If it fails, nothing is released.
//   binocular_update_test <release asset URL> <folder>
#include "../src/UpdateCheck.h"
#include <juce_core/juce_core.h>
#include <chrono>
#include <cstdio>
#include <thread>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: binocular_update_test <url> <folder>\n");
        return 2;
    }
    bn::setUpdateFolder(argv[2], false);
    bn::updateSetAssetForTest(argv[1]);
    bn::updateDownloadStart(false);
    double prog = 0;
    std::string path, err;
    int st = 1;
    for (int i = 0; i < 1800 && (st = bn::updateDownloadState(&prog, &path, &err)) == 1; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (st != 2) {
        fprintf(stderr, "download failed (state %d): %s\n", st, err.c_str());
        return 1;
    }
    const juce::int64 size = juce::File(path).getSize();
    printf("downloaded %s (%lld bytes)\n", path.c_str(), (long long)size);
    return size > 500000 ? 0 : 1;   // an installer, not an error page
}
