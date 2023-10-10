#pragma once

#include <memory>
#include "utils.h"

namespace core {

template<typename T>
using own_ptr = std::unique_ptr<T>;

template<typename T, typename ...Args>
own_ptr<T> make_owned(Args&& ...args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

}