
#include "gtest/gtest.h"
#include "gc.h"

namespace example {
    struct BTree {
        static int numInstances;

        BTree* left = nullptr;
        BTree* right = nullptr;

        int data = 0;

        BTree() {
            ++numInstances;
        }

        ~BTree() {
            --numInstances;
        }
    };

    int BTree::numInstances = 0;

    void trace(gc::Tracer tracer, BTree* bt) {
        tracer(bt->left);
        tracer(bt->right);
    }
}

using namespace gc;
using example::BTree;

TEST(GCTest, collects_everything) {
    Arena arena;

    root<BTree> tree{arena};
    tree = gcnew<BTree>(arena);
    tree->left = gcnew<BTree>(arena);
    tree->right = gcnew<BTree>(arena);

    root<BTree> interloper{arena};
    interloper = tree->left;

    tree = nullptr;

    EXPECT_EQ(3, BTree::numInstances);

    arena.collect();

    EXPECT_EQ(1, BTree::numInstances);

    interloper = nullptr;

    arena.collect();

    EXPECT_EQ(0, BTree::numInstances);
}

TEST(GCTest, collects_cycle) {
    Arena arena;

    root<BTree> tree{arena};
    tree = gcnew<BTree>(arena);
    tree->left = gcnew<BTree>(arena);
    tree->right = gcnew<BTree>(arena);
    tree->left->left = tree.get();

    arena.collect();
    EXPECT_EQ(3, BTree::numInstances);
    arena.collect();
    EXPECT_EQ(3, BTree::numInstances);

    tree = nullptr;
    arena.collect();
    EXPECT_EQ(0, BTree::numInstances);
}
