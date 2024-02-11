#pragma once

#include <atomic>

struct WeakReg;

struct RefCountBase {
    RefCountBase(const RefCountBase& r) = delete;
    RefCountBase(RefCountBase&& r) = delete;
    RefCountBase& operator=(const RefCountBase& r) = delete;
    RefCountBase& operator=(RefCountBase&& r) = delete;

    RefCountBase() noexcept = default;
    virtual ~RefCountBase() = default;
    virtual void DeleteThisObject() noexcept {
        delete this;
    }
    void IncRefCount() noexcept {
        ref_count_.fetch_add(1);
    }
    void DecRefCount() noexcept {
        auto v = ref_count_.fetch_sub(1) - 1;
        if (v == 0) {
            DeleteThisObject();
        }
    }
    bool IncIfNotZero() noexcept {
        for(int i = 0; i < 10; i++) {
            auto v = ref_count_.load();
            if (v == 0) {
                return false;
            }
            if (ref_count_.compare_exchange_weak(v, v + 1)) {
                return true;
            }
        }
        return false;
    }
    unsigned int GetRefCount() const noexcept {
        return ref_count_;
    }
private:
    std::atomic_uint ref_count_{0};
};

// RefCount is not thread safe
// use mutex when changing RefCount from multiple threads
// operator -> does not check for null

template<typename T>
struct RefCount {
    RefCount() noexcept : obj(0) {}
    RefCount(T *t) noexcept : obj(t) { if (obj) obj->IncRefCount(); }
    RefCount(RefCount &&r) noexcept : obj(r.obj) { r.obj = 0;}
    RefCount(const RefCount &r) noexcept : obj(r.obj) { if (obj) obj->IncRefCount();}
    
    ~RefCount() { Reset(); }
    RefCount& operator=(RefCount&& r) noexcept {
        if (r.obj != obj) {
            auto o = obj; obj = r.obj; r.obj = 0;
            if (o) o->DecRefCount();
        }
        return *this;
    }
    RefCount& operator=(const RefCount& r) noexcept {
        if (r.obj != obj) {
            Reset();
            if (r.obj) r.obj->IncRefCount();
            obj = r.obj;
        }
        return *this;
    }
    void Reset() {
        auto o = obj; obj = 0;
        if (o) o->DecRefCount();
    }
    struct CallAccess {
        T* operator->() const noexcept { return call;}
        CallAccess(T *t) noexcept : call(t) {
            call->IncRefCount();
        }
        ~CallAccess() noexcept {
            call->DecRefCount();
        }
    private:
        T* call;
    };
    CallAccess operator->() const noexcept {
        // TODO: check obj
        return obj;
    }
    inline operator bool() const {
        return obj != nullptr;
    }
    unsigned int GetRefCount() const noexcept {
        return obj ? obj->GetRefCount() : 0;
    }
    inline bool operator==(const RefCount& other) const {
        return obj == other.obj;
    }    
    inline T* GetUnsafe() const noexcept { return obj; }

    T* obj;
};