#pragma once

#include <utility>

template<class F>
class ScopeGuard {
    F func;
public:
    ScopeGuard(F &&f)
        : func(std::forward<F&&>(f))
    {
    }
    ~ScopeGuard()
    {
        func();
    }
};

template<class F>
ScopeGuard(F &&) -> ScopeGuard<F>;
