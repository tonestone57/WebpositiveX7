#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sys/time.h>
#include <cstring>

class BString {
public:
    std::string s;
    BString() {}
    BString(const char* str) : s(str ? str : "") {}
    BString(const BString& other) : s(other.s) {}

    bool operator<(const BString& other) const { return s < other.s; }
    bool operator==(const BString& other) const { return s == other.s; }

    const char* String() const { return s.c_str(); }
};

struct DomainNode {
    BString domain;
    bool fake;
    std::map<BString, DomainNode*> children;

    DomainNode(const BString& d, bool f)
        :
        domain(d),
        fake(f)
    {
    }

    ~DomainNode()
    {
        std::map<BString, DomainNode*>::iterator it;
        for (it = children.begin(); it != children.end(); it++) {
            delete it->second;
        }
    }
};

// Original PruneTree
static DomainNode*
PruneTree_Original(DomainNode* node)
{
    std::map<BString, DomainNode*> newChildren;
    std::map<BString, DomainNode*>::iterator it;

    for (it = node->children.begin(); it != node->children.end(); ++it) {
        DomainNode* child = it->second;
        // We need to detach child from node->children to prevent double free in node destructor
        // if we were not rebuilding newChildren from scratch.
        // But here we are building newChildren and assigning it to node->children at the end.
        // The old node->children map is destroyed, but the pointers in it are lost (not deleted).
        // Wait, ~DomainNode deletes children.
        // In the original code:
        // node->children = newChildren;
        // The operator= of std::map clears the old map.
        // But the pointers in the old map are NOT deleted by std::map destructor/assignment.
        // So they are leaked?
        // NO.
        // The pointers are MOVED to newChildren (if prunedChild returned same pointer).
        // If prunedChild != child (collapse), the old 'child' node is deleted inside PruneTree.
        // So the pointers in newChildren are the ones we want to keep.
        // The old map `node->children` is overwritten. The `std::map` clears its nodes (internal tree nodes),
        // but it does NOT call delete on the `DomainNode*` values.
        // So this is safe regarding double free.
        // BUT, what if `child` was NOT returned? (It was deleted).
        // Then `newChildren` contains `prunedChild` (which is `child`'s child).
        // The old `child` pointer is gone.
        // The old `node->children` map held `child`.
        // When `node->children = newChildren` happens, the old map is cleared.
        // The `child` pointer is just dropped from the map.
        // Since `child` was deleted, this is correct.

        DomainNode* prunedChild = PruneTree_Original(child);
        newChildren[prunedChild->domain] = prunedChild;
    }

    // Force clear old children to avoid them being deleted by ~DomainNode if we were destructing?
    // No, operator= handles map data.
    node->children = newChildren;

    if (node->fake && node->children.size() == 1) {
        DomainNode* child = node->children.begin()->second;
        // We must clear children so they are not deleted when node is deleted
        node->children.clear();
        delete node;
        return child;
    }

    return node;
}

// Optimized PruneTree
static DomainNode*
PruneTree_Optimized(DomainNode* node)
{
    // In-place modification strategy
    std::vector<BString> keysToRemove;
    std::vector<DomainNode*> nodesToInsert;

    // Use iterator to traverse.
    // Note: We cannot modify the map (insert/erase) while iterating.
    for (std::map<BString, DomainNode*>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
        DomainNode* child = it->second;
        DomainNode* prunedChild = PruneTree_Optimized(child);

        if (prunedChild != child) {
            // Child collapsed/changed
            keysToRemove.push_back(it->first);
            nodesToInsert.push_back(prunedChild);
        }
    }

    if (!keysToRemove.empty()) {
        for (size_t i = 0; i < keysToRemove.size(); ++i) {
            // Remove old key (and the pointer value, which is already deleted if collapsed)
            // Wait, if prunedChild != child, it means child was deleted (in the recursive call).
            // So node->children[key] is now a dangling pointer!
            // We must strictly not access it.
            // erase by key is safe.
            node->children.erase(keysToRemove[i]);
        }
        for (size_t i = 0; i < nodesToInsert.size(); ++i) {
            node->children[nodesToInsert[i]->domain] = nodesToInsert[i];
        }
    }

    if (node->fake && node->children.size() == 1) {
        DomainNode* child = node->children.begin()->second;
        node->children.clear();
        delete node;
        return child;
    }

    return node;
}

void BuildTree(DomainNode* root, int depth, int width) {
    if (depth == 0) return;
    for (int i = 0; i < width; ++i) {
        char buf[32];
        sprintf(buf, "d%d", i);
        BString domain(buf);
        DomainNode* child = new DomainNode(domain, depth > 1); // Internal nodes fake, leaves real
        root->children[domain] = child;
        BuildTree(child, depth - 1, width);
    }
}

// Deep chain to test collapsing
void BuildDeepChain(DomainNode* root, int depth) {
    DomainNode* current = root;
    for (int i = 0; i < depth; ++i) {
        BString domain("sub");
        DomainNode* child = new DomainNode(domain, true); // All fake
        current->children[domain] = child;
        current = child;
    }
    // Last one real
    current->fake = false;
}

int main() {
    int width = 5;
    int depth = 6;

    // Benchmark Original
    {
        DomainNode* root = new DomainNode("root", true);
        BuildTree(root, depth, width);
        // Add some deep chains
        BuildDeepChain(root, 100);

        struct timeval start, end;
        gettimeofday(&start, NULL);

        PruneTree_Original(root);

        gettimeofday(&end, NULL);
        long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        std::cout << "Original: " << us << " us" << std::endl;
        delete root;
    }

    // Benchmark Optimized
    {
        DomainNode* root = new DomainNode("root", true);
        BuildTree(root, depth, width);
        BuildDeepChain(root, 100);

        struct timeval start, end;
        gettimeofday(&start, NULL);

        PruneTree_Optimized(root);

        gettimeofday(&end, NULL);
        long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        std::cout << "Optimized: " << us << " us" << std::endl;
        delete root;
    }

    return 0;
}
