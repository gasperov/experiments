#include "ref_weak.hpp"

#include <unordered_map>
#include <mutex>

namespace {
    struct WeakRegImpl {
        std::mutex mtx;
        std::unordered_map<RefCountBase*, RefCount<WeakReg::WeakRef>> refs;

        static WeakRegImpl& inst() noexcept {
            static auto r = new WeakRegImpl;
            return *r;
        }
    };
}

RefCount<WeakReg::WeakRef> WeakReg::Add_(RefCountBase* obj) noexcept {
    auto& this_ = WeakRegImpl::inst();
    std::unique_lock lock1{this_.mtx};
    auto f = this_.refs.find(obj);
    if (f != this_.refs.end()) {
        return f->second;
    }
    obj->SetWeak();
    RefCount<WeakRef> w(new WeakRef{ obj });
    this_.refs[obj] = w;
    return std::move(w);
}

RefCount<RefCountBase> WeakReg::Get_(const RefCount<WeakRef> &t) noexcept {
    auto& this_ = WeakRegImpl::inst();
    auto obj = t.get_usafe();
    if (!obj) return nullptr;
    std::unique_lock lock1{this_.mtx};
    if (obj->ref == nullptr) {
        return nullptr;
    }
    auto f = this_.refs.find(obj->ref);
    if (f == this_.refs.end()) {
        return nullptr;
    }
    if (!obj->StillValid()) {
        return nullptr;
    }
    return obj->ref;
}

void WeakReg::Release(RefCountBase* obj) noexcept {
    auto& this_ = WeakRegImpl::inst();
    std::unique_lock lock1{this_.mtx};
    auto f = this_.refs.find(obj);
    if (f == this_.refs.end()) {
        return;
    }
    f->second->ref = nullptr;
    this_.refs.erase(f);
}

// assume this Release is called before last RefCountBase::Release is called
void ReleaseWeakPointerRef(RefCountBase* b) noexcept {
    WeakReg::Release(b);
}
