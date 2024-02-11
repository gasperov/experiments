#include <cstddef>
#include <iostream>
#include <cassert>

#include "ref_weak.hpp"

struct example: RefCountBase {
    void call() {
        std::cout << "call: " << GetRefCount() << "\n";
    }
};

int main() {
    {
        auto& reg = WeakReg::inst();
        RefCount<example> nu(nullptr);
        reg.Add(nu);

        RefCount<example> ref(new example);
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";
        ref->call();
        ref.get_usafe()->call();
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";

        {
            auto weak = reg.Add(ref);
        }

        auto weak = reg.Add(ref);
        {
            auto str = reg.Get<example>(weak);
            std::cout << "exam ref: " << ref.GetRefCount() << "\n";
            str->call();
        }
        ref.get_usafe()->call();
        ref.Reset();
        std::cout << "... released ...\n";
        {
            auto str = reg.Get<example>(weak);
            if (str) {
                std::cout << "wtf\n";
            }
        }
    }
    std::cout << "done\n";

    // Test case 1: Adding and getting a valid object
    {
        struct MyObject : RefCountBase {};
        WeakReg weakReg;
        RefCount<MyObject> obj(new MyObject);
        RefCount<WeakReg::WeakRef> weakRef = weakReg.Add(obj);
        RefCount<MyObject> retrievedObj = weakReg.Get<MyObject>(weakRef);
        assert(retrievedObj == obj);
    }

    // Test case 2: Adding and getting an invalid object
    {
        struct MyObject : RefCountBase {
            bool IsValid() const { return false; }
        };
        WeakReg weakReg;
        RefCount<MyObject> obj(new MyObject);
        RefCount<WeakReg::WeakRef> weakRef = weakReg.Add(obj);
        RefCount<MyObject> retrievedObj = weakReg.Get<MyObject>(weakRef);
        assert(retrievedObj);
    }

    // Test case 3: Adding and releasing an object
    {
        struct MyObject : RefCountBase {
            bool released = false;
            void Release() noexcept override { released = true; }
        };
        WeakReg weakReg;
        RefCount<MyObject> obj(new MyObject);
        RefCount<WeakReg::WeakRef> weakRef = weakReg.Add(obj);
        weakReg.Release(obj.get_usafe());
        assert(obj->released);
        RefCount<MyObject> retrievedObj = weakReg.Get<MyObject>(weakRef);
        assert(retrievedObj);
    }
    return 0;
}