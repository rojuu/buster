#pragma once

#include "containers/span.h"
#include "containers/string.h"
#include "containers/vector.h"
#include "utils.h"

namespace bstr::core {

template<typename CharT>
class basic_string_builder {
public:
	void reserve(usz amount) {
		m_str_buf.resize(amount+1);
		zero_end_of_buffer();
	}
	void append(const basic_string<CharT>& str) { append(span<const CharT> { str.data(), str.size() }); }
	void append(const CharT* str) { span<const CharT>{ str, strlen(str); } }
	void append(CharT ch) {
		char buf[1] = { 0 };
		buf[0] = ch;
		append(span<const CharT>{buf, 1});
	}
	void append(span<const CharT> str) {
		ensure_capacity(str.size());
		usz i = m_str_size;
		for (CharT c : str) {
			m_str_buf[i++] = c;
		}
		m_str_size += str.size();
	}
	void ensure_capacity(usz amount) {
		if (m_str_buf.size() < m_str_size + amount + 1) {
			usz needed_amount = m_str_size + amount;
			usz double_reserve = m_str_buf.size() * 2;
			if (double_reserve == 0) double_reserve = 1;
			usz reserve_amount = max(double_reserve, needed_amount);
			reserve(reserve_amount);
		}
	}
	void resize(usz new_size) {
		m_str_size = new_size;
		zero_end_of_buffer();
	}
	CharT* c_str() {
		return m_str_buf.data(); 
	}
	basic_string<CharT> str() {
		return basic_string<CharT>(m_str_buf.data(), m_str_size);
	}

	usz size() const { return m_str_size; }
	CharT operator[](usz i) const {
		ASSERT(i <= m_str_size, "str_builder out of bounds");
		return m_str_buf[i];
	}
private:
	void zero_end_of_buffer() {
		if (m_str_buf.empty()) return;
		ASSERT(m_str_size <= m_str_buf.size(), "string bigger than underlying buffer");
		memset(m_str_buf.data() + m_str_size, 0, (m_str_buf.size() - m_str_size) * sizeof(CharT));
	}
private:
	usz m_str_size = 0;
	//TODO: Use something where we won't have double capacity (e.g. a custom buffer class)
	vector<CharT> m_str_buf;
};

using string_builder = basic_string_builder<char>;

}

using bstr::core::string_builder;