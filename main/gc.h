
#pragma once

#include <cassert>
#include <utility>
#include <unordered_set>

namespace gc {

    struct Arena;
    struct TraceHelper;

    // Always used as Object*.  Points into a HasGCData<T>
    using Object = void;

    using Tracer = TraceHelper&;
    using Visitor = void (*)(Tracer, Object*);
    using Destructor = void (*)(Object*);

    struct GCData {
        Visitor visit = nullptr;
        Destructor destroy = nullptr;

        bool color;
        GCData* next;
        GCData* prev;
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
        Arena();
        Arena(const Arena&) = delete;
        void operator=(const Arena&) = delete;

        void collect();

        template <typename T, typename... Args>
        T* gcnew(Args&&... args) {
            auto hgc = new HasGCData<T>{
                GCData {
                    [](Tracer t, Object* p) { trace(t, static_cast<T*>(p)); },
                    [](Object* p) { delete hasGCData<T>(static_cast<T*>(p)); }
                },
                T{ std::forward<Args>(args)... }
            };

            if (whiteHead)
                whiteHead->prev = &hgc->gc;

            hgc->gc.color = currentWhite;
            hgc->gc.next = whiteHead;
            hgc->gc.prev = nullptr;
            whiteHead = &hgc->gc;

            return &hgc->datum;
        }

    public:
        using Set = std::unordered_set<Object*>;

        bool currentWhite = false;

        GCData* blackHead = nullptr;
        GCData* whiteHead = nullptr;

        std::unordered_set<RootBase*> roots;
    };

    struct TraceHelper {
        Arena::Set& out;

        bool currentWhite;

        void operator()(Object* obj);
    };

    void dump(Arena& arena);

}
