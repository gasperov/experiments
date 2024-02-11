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
        RefCount<example> nu(nullptr);
        WeakReg::Add(nu);

        RefCount<example> ref(new example);
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";
        ref->call();
        ref.get_usafe()->call();
        std::cout << "exam ref: " << ref.GetRefCount() << "\n";

        {
            auto weak = WeakReg::Add(ref);
        }

        auto weak = WeakReg::Add(ref);
        {
            auto str = WeakReg::Get<example>(weak);
            std::cout << "exam ref: " << ref.GetRefCount() << "\n";
            str->call();
        }
        ref.get_usafe()->call();
        ref.Reset();
        std::cout << "... released ...\n";
        {
            auto str = WeakReg::Get<example>(weak);
            if (str) {
                std::cout << "wtf\n";
            }
        }
    }
    std::cout << "done\n";

    // Test case 1: Adding and getting a valid object
    {
        struct MyObject : RefCountBase {};
        RefCount<MyObject> obj(new MyObject);
        RefCount<WeakReg::WeakRef> weakRef = WeakReg::Add(obj);
        RefCount<MyObject> retrievedObj = WeakReg::Get<MyObject>(weakRef);
        assert(retrievedObj == obj);
    }

    // Test case 2: Adding and getting an invalid object
    {
        struct MyObject : RefCountBase {
            bool IsValid() const { return false; }
        };
        RefCount<MyObject> obj(new MyObject);
        RefCount<WeakReg::WeakRef> weakRef = WeakReg::Add(obj);
        RefCount<MyObject> retrievedObj = WeakReg::Get<MyObject>(weakRef);
        assert(retrievedObj);
    }

    // Test case 3: Adding and releasing an object
    {
        struct MyObject : RefCountBase {
            bool released = false;
            void delete_this() noexcept override { released = true; }
        };
        auto x = new MyObject;
        RefCount<MyObject> obj(x);
        RefCount<WeakReg::WeakRef> weakRef = WeakReg::Add(obj);
        obj.Reset();
        assert(x->released);
        RefCount<MyObject> retrievedObj = WeakReg::Get<MyObject>(weakRef);
        assert(retrievedObj == nullptr);
    }
    return 0;
}