
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
        printf("dump\tblack = %d\n", arena.currentBlack);
        for (auto o: arena.objects) {
            auto gcd = getGCData(o);
            printf("\t%p %d\n", o, gcd->color);
        }
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

            if (gcd->color == currentBlack)
                continue;

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

        std::swap(currentBlack, currentWhite);
    }

}
