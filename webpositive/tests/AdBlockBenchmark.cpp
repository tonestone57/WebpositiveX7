/*
 * Benchmark for AdBlock loop
 */

#include <stdio.h>
#include <vector>
#include <cstring>
#include <String.h>
#include <Url.h>
#include <OS.h>

static const char* kBlockedDomains[] = {
    "doubleclick.net",
    "googlesyndication.com",
    "google-analytics.com",
    "adservice.google.com",
    "facebook.net",
    "connect.facebook.net",
    NULL
};

void TestOriginalImplementation(const BString& url, int iterations) {
    bigtime_t start = system_time();

    BUrl checkUrl(url);
    if (checkUrl.IsValid()) {
        BString host = checkUrl.Host();
        for (int k = 0; k < iterations; k++) {
            for (int i = 0; kBlockedDomains[i]; i++) {
                if (host == kBlockedDomains[i] || host.EndsWith(BString(".") << kBlockedDomains[i])) {
                    // Match found
                    break;
                }
            }
        }
    }

    bigtime_t end = system_time();
    printf("Original Implementation: %" B_PRId64 " us\n", end - start);
}

void TestOptimizedImplementation(const BString& url, int iterations) {
    bigtime_t start = system_time();

    BUrl checkUrl(url);
    if (checkUrl.IsValid()) {
        BString host = checkUrl.Host();
        for (int k = 0; k < iterations; k++) {
            for (int i = 0; kBlockedDomains[i]; i++) {
                if (host == kBlockedDomains[i]) {
                    break;
                }

                int32 hostLen = host.Length();
                int32 domainLen = strlen(kBlockedDomains[i]);
                if (hostLen > domainLen + 1 && host.EndsWith(kBlockedDomains[i])) {
                    if (host.ByteAt(hostLen - domainLen - 1) == '.') {
                        // Match found
                        break;
                    }
                }
            }
        }
    }

    bigtime_t end = system_time();
    printf("Optimized Implementation: %" B_PRId64 " us\n", end - start);
}

int main() {
    BString testUrl1 = "https://www.google-analytics.com/analytics.js";
    BString testUrl2 = "https://subdomain.doubleclick.net/ad";
    BString testUrl3 = "https://example.com/page";

    int iterations = 100000;

    printf("Benchmarking with match 1 (www.google-analytics.com)...\n");
    TestOriginalImplementation(testUrl1, iterations);
    TestOptimizedImplementation(testUrl1, iterations);

    printf("\nBenchmarking with match 2 (subdomain.doubleclick.net)...\n");
    TestOriginalImplementation(testUrl2, iterations);
    TestOptimizedImplementation(testUrl2, iterations);

    printf("\nBenchmarking with no match (example.com)...\n");
    TestOriginalImplementation(testUrl3, iterations);
    TestOptimizedImplementation(testUrl3, iterations);

    return 0;
}
