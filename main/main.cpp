
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
    gray,
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
GCData* getGCData(T* t) {
    auto ofs = offsetof(HasGCData<T>, datum);
    HasGCData<T>* hgcd = reinterpret_cast<HasGCData<T>*>(
        reinterpret_cast<char*>(t) - ofs
    );
    return &hgcd->gc;
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
        ptr = rhs;
        return *this;
    }

    T& operator *() {
        return *static_cast<T*>(ptr);
    }

    T* operator ->() {
        return static_cast<T*>(ptr);
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
            [](Object* p) { delete static_cast<T*>(p); }
        },
        T{ std::forward<Args>(args)... }
    };
    arena.objects.insert(hgc);
    return &hgc->datum;
}

struct TraceHelper {
    Arena::Set& out;

    void operator()(Object* obj);
};

void TraceHelper::operator()(Object* obj) {
    out.insert(getGCData(obj));
}

void Arena::collect() {
    gray.clear();

    for (RootBase* root: roots) {
        if (root->ptr) {
            auto gcdata = getGCData(root->ptr);
            gray.insert(gcdata);
        }
    }

    while (!gray.empty()) {
        Object* o = *gray.begin();
        printf("Gray %p\n", o);
        gray.erase(gray.begin());

        auto helper = TraceHelper{gray};
        GCData* gcd = getGCData(o);
        gcd->color = currentBlack;
        gcd->visit(helper, o);
    }

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

    tree = nullptr;

    arena.collect();

    return 0;
}
