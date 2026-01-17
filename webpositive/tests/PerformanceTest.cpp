#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/time.h>

// --- Mocks ---

class BString {
public:
    std::string s;
    BString() {}
    BString(const char* str) : s(str ? str : "") {}
    BString(const BString& other) : s(other.s) {}

    const char* String() const { return s.c_str(); }
    int FindFirst(char c) const { return (int)s.find(c); }
    void Remove(int pos, int len) { s.erase(pos, len); }
    int Length() const { return (int)s.length(); }

    bool operator<(const BString& other) const { return s < other.s; }
    bool operator==(const BString& other) const { return s == other.s; }
    bool operator==(const char* other) const { return s == (other ? other : ""); }
    friend bool operator==(const char* lhs, const BString& rhs) { return rhs == lhs; }

    // Helper for printf
    const char* c_str() const { return s.c_str(); }
};

class BNetworkCookie {
public:
    BNetworkCookie(BString d) : fDomain(d) {}
    BString Domain() const { return fDomain; }
private:
    BString fDomain;
};

class BNetworkCookieJar {
public:
    std::vector<BNetworkCookie> fCookies;

    class Iterator {
    public:
        Iterator(const std::vector<BNetworkCookie>& cookies) : fCookies(cookies), index(0) {}
        const BNetworkCookie* Next() {
            if (index < fCookies.size()) return &fCookies[index++];
            return NULL;
        }
    private:
        const std::vector<BNetworkCookie>& fCookies;
        size_t index;
    };

    Iterator GetIterator() { return Iterator(fCookies); }
    void AddCookie(const BNetworkCookie& c) { fCookies.push_back(c); }
};

class BListItem {
public:
    BListItem() : fOutlineLevel(0) {}
    virtual ~BListItem() {}
    void SetOutlineLevel(int l) { fOutlineLevel = l; }
    int OutlineLevel() const { return fOutlineLevel; }
private:
    int fOutlineLevel;
};

class BStringItem : public BListItem {
public:
    BStringItem(BString text) : fText(text) {}
    const char* Text() const { return fText.String(); }
    BString fText;
};

class DomainItem : public BStringItem {
public:
    DomainItem(BString text, bool empty) : BStringItem(text), fEmpty(empty) {}
    bool fEmpty;
    void SetEnabled(bool) {}
};

// Mock BOutlineListView with simplified but structurally correct behavior for the operations used
class BOutlineListView {
public:
    struct Node {
        BListItem* item;
        std::vector<Node*> children;
        Node* parent;

        Node(BListItem* i) : item(i), parent(NULL) {}
        ~Node() {
            for(size_t i=0; i<children.size(); ++i) delete children[i];
        }
    };

    Node* root;
    std::vector<BListItem*> fullList;
    std::map<BListItem*, Node*> nodeMap;

    BOutlineListView() {
        root = new Node(NULL); // dummy root
    }

    ~BOutlineListView() {
        delete root;
    }

    void MakeEmpty() {
        delete root;
        root = new Node(NULL);
        fullList.clear();
        nodeMap.clear();
    }

    void AddItem(BListItem* item) {
        Node* node = new Node(item);
        node->parent = root;
        root->children.push_back(node);
        nodeMap[item] = node;
        fullList.push_back(item);
    }

    void AddItem(BListItem* item, int index) {
        Node* node = new Node(item);
        nodeMap[item] = node;

        if (index >= (int)fullList.size()) fullList.push_back(item);
        else fullList.insert(fullList.begin() + index, item);

        int level = item->OutlineLevel();
        Node* parentNode = root;

        if (level > 0) {
            bool found = false;
            for (int i = index - 1; i >= 0; i--) {
                if (fullList[i]->OutlineLevel() == level - 1) {
                    parentNode = nodeMap[fullList[i]];
                    found = true;
                    break;
                }
            }
            if (!found) parentNode = root;
        }

        node->parent = parentNode;

        // Use simpler insertion for benchmark speed (scanning children is slow)
        // But the original code is slow because of ItemUnderAt scanning.
        // We must simulate that.
        std::vector<Node*>::iterator it = parentNode->children.begin();
        while (it != parentNode->children.end()) {
            int childIndex = FullListIndexOf((*it)->item);
            if (childIndex > index) {
                break;
            }
            it++;
        }
        parentNode->children.insert(it, node);
    }

