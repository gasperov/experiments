#include <cstddef>
#include <iostream>
#include <cassert>

#include "ref_weak.hpp"

namespace {
    int g_count = 0;

    struct example: RefCountBaseStrong {
        void call() {
            std::cout << "call: " << GetRefCount() << "\n";
        }
        example() {
            g_count++;
        }
        ~example() {
            g_count--;
        }
    };
}

int main() {
    {
        RefCount<example> nu(nullptr);

        RefCount<example> ref(new example);
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";
        ref->call();
        ref.GetUnsafe()->call();
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";

        auto weak = ref->GetWeak();
        {
            auto str = GetStrong<example>(weak);
            std::cout << "exam ref: " << ref.GetRefCount() << "\n";
            str->call();
        }
        ref.GetUnsafe()->call();
        ref.Reset();
        std::cout << "... released ...\n";
        {
            auto str = GetStrong<example>(weak);
            if (str) {
                std::cout << "wtf\n";
            }
        }
        assert(g_count == 0);
    }
    std::cout << "done\n";

    return 0;
}