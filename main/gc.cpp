
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
        for (auto o: *arena.blackSet) {
            printf("\t%p black\n", o);
        }
        for (auto o: *arena.whiteSet) {
            printf("\t%p white\n", o);
        }
    }

    Arena::Arena()
        : blackSet(&one)
        , whiteSet(&two)
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

            if (blackSet->count(o))
                continue;

            blackSet->insert(o);
            whiteSet->erase(o);

            auto helper = TraceHelper{gray, *blackSet, *whiteSet};
            GCData* gcd = getGCData(o);

            gcd->visit(helper, o);
        }

        for (Object* o: *whiteSet) {
            GCData* gcd = getGCData(o);
            gcd->destroy(o);
        }

        std::swap(blackSet, whiteSet);
    }

}
