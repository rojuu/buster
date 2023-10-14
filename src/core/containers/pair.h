#pragma once

namespace bstr::core::containers {

template<class T1, class T2>
struct pair {
	T1 first;
	T2 second;
};

}

using bstr::core::containers::pair;