    BListItem* ItemUnderAt(BListItem* parent, bool collapsed, int index) {
        Node* pNode = parent ? nodeMap[parent] : root;
        if (!pNode) return NULL;
        if (index >= 0 && index < (int)pNode->children.size()) {
            return pNode->children[index]->item;
        }
        return NULL;
    }

    int CountItemsUnder(BListItem* parent, bool collapsed) {
        Node* pNode = parent ? nodeMap[parent] : root;
        if (!pNode) return 0;
        return (int)pNode->children.size();
    }

    BListItem* FullListItemAt(int index) {
        if (index >= 0 && index < (int)fullList.size()) return fullList[index];
        return NULL;
    }

    int FullListCountItems() {
        return (int)fullList.size();
    }

    int FullListIndexOf(BListItem* item) {
        for(size_t i=0; i<fullList.size(); ++i) {
            if (fullList[i] == item) return (int)i;
        }
        return -1;
    }

    void RemoveItem(BListItem* item) { }
    void Select(int i) {}
};

// --- Benchmark Class ---

class CookieWindowBenchmark {
public:
    BOutlineListView* fDomains;
    BNetworkCookieJar fCookieJar;
    std::map<BString, std::vector<BNetworkCookie> > fCookieMap;
    std::map<BString, BStringItem*> fDomainMap; // The optimization

    CookieWindowBenchmark() {
        fDomains = new BOutlineListView();
    }

    ~CookieWindowBenchmark() {
        delete fDomains;
    }

    // Original _AddDomain
    BStringItem* _AddDomain_Original(BString domain, bool fake) {
        BStringItem* parent = NULL;
        int firstDot = domain.FindFirst('.');
        if (firstDot >= 0) {
            BString parentDomain(domain);
            parentDomain.Remove(0, firstDot + 1);
            parent = _AddDomain_Original(parentDomain, true);
        } else {
            parent = (BStringItem*)fDomains->FullListItemAt(0);
        }

        BListItem* existing;
        int i = 0;
        // check that we aren't already there
        while ((existing = fDomains->ItemUnderAt(parent, true, i++)) != NULL) {
            DomainItem* stringItem = (DomainItem*)existing;
            if (stringItem->Text() == domain) {
                if (fake == false)
                    stringItem->fEmpty = false;
                return stringItem;
            }
        }

        // Insert the new item, keeping the list alphabetically sorted
        BStringItem* domainItem = new DomainItem(domain, fake);
        domainItem->SetOutlineLevel(parent->OutlineLevel() + 1);
        BStringItem* sibling = NULL;
        int siblingCount = fDomains->CountItemsUnder(parent, true);
        for (i = 0; i < siblingCount; i++) {
            sibling = (BStringItem*)fDomains->ItemUnderAt(parent, true, i);
            if (strcmp(sibling->Text(), domainItem->Text()) > 0) {
                fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling));
                return domainItem;
            }
        }

        if (sibling) {
            fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling)
                + fDomains->CountItemsUnder(sibling, false) + 1);
        } else {
            fDomains->AddItem(domainItem, fDomains->FullListIndexOf(parent) + 1);
        }

        return domainItem;
    }

    void _BuildDomainList_Original() {
        fDomains->MakeEmpty();
        DomainItem* rootItem = new DomainItem("", true);
        fDomains->AddItem(rootItem);

        fCookieMap.clear();
        BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();

        const BNetworkCookie* cookie;
        while ((cookie = it.Next()) != NULL) {
            BString domain = cookie->Domain();
            if (fCookieMap.find(domain) == fCookieMap.end())
                _AddDomain_Original(domain, false);

            fCookieMap[domain].push_back(*cookie);
        }
    }

    // Optimized _AddDomain: uses fDomainMap for fast existence check
    BStringItem* _AddDomain_Optimized(BString domain, bool fake) {
        if (fDomainMap.count(domain)) {
             DomainItem* item = (DomainItem*)fDomainMap[domain];
             if (!fake) item->fEmpty = false;
             return item;
        }

        BStringItem* parent = NULL;
        int firstDot = domain.FindFirst('.');
        if (firstDot >= 0) {
            BString parentDomain(domain);
            parentDomain.Remove(0, firstDot + 1);
            parent = _AddDomain_Optimized(parentDomain, true);
        } else {
            parent = (BStringItem*)fDomains->FullListItemAt(0);
        }

        // Insert the new item, keeping the list alphabetically sorted
        BStringItem* domainItem = new DomainItem(domain, fake);
        domainItem->SetOutlineLevel(parent->OutlineLevel() + 1);
        fDomainMap[domain] = domainItem;

        BStringItem* sibling = NULL;
        int siblingCount = fDomains->CountItemsUnder(parent, true);
        for (int i = 0; i < siblingCount; i++) {
            sibling = (BStringItem*)fDomains->ItemUnderAt(parent, true, i);
            if (strcmp(sibling->Text(), domainItem->Text()) > 0) {
                fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling));
                return domainItem;
            }
        }

        if (sibling) {
            fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling)
                + fDomains->CountItemsUnder(sibling, false) + 1);
        } else {
            fDomains->AddItem(domainItem, fDomains->FullListIndexOf(parent) + 1);
        }

        return domainItem;
    }

    void _BuildDomainList_Optimized_Iter() {
        fDomains->MakeEmpty();
        DomainItem* rootItem = new DomainItem("", true);
        fDomains->AddItem(rootItem);

        fCookieMap.clear();
        fDomainMap.clear();

        BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();

        const BNetworkCookie* cookie;
        while ((cookie = it.Next()) != NULL) {
            BString domain = cookie->Domain();
            if (fCookieMap.find(domain) == fCookieMap.end())
                _AddDomain_Optimized(domain, false);

            fCookieMap[domain].push_back(*cookie);
        }
    }

    // Fully Optimized: 2-pass approach
    void _BuildDomainList_FullyOptimized() {
        fDomains->MakeEmpty();
        DomainItem* rootItem = new DomainItem("", true);
        fDomains->AddItem(rootItem);

        fCookieMap.clear();
        fDomainMap.clear();

        // Pass 1: Build fCookieMap (and sorted domain list implicitly)
        BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();
        const BNetworkCookie* cookie;
        while ((cookie = it.Next()) != NULL) {
            fCookieMap[cookie->Domain()].push_back(*cookie);
        }

        // Pass 2: Iterate sorted map to add domains
        for(std::map<BString, std::vector<BNetworkCookie> >::iterator it = fCookieMap.begin();
            it != fCookieMap.end(); ++it) {
            _AddDomain_Optimized(it->first, false);
        }
    }
};

