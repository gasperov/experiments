#include "ref_weak.hpp"

void ReleaseWeakPointerRef(RefCountBase* b) noexcept {
    WeakReg::inst().Release(b);
}
