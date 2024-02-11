#pragma once

#include <atomic>

struct RefCountBase;
struct WeakReg;
void ReleaseWeakPointerRef(RefCountBase* b) noexcept;

struct RefCountBase {
    RefCountBase(const RefCountBase& r) = delete;
    RefCountBase(RefCountBase&& r) = delete;
    RefCountBase& operator=(const RefCountBase& r) = delete;
    RefCountBase& operator=(RefCountBase&& r) = delete;

    RefCountBase() noexcept = default;
    virtual ~RefCountBase() = default;
    virtual void Release() noexcept {
        delete this;
    }
    void IncRefCount() noexcept {
        ref_count_.fetch_add(2);
    }
    void DecRefCount() noexcept {
        auto v = ref_count_.fetch_sub(2) - 2;
        if (v < 2) {
            if (v == 1) {
                ReleaseWeakPointerRef(this);
            }
            Release();
        }
    }
    void SetWeak() noexcept {
        auto v = ref_count_.load();
        while (((v&1) == 0) && !ref_count_.compare_exchange_strong(v,v|1)) {}
    }
    bool StillValid() noexcept {
        return ref_count_ >= 2;
    }
    unsigned int GetRefCount() const noexcept {
        return ref_count_ / 2;
    }
    bool IsWeak() const noexcept {
        return (ref_count_.load()&1) == 1;
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
            auto o = obj; obj = r.obj;
            if (o) o->DecRefCount();
            if (obj) obj->IncRefCount();
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
    inline T* get_usafe() const noexcept { return obj; }
protected:
friend WeakReg;
    T* obj;
};