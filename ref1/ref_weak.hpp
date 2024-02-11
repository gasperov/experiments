#pragma once

#include "ref.hpp"
#include <unordered_map>
#include <mutex>

struct WeakReg {
    struct WeakRef : RefCountBase {
        friend WeakReg;
    private:
        WeakRef(RefCountBase *t) : ref(t) {}
        RefCountBase* ref;
    };

    // assume this Release is called before last RefCountBase::Release is called
    void Release(RefCountBase *obj) noexcept {
        std::unique_lock lock1{mtx};
        auto f = refs.find(obj);
        if (f == refs.end()) {
            return;
        }
        f->second->ref = nullptr;
        refs.erase(f);
    }

    template<typename T>
    RefCount<T> Get(const RefCount<WeakRef> &t) noexcept {
        auto r = Get_(t);
        RefCount<T> ret;
        ret.obj = static_cast<T*>(r.obj);
        r.obj = nullptr;
        return std::move(ret);
    }

    template<typename T>
    RefCount<WeakRef> Add(RefCount<T> &t) noexcept {
        if (!t) return nullptr;
        return Add_(t.obj);
    }

    static WeakReg& inst() noexcept {
        static auto r = new WeakReg;
        return *r;
    }
private:
    std::mutex mtx;
    std::unordered_map<RefCountBase*, RefCount<WeakRef>> refs;
    
    RefCount<WeakRef> Add_(RefCountBase* obj) noexcept {
        std::unique_lock lock1{mtx};
        auto f = refs.find(obj);
        if (f != refs.end()) {
            return f->second;
        }
        obj->SetWeak();
        RefCount<WeakRef> w(new WeakRef{obj});
        refs[obj] = w;
        return std::move(w);
    }

    RefCount<RefCountBase> Get_(const RefCount<WeakRef> &t) noexcept {
        if (!t.obj) return nullptr;
        std::unique_lock lock1{mtx};
        if (t.obj->ref == nullptr) {
            return nullptr;
        }
        auto f = refs.find(t.obj->ref);
        if (f == refs.end()) {
            return nullptr;
        }
        if (!t.obj->StillValid()) {
            return nullptr;
        }
        return t.obj->ref;
    }
};
