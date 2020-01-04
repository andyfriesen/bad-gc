
#include <iostream>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <unordered_set>

struct Arena;
struct TraceHelper;

// Always used as Object*.  Points into a HasGCData<T>
using Object = void;

using Tracer = TraceHelper&;
using Visitor = void (*)(Tracer, Object*);
using Destructor = void (*)(Object*);

enum Color {
    black,
    white
};

struct GCData {
    Color color;
    Visitor visit = nullptr;
    Destructor destroy = nullptr;
};

template <typename T>
struct HasGCData {
    GCData gc;
    alignas(8) T datum;
};

template <typename T>
HasGCData<T>* hasGCData(T* t) {
    if (!t) {
        return nullptr;
    }

    auto ofs = offsetof(HasGCData<T>, datum);
    return reinterpret_cast<HasGCData<T>*>(
        reinterpret_cast<char*>(t) - ofs
    );
}

template <typename T>
GCData* getGCData(T* t) {
    auto hgcd = hasGCData(t);
    if (hgcd) {
        return &hgcd->gc;
    } else {
        return nullptr;
    }
}

GCData* getGCData(Object* p) {
    return getGCData(static_cast<int*>(p));
}

struct Arena;

struct RootBase {
    Arena& arena;
    Object* ptr = nullptr;

    explicit RootBase(Arena& arena, Object* ptr);
    ~RootBase();
};

template <typename T>
struct root : RootBase {
    explicit root(Arena& arena, T* ptr=nullptr)
        : RootBase(arena, ptr)
    {}

    root& operator=(T* rhs) {
        printf("Assign %p = %p\n", this, rhs);
        ptr = rhs;
        return *this;
    }

    T* get() {
        return static_cast<T*>(ptr);
    }

    T& operator *() {
        return *get();
    }

    T* operator ->() {
        return get();
    }
};

struct Arena {

    void collect();

public:
    using Set = std::unordered_set<Object*>;

    Color currentWhite = white;
    Color currentBlack = black;

    Set objects;
    Set gray;

    std::unordered_set<RootBase*> roots;
};

RootBase::RootBase(Arena& arena, Object* ptr)
    : arena(arena)
    , ptr(ptr)
{
    arena.roots.insert(this);
}

RootBase::~RootBase() {
    arena.roots.erase(this);
}

template <typename T, typename... Args>
T* gcnew(Arena& arena, Args&&... args) {
    auto hgc = new HasGCData<T>{
        GCData {
            arena.currentWhite,
            [](Tracer t, Object* p) { trace(t, static_cast<T*>(p)); },
            [](Object* p) { delete hasGCData<T>(static_cast<T*>(p)); }
        },
        T{ std::forward<Args>(args)... }
    };
    arena.objects.insert(&hgc->datum);
    return &hgc->datum;
}

struct TraceHelper {
    Arena::Set& out;
    Color currentWhite;
    Color currentBlack;

    void operator()(Object* obj);
};

void dump(Arena& arena) {
    printf("dump\tblack = %d\n", arena.currentBlack);
    for (auto o: arena.objects) {
        auto gcd = getGCData(o);
        printf("\t%p %d\n", o, gcd->color);
    }
}

void TraceHelper::operator()(Object* obj) {
    if (!obj) {
        return;
    }

    auto gcd = getGCData(obj);
    if (gcd->color == currentBlack) {
        return;
    }

    gcd->color = currentWhite;
    out.insert(obj);
}

void Arena::collect() {
    gray.clear();

    for (RootBase* root: roots) {
        if (root->ptr) {
            gray.insert(root->ptr);
        }
    }

    while (!gray.empty()) {
        Object* o = *gray.begin();
        gray.erase(gray.begin());

        auto helper = TraceHelper{gray, currentWhite, currentBlack};
        GCData* gcd = getGCData(o);

        gcd->visit(helper, o);
    }

    dump(*this);

    auto it = objects.begin();
    while (it != objects.end()) {
        Object* o = *it;
        GCData* gcd = getGCData(o);
        auto it2 = it;
        ++it;

        if (gcd->color == currentWhite) {
            gcd->destroy(o);
            objects.erase(it2);
        }
    }

    std::swap(currentBlack, currentWhite);
}

////////////////////////////////////////////////////////////////////////////////

namespace example {
    struct BTree {
        GCData gc;

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

    GCData* gcdata(BTree* bt) {
        return &bt->gc;
    }

    void trace(Tracer tracer, BTree* bt) {
        printf("Trace %p\n", bt);
        tracer(bt->left);
        tracer(bt->right);
    }
}

int main() {
    using example::BTree;

    Arena arena;

    root<BTree> tree{arena};
    tree = gcnew<BTree>(arena);
    tree->left = gcnew<BTree>(arena);
    tree->right = gcnew<BTree>(arena);

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