int main() {
    CookieWindowBenchmark app;

    // Populate with cookies
    int numDomains = 2000;
    int cookiesPerDomain = 5;

    printf("Populating %d domains with %d cookies each...\n", numDomains, cookiesPerDomain);

    for (int i=0; i<numDomains; i++) {
        char buf[64];
        sprintf(buf, "d%05d.com", i); // Padding to ensure random-ish vs sorted
        BString domain(buf);

        for (int c=0; c<cookiesPerDomain; c++) {
            app.fCookieJar.AddCookie(BNetworkCookie(domain));
        }

        // Subdomains
        if (i % 10 == 0) {
            sprintf(buf, "sub.d%05d.com", i);
            BString sub(buf);
            for (int c=0; c<cookiesPerDomain; c++) {
                app.fCookieJar.AddCookie(BNetworkCookie(sub));
            }
        }
    }

    printf("Total cookies: %lu\n", app.fCookieJar.fCookies.size());

    // Measure Original
    struct timeval start, end;
    gettimeofday(&start, NULL);
    app._BuildDomainList_Original();
    gettimeofday(&end, NULL);
    double elapsedOriginal = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Original _BuildDomainList: %.4f seconds\n", elapsedOriginal);

    // Measure Optimized 1 (Just caching existence)
    gettimeofday(&start, NULL);
    app._BuildDomainList_Optimized_Iter();
    gettimeofday(&end, NULL);
    double elapsedOptimized1 = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Optimized 1 (Cache): %.4f seconds\n", elapsedOptimized1);

    // Measure Optimized 2 (2-pass sorted)
    gettimeofday(&start, NULL);
    app._BuildDomainList_FullyOptimized();
    gettimeofday(&end, NULL);
    double elapsedOptimized2 = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Optimized 2 (Sorted): %.4f seconds\n", elapsedOptimized2);

    return 0;
}
