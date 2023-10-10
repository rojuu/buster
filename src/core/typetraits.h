#include <type_traits>

namespace core {

template<class T>
using remove_reference_t = std::remove_reference_t<T>;

}