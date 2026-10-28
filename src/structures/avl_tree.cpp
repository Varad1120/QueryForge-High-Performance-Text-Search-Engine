#include "structures/avl_tree.hpp"

AVLTree::AVLTree() : root_(nullptr) {}

AVLTree::~AVLTree() {
    destroyRec(root_);
}

void AVLTree::destroyRec(AVLNode* n) {
    if (!n) return;
    destroyRec(n->left);
    destroyRec(n->right);
    delete n;
}

void AVLTree::insert(long long key) {
    root_ = insertRec(root_, key);
}

bool AVLTree::contains(long long key) const {
    const AVLNode* cur = root_;
    while (cur) {
        if (key == cur->key)     return true;
        else if (key < cur->key) cur = cur->left;
        else                     cur = cur->right;
    }
    return false;
}

std::vector<long long> AVLTree::inOrder() const {
    std::vector<long long> out;
    inOrderRec(root_, out);
    return out;
}

int AVLTree::height() const {
    return nodeHeight(root_);
}

bool AVLTree::isValid() const {
    return isValidRec(root_, nullptr, nullptr);
}

int AVLTree::nodeHeight(const AVLNode* n) const {
    return n ? n->height : 0;
}

int AVLTree::balanceFactor(const AVLNode* n) const {
    if (!n) return 0;
    return nodeHeight(n->left) - nodeHeight(n->right);
}

void AVLTree::updateHeight(AVLNode* n) {
    if (!n) return;
    int lh = nodeHeight(n->left);
    int rh = nodeHeight(n->right);
    n->height = 1 + (lh > rh ? lh : rh);
}

AVLNode* AVLTree::rotateRight(AVLNode* y) {
    AVLNode* x  = y->left;
    AVLNode* T2 = x->right;
    x->right = y;
    y->left  = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
}

AVLNode* AVLTree::rotateLeft(AVLNode* x) {
    AVLNode* y  = x->right;
    AVLNode* T2 = y->left;
    y->left  = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

AVLNode* AVLTree::rebalance(AVLNode* n) {
    updateHeight(n);
    int bf = balanceFactor(n);

    if (bf > 1) {
        if (balanceFactor(n->left) < 0)
            n->left = rotateLeft(n->left);
        return rotateRight(n);
    }

    if (bf < -1) {
        if (balanceFactor(n->right) > 0)
            n->right = rotateRight(n->right);
        return rotateLeft(n);
    }

    return n;
}

AVLNode* AVLTree::insertRec(AVLNode* n, long long key) {
    if (!n) return new AVLNode(key);

    if (key < n->key)      n->left  = insertRec(n->left,  key);
    else if (key > n->key) n->right = insertRec(n->right, key);
    else                   return n;

    return rebalance(n);
}

void AVLTree::inOrderRec(const AVLNode* n, std::vector<long long>& out) const {
    if (!n) return;
    inOrderRec(n->left, out);
    out.push_back(n->key);
    inOrderRec(n->right, out);
}

bool AVLTree::isValidRec(const AVLNode* n, long long* minKey, long long* maxKey) const {
    if (!n) return true;

    if (minKey && n->key <= *minKey) return false;
    if (maxKey && n->key >= *maxKey) return false;

    int bf = balanceFactor(n);
    if (bf < -1 || bf > 1) return false;

    int expected = 1 + (nodeHeight(n->left) > nodeHeight(n->right)
                        ? nodeHeight(n->left) : nodeHeight(n->right));
    if (n->height != expected) return false;

    return isValidRec(n->left,  minKey,  const_cast<long long*>(&n->key))
        && isValidRec(n->right, const_cast<long long*>(&n->key), maxKey);
}
