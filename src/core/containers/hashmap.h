#pragma once

#include <unordered_map>

namespace bstr::core::containers {

template<class K, class V>
using hash_map = ::std::unordered_map<K, V>;

}

using bstr::core::containers::hash_map;