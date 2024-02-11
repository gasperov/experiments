#pragma once

#include <mutex>
#include "ref.hpp"

struct RefCountBaseStrong : RefCountBase {
    RefCountBaseStrong() noexcept {
        weak_ref_ = new WeakRef(this);
    }

    void DeleteThisObject() noexcept override final {
        weak_ref_.GetUnsafe()->Release();
        delete this;
    }

    struct WeakRef : RefCountBase {
        RefCountBaseStrong* GetStrong() noexcept {
            while(true) {
                std::unique_lock lock1{mtx};
                if (ref == nullptr) {
                    return nullptr;
                }
                if (ref->IncIfNotZero()) {
                    return ref;
                }
            }
            return nullptr;
        }
    private:
        friend RefCountBaseStrong;
        WeakRef(RefCountBaseStrong* t) : ref(t) {}
        RefCountBaseStrong* ref;
        std::mutex mtx;

        void Release() {
            std::unique_lock lock1{mtx};
            ref = nullptr;
        }
    };

    RefCount<WeakRef> GetWeak() const noexcept {
        return weak_ref_;
    }
       
private:
    friend WeakRef;
    RefCount<WeakRef> weak_ref_;
};


template<typename T>
static RefCount<T> GetStrong(const RefCount<RefCountBaseStrong::WeakRef>& t) noexcept {
    auto ref = t.GetUnsafe()->GetStrong();
    RefCount<T> ret;
    ret.obj = static_cast<T*>(ref);
    return std::move(ret);
}
