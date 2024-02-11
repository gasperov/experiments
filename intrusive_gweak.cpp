#include <cstddef>
#include <atomic>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <mutex>

struct RefCountBase;
struct WeakReg;
void call_weak_destr(RefCountBase* b) noexcept;

struct RefCountBase {
    RefCountBase(const RefCountBase& r) = delete;
    RefCountBase(RefCountBase&& r) = delete;
    RefCountBase& operator=(const RefCountBase& r) = delete;
    RefCountBase& operator=(RefCountBase&& r) = delete;

    RefCountBase() noexcept = default;
    virtual ~RefCountBase() = default;

    void IncRefCount() noexcept {
        ref_count_.fetch_add(2);
    }
    void DecRefCount() noexcept {
        auto v = ref_count_.fetch_sub(2) - 2;
        if (v < 2) {
            if (v == 1) {
                call_weak_destr(this);
            }
            delete this;
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
    uint GetRefCount() const noexcept {
        return obj ? obj->GetRefCount() : 0;
    }
    inline T* get_usafe() const noexcept { return obj; }
protected:
friend WeakReg;
    T* obj;
};

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
void call_weak_destr(RefCountBase* b) noexcept {
    WeakReg::inst().Release(b);
}

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
    return 0;
}