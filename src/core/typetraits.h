#include <type_traits>

template<class T>
using remove_reference_t = std::remove_reference_t<T>;
