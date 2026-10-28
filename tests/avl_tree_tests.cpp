#include <catch2/catch_test_macros.hpp>
#include "structures/avl_tree.hpp"
#include <algorithm>
#include <random>
#include <vector>

TEST_CASE("AVLTree sorted insertion maintains AVL invariant", "[avl_tree]") {
    AVLTree tree;
    for (long long i = 1; i <= 20; ++i) {
        tree.insert(i);
        REQUIRE(tree.isValid());
    }
    REQUIRE(tree.height() <= 5);
    for (long long i = 1; i <= 20; ++i) {
        REQUIRE(tree.contains(i));
    }
}

TEST_CASE("AVLTree reverse-sorted insertion maintains AVL invariant", "[avl_tree]") {
    AVLTree tree;
    for (long long i = 20; i >= 1; --i) {
        tree.insert(i);
        REQUIRE(tree.isValid());
    }
    REQUIRE(tree.height() <= 5);
    for (long long i = 1; i <= 20; ++i) {
        REQUIRE(tree.contains(i));
    }
}

TEST_CASE("AVLTree handles duplicate keys gracefully", "[avl_tree]") {
    AVLTree tree;
    tree.insert(10);
    tree.insert(20);
    tree.insert(10);
    REQUIRE(tree.isValid());
    std::vector<long long> expected = {10, 20};
    REQUIRE(tree.inOrder() == expected);
}

TEST_CASE("AVLTree contains checks present and absent keys", "[avl_tree]") {
    AVLTree tree;
    REQUIRE_FALSE(tree.contains(42));

    tree.insert(42);
    REQUIRE(tree.contains(42));
    REQUIRE_FALSE(tree.contains(41));
    REQUIRE_FALSE(tree.contains(43));
}

TEST_CASE("AVLTree inOrder returns keys in strictly ascending order", "[avl_tree]") {
    AVLTree tree;
    std::vector<long long> keys = {15, 3, 9, 24, 1, 8, 12, 19, 30};
    for (auto k : keys) {
        tree.insert(k);
    }
    std::sort(keys.begin(), keys.end());
    REQUIRE(tree.inOrder() == keys);
}

TEST_CASE("AVLTree exercises all four insertion rotation cases", "[avl_tree]") {
    SECTION("Left-Left rotation") {
        AVLTree tree;
        tree.insert(30);
        tree.insert(20);
        tree.insert(10);
        REQUIRE(tree.isValid());
        REQUIRE(tree.height() == 2);
    }

    SECTION("Right-Right rotation") {
        AVLTree tree;
        tree.insert(10);
        tree.insert(20);
        tree.insert(30);
        REQUIRE(tree.isValid());
        REQUIRE(tree.height() == 2);
    }

    SECTION("Left-Right rotation") {
        AVLTree tree;
        tree.insert(30);
        tree.insert(10);
        tree.insert(20);
        REQUIRE(tree.isValid());
        REQUIRE(tree.height() == 2);
    }

    SECTION("Right-Left rotation") {
        AVLTree tree;
        tree.insert(10);
        tree.insert(30);
        tree.insert(20);
        REQUIRE(tree.isValid());
        REQUIRE(tree.height() == 2);
    }
}

TEST_CASE("AVLTree deletion handles leaf, single-child, and two-children nodes", "[avl_tree]") {
    AVLTree tree;
    std::vector<long long> keys = {20, 10, 30, 5, 15, 25, 35, 12};
    for (auto k : keys) {
        tree.insert(k);
    }

    SECTION("Delete leaf node") {
        tree.remove(5);
        REQUIRE_FALSE(tree.contains(5));
        REQUIRE(tree.isValid());
    }

    SECTION("Delete single-child node") {
        tree.remove(15);
        REQUIRE_FALSE(tree.contains(15));
        REQUIRE(tree.contains(12));
        REQUIRE(tree.isValid());
    }

    SECTION("Delete two-child node") {
        tree.remove(20);
        REQUIRE_FALSE(tree.contains(20));
        REQUIRE(tree.contains(25));
        REQUIRE(tree.contains(10));
        REQUIRE(tree.contains(30));
        REQUIRE(tree.isValid());
    }

    SECTION("Delete nonexistent key is no-op") {
        tree.remove(999);
        REQUIRE(tree.isValid());
        REQUIRE(tree.inOrder().size() == keys.size());
    }
}

TEST_CASE("AVLTree exercises all four deletion rotation cases", "[avl_tree]") {
    SECTION("LL rotation on delete") {
        AVLTree tree;
        tree.insert(4);
        tree.insert(2);
        tree.insert(5);
        tree.insert(1);
        tree.insert(3);
        tree.remove(5);
        REQUIRE(tree.isValid());
        REQUIRE_FALSE(tree.contains(5));
    }

    SECTION("LR rotation on delete") {
        AVLTree tree;
        tree.insert(4);
        tree.insert(2);
        tree.insert(5);
        tree.insert(3);
        tree.remove(5);
        REQUIRE(tree.isValid());
        REQUIRE_FALSE(tree.contains(5));
    }

    SECTION("RR rotation on delete") {
        AVLTree tree;
        tree.insert(2);
        tree.insert(1);
        tree.insert(4);
        tree.insert(3);
        tree.insert(5);
        tree.remove(1);
        REQUIRE(tree.isValid());
        REQUIRE_FALSE(tree.contains(1));
    }

    SECTION("RL rotation on delete") {
        AVLTree tree;
        tree.insert(2);
        tree.insert(1);
        tree.insert(4);
        tree.insert(3);
        tree.remove(1);
        REQUIRE(tree.isValid());
        REQUIRE_FALSE(tree.contains(1));
    }
}

TEST_CASE("AVLTree rangeQuery returns correct slice in ascending order", "[avl_tree]") {
    AVLTree tree;
    for (long long i = 1; i <= 20; ++i) {
        tree.insert(i);
    }

    std::vector<long long> expected = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    REQUIRE(tree.rangeQuery(5, 15) == expected);

    REQUIRE(tree.rangeQuery(25, 30).empty());
    REQUIRE(tree.rangeQuery(-5, 0).empty());

    std::vector<long long> single = {10};
    REQUIRE(tree.rangeQuery(10, 10) == single);
    REQUIRE(tree.rangeQuery(15, 10).empty());
}

TEST_CASE("AVLTree random insertion and deletion stress test", "[avl_tree]") {
    AVLTree tree;
    std::mt19937_64 rng(1337);
    std::vector<long long> values;
    for (long long i = 1; i <= 200; ++i) {
        values.push_back(i);
    }
    std::shuffle(values.begin(), values.end(), rng);

    for (auto v : values) {
        tree.insert(v);
        REQUIRE(tree.isValid());
    }

    std::shuffle(values.begin(), values.end(), rng);
    for (std::size_t i = 0; i < 100; ++i) {
        tree.remove(values[i]);
        REQUIRE(tree.isValid());
        REQUIRE_FALSE(tree.contains(values[i]));
    }

    for (std::size_t i = 100; i < 200; ++i) {
        REQUIRE(tree.contains(values[i]));
    }
}
