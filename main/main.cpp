
#include <iostream>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include "gc.h"

using gc::Arena;
using gc::dump;
using gc::root;

namespace example {
    struct BTree {
        BTree* left = nullptr;
        BTree* right = nullptr;

        int data = 0;

        BTree() {
            printf("BTree() %p\n", this);
        }

        ~BTree() {
            printf("~BTree() %p\n", this);
        }
    };

    void trace(gc::Tracer tracer, BTree* bt) {
        printf("Trace %p\n", bt);
        tracer(bt->left);
        tracer(bt->right);
    }
}

int main() {
    using example::BTree;

    Arena arena;

    root<BTree> tree{arena};
    tree = arena.gcnew<BTree>();
    tree->left = arena.gcnew<BTree>();
    tree->right = arena.gcnew<BTree>();

    // tree->right->left = tree.get();
    // tree->left->left = tree.get();

    root<BTree> interloper{arena};
    interloper = tree->left;

    tree = nullptr;

    dump(arena);

    arena.collect();

    printf("hoh\n");
    interloper = nullptr;

    arena.collect();

    return 0;
}
