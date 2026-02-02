#include <stdio.h>
#include <vector>
#include <algorithm>
#include <String.h>
#include <Url.h>
#include <OS.h>

// Mocking the blocked domains list from BrowserWindow.cpp
static const std::vector<BString> kBlockedDomains = [] {
    std::vector<BString> domains;
    domains.push_back("adservice.google.com");
    domains.push_back("connect.facebook.net");
    domains.push_back("doubleclick.net");
    domains.push_back("facebook.net");
    domains.push_back("google-analytics.com");
    domains.push_back("googlesyndication.com");
    std::sort(domains.begin(), domains.end());
    return domains;
}();

bool IsBlocked(const BString& url) {
    BUrl checkUrl(url.String(), true);
    if (checkUrl.IsValid()) {
        BString host = checkUrl.Host();
        host.ToLower();

        // Iterate up the domain hierarchy
        // Checks: "mail.google.com", then "google.com", then "com"
        while (!host.IsEmpty()) {
            if (std::binary_search(kBlockedDomains.begin(), kBlockedDomains.end(), host)) {
                 return true;
            }

            // Find the next dot to strip the subdomain
            int32 firstDot = host.FindFirst('.');
            if (firstDot == B_ERROR) break; // No more parents to check

            host.Remove(0, firstDot + 1);
        }
    }
    return false;
}

int main() {
    int failed = 0;

    // Test cases
    struct TestCase {
        const char* url;
        bool shouldBlock;
    };

    TestCase cases[] = {
        {"https://www.google-analytics.com/analytics.js", true},
        {"http://adservice.google.com/test", true},
        {"https://sub.doubleclick.net/something", true},
        {"https://doubleclick.net", true},
        {"https://facebook.net", true},
        {"https://www.facebook.net", true},
        {"https://connect.facebook.net", true},
        {"https://google.com", false},
        {"https://example.com", false},
        {"https://notblocked.doubleclick.net.evil.com", false}, // Should not match doubleclick.net in middle
        {"https://analytics.com", false},
        {"https://myfacebook.net", false}, // Suffix match but not subdomain
        {"https://a.b.c.doubleclick.net", true}
    };

    for (const auto& test : cases) {
        bool result = IsBlocked(test.url);
        if (result != test.shouldBlock) {
            printf("FAIL: %s - Expected %d, got %d\n", test.url, test.shouldBlock, result);
            failed++;
        } else {
             printf("PASS: %s\n", test.url);
        }
    }

    if (failed == 0) printf("All tests passed!\n");
    return failed;
}
