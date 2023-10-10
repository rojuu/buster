#pragma once

#include <unordered_map>

namespace core::containers {

template<class K, class V>
using HashMap = std::unordered_map<K, V>;

}

using core::containers::HashMap;