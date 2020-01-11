
#include "gc.h"

namespace gc {

    RootBase::RootBase(Arena& arena, Object* ptr)
        : arena(arena)
        , ptr(ptr)
    {
        arena.roots.insert(this);
    }

    RootBase::~RootBase() {
        arena.roots.erase(this);
    }

    void TraceHelper::operator()(Object* obj) {
        if (obj) {
            out.insert(obj);
        }
    }

    void dump(Arena& arena) {
        printf("dump\n");

        auto gross = [](GCData* gcd) {
            return reinterpret_cast<HasGCData<int>*>(gcd);
        };

        GCData* p = arena.blackHead;
        while (p) {
            printf("\t%p black\n", gross(p));
            p = p->next;
        }

        p = arena.whiteHead;
        while (p) {
            printf("\t%p white\n", gross(p));
            p = p->next;
        }
    }

    Arena::Arena()
        : currentWhite(false)
    {}

    void Arena::collect() {
        Set gray;

        for (RootBase* root: roots) {
            if (root->ptr) {
                gray.insert(root->ptr);
            }
        }

        while (!gray.empty()) {
            Object* o = *gray.begin();
            gray.erase(gray.begin());

            GCData* gcd = getGCData(o);

            if (gcd->color == !currentWhite)
                continue;

            {
                // remove from white set
                GCData* prev = gcd->prev;
                GCData* next = gcd->next;

                if (prev)
                    prev->next = next;
                else
                    whiteHead = next;

                if (next)
                    next->prev = prev;
            }

            {
                // add to black set
                if (blackHead)
                    blackHead->prev = gcd;

                gcd->prev = nullptr;
                gcd->next = blackHead;
                blackHead = gcd;
            }

            auto helper = TraceHelper{gray, currentWhite};
            gcd->color = !currentWhite;
            gcd->visit(helper, o);
        }

        GCData* gcd = whiteHead;
        while (gcd) {
            auto next = gcd->next;
            // FIXME
            HasGCData<int>* hgcd = reinterpret_cast<HasGCData<int>*>(gcd);
            gcd->destroy(&hgcd->datum);

            gcd = next;
        }

        currentWhite = !currentWhite;
        whiteHead = blackHead;
        blackHead = nullptr;
    }

}
