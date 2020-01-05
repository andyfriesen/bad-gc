
#pragma once

#include <cassert>
#include <utility>
#include <unordered_map>
#include <unordered_set>

namespace gc {

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

    inline GCData* getGCData(Object* p) {
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

    void dump(Arena& arena);

}
