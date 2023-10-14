#pragma once

#include "def.h"
#include "utils.h"
#include <iterator>
#include "string.h"

namespace bstr::core::containers {

template<typename T
 // TODO?: , usz Extent = std::dynamic_extent
>
struct span
{
	explicit constexpr span(T *first, usz count)
		: m_data(first)
		, m_size(count)
	{
	}
	
	explicit constexpr span(T *first, T* last)
		: m_data(first)
		, m_size(last - first)
	{
	}

	template< class SpanLike >
	constexpr span(SpanLike&& span_like) noexcept
		: m_data(span_like.data())
		, m_size(span_like.size())
	{
	}

	
	//template< class U, usz N >
	//constexpr span(std::array<U, N>& arr) noexcept
	//	: m_data(arr.data())
	//  , m_size(arr.size())
	//{
	//}
	
	//template< class U, usz N >
	//constexpr span(const std::array<U, N>& arr) noexcept
	//	: m_data(arr.data())
	//  , m_size(arr.size())
	//{
	//}
	
	//template< class U, usz N >
	//explicit constexpr span(const std::span<U, N>& source) noexcept
	//	: m_data(source.m_data)
	//	, m_size(source.m_size)
	//{
	//}
	
	constexpr span(const span& other) noexcept = default;

	T *begin() const { return m_data; }
	const  T* cbegin() const { return m_data; } 
	T *end() { return m_data + m_size; }
	const T *cend() const { return m_data + m_size; }

	constexpr auto rbegin() { return std::reverse_iterator(begin()); }
	constexpr auto crbegin() { return std::reverse_iterator(cbegin()); }
	constexpr auto rend() { return std::reverse_iterator(end()); }
	constexpr auto crend() { return std::reverse_iterator(cend()); }

	constexpr T &front() { return (*this)[0]; }
	constexpr const T &front() const { return (*this)[0]; }
	
	constexpr T &back() { return (*this)[m_size-1]; }
	constexpr const T &back() const { return (*this)[m_size-1]; }

	constexpr T& operator[](usz i) { ASSERT(i <= m_size, "span out of bounds"); return m_data[i]; }
	constexpr const T& operator[](usz i) const { ASSERT(i <= m_size, "span out of bounds"); return m_data[i]; }

	constexpr T *data() { return m_data; }
	constexpr const T *data() const { return m_data; }
	
	constexpr usz size() const { return m_size; }
	constexpr usz size_bytes() const { return m_size * sizeof(T); }
	
	constexpr usz empty() const { return m_size == 0; }
	
	constexpr span<T> first(usz count) const
	{
		return span<T> { m_data, count };
	}

	constexpr span<T> last(usz count) const
	{
		return span<T> { m_data + (m_size - count), count };
	}

	constexpr span<T> subspan( usz offset, usz count) const
	{
		return span<T> { m_data + offset, count };
	}

private:
	T* m_data{};
	usz m_size{};
};

}

using bstr::core::containers::span;