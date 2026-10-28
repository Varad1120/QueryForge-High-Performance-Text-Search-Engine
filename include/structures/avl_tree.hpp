#pragma once
#include <vector>

struct AVLNode {
    long long key;
    int       height;
    AVLNode*  left;
    AVLNode*  right;

    explicit AVLNode(long long k)
        : key(k), height(1), left(nullptr), right(nullptr) {}
};

class AVLTree {
public:
    AVLTree();
    ~AVLTree();

    AVLTree(const AVLTree&)            = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(long long key);
    void remove(long long key);
    bool contains(long long key) const;

    std::vector<long long> rangeQuery(long long low, long long high) const;
    std::vector<long long> inOrder() const;

    int  height() const;
    bool isValid() const;

private:
    AVLNode* root_;

    int  nodeHeight(const AVLNode* n) const;
    int  balanceFactor(const AVLNode* n) const;
    void updateHeight(AVLNode* n);

    AVLNode* rotateRight(AVLNode* y);
    AVLNode* rotateLeft(AVLNode* x);
    AVLNode* rebalance(AVLNode* n);

    AVLNode* insertRec(AVLNode* n, long long key);
    AVLNode* removeRec(AVLNode* n, long long key);
    AVLNode* minNode(AVLNode* n) const;

    void inOrderRec(const AVLNode* n, std::vector<long long>& out) const;
    void rangeRec(const AVLNode* n, long long lo, long long hi,
                  std::vector<long long>& out) const;

    void destroyRec(AVLNode* n);
    bool isValidRec(const AVLNode* n, long long* minKey, long long* maxKey) const;
};
