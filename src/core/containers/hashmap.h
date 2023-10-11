#pragma once

#include <unordered_map>

namespace core::containers {

template<class K, class V>
using hash_map = std::unordered_map<K, V>;

}

using core::containers::hash_map;