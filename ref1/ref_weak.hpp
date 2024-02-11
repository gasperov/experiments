#pragma once

#include "ref.hpp"

struct WeakReg {
    struct WeakRef : RefCountBase { 
        friend WeakReg;
    private:
        WeakRef(RefCountBase *t) : ref(t) {}
        RefCountBase* ref;
    };

    template<typename T>
    static RefCount<WeakRef> Add(RefCount<T> &t) noexcept {
        if (!t) return nullptr;
        return Add_(t.obj);
    }    

    template<typename T>
    static RefCount<T> Get(const RefCount<WeakRef> &t) noexcept {
        auto r = Get_(t);
        RefCount<T> ret;
        ret.obj = static_cast<T*>(r.obj);
        r.obj = nullptr;
        return std::move(ret);
    }

    static void Release(RefCountBase* obj) noexcept;
    
private:
    WeakReg() = delete;
    static RefCount<WeakRef> Add_(RefCountBase* obj) noexcept;
    static RefCount<RefCountBase> Get_(const RefCount<WeakRef> &t) noexcept;
};