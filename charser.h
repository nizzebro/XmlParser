#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <functional>

namespace char_parsers
{

using UChar = uint_least32_t;

/// \brief Common character-checking unary predicate functors.

struct eq
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr eq(CharT c) noexcept: ch(static_cast<std::make_unsigned_t<CharT> >(c)){}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{ return static_cast<std::make_unsigned_t<CharT> >(c) == ch; }
    UChar ch;
};

struct gt
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr gt(CharT c) noexcept: ch(static_cast<std::make_unsigned_t<CharT> >(c)){}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{ return static_cast<std::make_unsigned_t<CharT> >(c) > ch; }
    UChar ch;
};

struct lt
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr lt(CharT c) noexcept: ch(static_cast<std::make_unsigned_t<CharT> >(c)){}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{ return static_cast<std::make_unsigned_t<CharT>>(c) < ch; }
    UChar ch;
};

struct gt_eq
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr gt_eq(CharT c) noexcept: ch(static_cast<std::make_unsigned_t<CharT> >(c)){}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept 
	{ return static_cast<std::make_unsigned_t<CharT>>(c) >= ch; }
    UChar ch;
};

struct lt_eq
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr lt_eq(CharT c) noexcept: ch(static_cast<std::make_unsigned_t<CharT> >(c)){}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{ return static_cast<std::make_unsigned_t<CharT>>(c) <= ch; }
    UChar ch;
};


struct of_range
{
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr of_range(CharT first, CharT last) noexcept :
		ch1(static_cast<std::make_unsigned_t<CharT> >(first)),
		ch2(static_cast<std::make_unsigned_t<CharT> >(last))
	{
	}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{ 
		return static_cast<std::make_unsigned_t<CharT> >(c) >= ch1 && 
			static_cast<std::make_unsigned_t<CharT> >(c) <= ch2;
	}
	UChar ch1;
	UChar ch2;
};


template<typename T>
struct any_of
{
	constexpr any_of(const std::initializer_list<T>& l) noexcept : begin(l.begin()), end(l.end()) {}
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{
		
		for (const T* p = begin; p != end; ++p)
		{
			if (check<std::is_integral_v<T> >
				(static_cast<std::make_unsigned_t<CharT>>(c), *p)) return true;
		}
		return false;
	}
	const T* begin;
	const T* end;

	template<bool IsCharT = false>
	constexpr static bool check(UChar c, T t) noexcept {return (t(c));}
	template<>
	constexpr static bool check<true>(UChar c, T t) noexcept 
	{ 
		return (c == static_cast<std::make_unsigned_t<T> >(t));
	}

};

template<class T>
any_of(std::initializer_list<T> l)->any_of<T>; // deduction guide

template<typename T>
struct all_of: any_of<T>
{
	using any_of<T>::any_of;
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (CharT c) noexcept
	{
		for (const T* p = any_of<T>::begin; p != any_of<T>::end; ++p)
		{
			if (!any_of<T>::check<std::is_integral_v<T> >
				(static_cast<std::make_unsigned_t<CharT>>(c),	*p)) return false;
		}
		return true;
	}
};
template<class T>
all_of(std::initializer_list<T> l)->all_of<T>; // deduction guide

template<typename T>
struct not_of : any_of<T>
{
	using any_of<T>::any_of;
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	constexpr bool operator() (T c) noexcept
	{
		for (const T* p = any_of<T>::begin; p != any_of<T>::end; ++p)
		{
			if (any_of<T>::check<std::is_integral_v<T> >
				(static_cast<std::make_unsigned_t<CharT>>(c),	*p)) return false;
		}
		return true;
	}
};
template<class T>
not_of(std::initializer_list<T> l)->not_of<T>; // deduction guide


/// \brief Character-checking unary predicate class.
/// \detail A wrapper to std::function; provides ability for both 
/// predicates, single characters and braced lists of characters 
/// to be passed to character-seacrhing functions.
struct predicate : public std::function<bool(UChar)>
{
	using std::function<bool(UChar)>::function;
	using std::function<bool(UChar)>::operator=;
	/// Constructs a predicate to check if a char is equal to 
	/// that passed to this constructor.
	template<typename CharT, std::enable_if_t<std::is_integral_v<CharT>, bool> = true>
	predicate(CharT ch) : std::function<bool(UChar)>(eq(ch)) {}
	/// Constructs a predicate to check a char against either
	/// list of characters or a list of predicates.
	/// \param T Either UChar or a predicate which takes UChar 
	/// and returns boolean.
	template<typename T>
	predicate(const std::initializer_list<T>& list) :
		std::function<bool(UChar)>(any_of<T>(list)) {}
};

/// \brief Charcter encoding constants.
enum struct Encoding
{
	A, UTF8, UTF16
};

/// \brief Common base class for charser implementations.
/// \tparam Character type.
template<typename CharT>
class charser_base 
{
	
protected:

	const CharT* _p1;
	const CharT* _p2;

public:
	using char_type = CharT;
	using self_type = charser_base<char_type>;
	using stl_string_view = std::basic_string_view<char_type, std::char_traits<char_type> >;

	charser_base() noexcept : _p1(0), _p2(0) {}

	charser_base(const char_type* begin, const char_type* end) noexcept :
		_p1(begin), _p2(end)
	{
	}

	void setBegin(const char_type* p) noexcept { _p1 = p; }

	void setEnd(const char_type* p) noexcept { _p2 = p; }

	void assign(const char_type* begin, const char_type* end) noexcept
	{
		setBegin(begin); setEnd(end);
	}

	void clear() noexcept { assign(0, 0); }

	operator stl_string_view() const noexcept
	{
		return stl_string_view(_p1, _p2 - _p1);
	}

	friend std::basic_ostream<char_type, std::char_traits<char_type> >&
		operator<<(std::basic_ostream<char_type, std::char_traits<char_type> >&
			os, const charser_base<char_type>& it)
	{
		os << static_cast<const stl_string_view&>(it);
		return os;
	}

	std::size_t size() const noexcept { return _p2 - _p1; }

	bool empty() const noexcept { return _p2 == _p1; }

	operator bool() const noexcept { return !empty(); }

	constexpr const char_type* begin() const noexcept { return _p1; }

	constexpr const char_type* end() const noexcept { return  _p2; }

	constexpr bool operator==(const stl_string_view& s) const noexcept
	{
		return  static_cast<const stl_string_view&>(*this) == s;
	}

	constexpr bool operator!=(self_type s) const noexcept
	{
		return  !operator==(s);
	}

	friend bool operator==(const stl_string_view& s, const charser_base<char_type>& it)  noexcept
	{
		return it == s;
	}
	friend bool operator!=(const stl_string_view& s, const charser_base<char_type>& it)  noexcept
	{
		return it != s;
	}

};

/// \brief  Implementation of forward charser methods for fixed-size encodings.
/// \tparam Character type.
template<typename CharT>
class charser_base_A: public charser_base<CharT>
{
protected:
	using charser_base<CharT>::_p1;
	using charser_base<CharT>::_p2;

public:
	using typename charser_base<CharT>::char_type;
	using typename charser_base<char_type>::stl_string_view;

	charser_base_A() :charser_base<char_type>() {}
	charser_base_A(const char_type* begin, const char_type* end) :
		charser_base<char_type>(begin, end) {}

	using charser_base<char_type>::size;
	using charser_base<char_type>::empty;

	const char_type* get() const noexcept { return _p1; }

	std::size_t length() const noexcept	{return this->size();}

	void unchecked_skip(std::size_t n = 1) noexcept { while(n) { ++_p1; --n;};}

	bool skip(std::size_t n = 1) noexcept
	{ 
		while (n)
		{ 
			if (empty()) return false; 
			++_p1; 
			--n; 
		}
		return true;
	}

	UChar peek() const noexcept { return !empty()? static_cast<std::uint8_t>(*_p1) : 0; }

	UChar getc() noexcept
	{
		if (!empty()) return static_cast<std::uint8_t>(*_p1++);
		return 0;
	}

	bool skip_if(predicate q) noexcept
	{
		auto b = startsWith(q);
		if(b) ++_p1;
		return b;
	}

	std::size_t skip_while(stl_string_view s) noexcept
	{
		std::size_t n = 0;
		std::size_t ncp = std::min(size(), s.size());
		for (const char_type* p = s.data(); ncp && (*p == *_p1); ++p, ++_p1, ++n, --ncp) {}
		return n;
	}

	
	bool skip_if(stl_string_view s) noexcept
	{
		auto b = startsWith(s);
		if (b) _p1 += s.size();
		return b; 
	}

	UChar seek(predicate q, bool skipFound = false) noexcept
	{
		for (auto n = this->size(); n; ++_p1, --n)
		{
			auto c = *_p1;
			if (q(c))
			{
				if (skipFound) ++_p1;
				return c;
			}
		}
		return  0;
	}

	UChar seek_span(predicate q, bool skipFound, charser_base<char_type>& span,
		bool appendFound = false) noexcept
	{
		auto p1 = _p1;
		auto c = seek(q, false);
		auto p2 = _p1;
		if (c)
		{	
			if (skipFound) ++_p1;
			if (appendFound) ++p2;
		}
		span.assign(p1, p2);
		return c;
	}

	bool seek(stl_string_view s, bool skipFound = false) noexcept
	{
		do {
			if (skip_if(s))
			{
				if (!skipFound) _p1 -= s.size();
				return true;
			}	
		} while (skip());
		return false;
	}

	
	bool seek_span(stl_string_view s, bool skipFound, charser_base<char_type>& span,
		bool appendFound = false) noexcept
	{
		auto p1 = get();
		bool found = seek(s, false);
		auto p2 = get();
		if (found)
		{
			if (skipFound) _p1 += s.size();
			if (appendFound) p2 += s.size();
		}
		span.assign(p1, p2);
		return found;
	}

	
	bool startsWith(predicate q) const noexcept	
	{ 
		return (!empty() && q(*_p1));
	}

	bool startsWith(stl_string_view s) const noexcept
	{
		auto ncp = s.size();
		if (ncp <= size())
		{
			const char_type* pStr = s.data();
			for (const char_type* p = _p1;	ncp; ++pStr, ++p, --ncp)
			{
				if (*pStr != *p) return false;
			}
			return true;
		}
		return false;
	}

	bool endsWith(predicate q) const noexcept
	{
		if (!empty()) return q(*(_p2 - 1));
		return false;
	}


	bool endsWith(stl_string_view s) const noexcept
	{
		auto n = s.size();
		if (n > size()) return false;
		const char_type* pStr = s.data() + s.size();
		for (const char_type* p = _p2; n; --n)
		{
			if(*--pStr != *--p) return false;
		}
		return true;
	}

	bool contains(predicate q) const noexcept
	{
		charser_base_A<char_type> it(*this);
		return(it.seek(q));
	}

	bool contains(stl_string_view s) const noexcept
	{
		charser_base_A<char_type> it(*this);
		return(it.seek(s));
	}

	void trimLeading(predicate q) noexcept	{while (skip_if(q)) {};}

	void trimTrailing(predicate q) noexcept	{while (endsWith(q)) { --_p2; }	}

	///@}

};

/// \brief Implementation forward charser methods for UTF-8 (no error checks).
/// \tparam Character type.
template<typename CharT>
class charser_base_UTF8 : public charser_base_A<CharT>
{
	using charser_base<CharT>::_p1;
	using charser_base<CharT>::_p2;


	constexpr static bool is_trailing(CharT c) noexcept
	{
		return (c & 0xC0) == 0x80;
	}

	constexpr static bool is_leading(CharT c) noexcept
	{
		return (c & 0xC0) == 0xC0;
	}

public:
	using typename charser_base<CharT>::char_type;
	using typename charser_base<char_type>::stl_string_view;

	charser_base_UTF8() :charser_base_A<char_type>() {}
	charser_base_UTF8(const char_type* begin, const char_type* end) :
		charser_base_A<char_type>(begin, end) {}

	using charser_base<char_type>::empty;
	using charser_base<char_type>::size;

	std::size_t length() const noexcept
	{
		std::size_t n = 0;
		std::size_t ncp = size();
		for (const char_type* p = _p1; ncp; --ncp)
		{
			if (!is_trailing(*p++)) ++n;
		}
		return n;
	}

	void unchecked_skip(std::size_t n = 1) noexcept
	{
		while (n)
		{
			while (is_trailing(*++_p1)) {} 
			--n;
		}
	}

	bool skip(std::size_t n = 1) noexcept
	{
		std::size_t ncp = size();
		while (ncp)
		{
			if (!is_trailing(*_p1))
			{
				if(--n == (std::size_t) - 1) return true;
			}
			++_p1;
			--ncp;
		}
		return --n == (std::size_t) -1;
	}

	UChar peek() const noexcept
	{ 
		auto p = _p1;
		auto c = getc();
		_p1 = p;
		return c;
	}

	UChar getc() noexcept
	{
		auto ncp = size();
		if (ncp)
		{
			UChar c = static_cast<uint8_t>(*_p1++);
			if (c < 0x80) return c;
			if (c < 0xC0) // extra trailing codes; ignore and skip 
			{
				do { ++_p1; } while (--ncp && is_trailing(*_p1));
			}
			if (c < 0xE0) // 2 bytes
			{
				c = (c << 6) & 0x7ff;
				if (--ncp && is_trailing(*_p1)) 
					c += (static_cast<uint8_t>(*_p1++) & 0x3f);
				return c;
			}
			if (c < 0xF0) // 3 bytes
			{
				c = (c << 12) & 0xffff;
				if (--ncp && is_trailing(*_p1)) 
					c += ((static_cast<uint8_t>(*_p1++) & 0x3f) << 6);
				if (--ncp && is_trailing(*_p1)) 
					c += (static_cast<uint8_t>(*_p1++) & 0x3f);
				return c;
			}
			c = (c << 18) & 0x1fffff;
			if (--ncp && is_trailing(*_p1)) 
				c += ((static_cast<uint8_t>(*_p1++) & 0x3F) << 12);
			if (--ncp && is_trailing(*_p1))
				c += ((static_cast<uint8_t>(*_p1++) & 0x3f) << 6);
			if (--ncp && is_trailing(*_p1)) 
				c += (static_cast<uint8_t>(*_p1++) & 0x3f);
			return c;		
		}
		return 0; // end of string
	}

	bool skip_if(predicate q) noexcept
	{
		auto p = _p1;
		auto c = getc();
		auto b = c && q(c);
		if (!b) _p1 = p;
		return b;
	}

	std::size_t skip_while(stl_string_view s) noexcept
	{
		charser_base_UTF8<char_type> str(s);
		std::size_t n = 0;
		auto p = _p1;
		UChar c;
		while (c = str.getc() && getc() == c)
		{
			++n;
			std::swap(_p1,p);
		}

		return n;
	}


	bool skip_if(stl_string_view s) noexcept
	{
		return charser_base_A<char_type>::skip_if(s);
	}

	constexpr UChar seek(predicate q, bool skipFound = false) noexcept
	{
		auto p = _p1;
		while(auto c = getc())
		{
			if (q(c))
			{
				if (!skipFound) _p1 = p;
				return c;
			}
			p = _p1;
		}
		return  0;
	}

	constexpr UChar seek_span(predicate q, bool skipFound, charser_base<char_type>& span,
		bool appendFound = false) noexcept
	{
		auto p1 = _p1;
		auto c = seek(q, false);
		auto p2 = _p1;
		if (c && (skipFound || appendFound))
		{
			skip();
			auto p3 = _p1;
			if (appendFound) p2 = p3;
			if (!skipFound) _p1 = p2;
		}
		span.assign(p1, p2);
		return c;
	}

	constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept
	{
		return charser_base_A<char_type>::seek(s, skipFound);
	}


	constexpr bool seek_span(stl_string_view s, bool skipFound, charser_base<char_type>& span,
		bool appendFound = false) noexcept
	{
		return charser_base_A<char_type>::seek_span(s, skipFound, span, appendFound);
	}


	constexpr bool startsWith(predicate q) const noexcept
	{
		auto c = peek();
		return c && q(c);
	}

	constexpr bool startsWith(stl_string_view s) const noexcept
	{
		return charser_base_A<char_type>::startsWith(s);
	}

	constexpr bool endsWith(predicate q) const noexcept
	{
		auto ncp = size();
		auto p = _p2 - 1;
		while (ncp)
		{
			if (!is_trailing(*p))
			{
				return q(charser_base_UTF8<char_type>(p, _p2).getc());
			}
			--p;
			--ncp;
		}
		return false;
	}

	constexpr bool endsWith(stl_string_view s) const noexcept
	{
		return charser_base_A<char_type>::endsWith(s);
	}

	constexpr bool contains(predicate q) const noexcept
	{
		charser_base_UTF8<char_type> it(*this);
		return(it.seek(q));
	}

	constexpr bool contains(stl_string_view s) const noexcept
	{
		charser_base_UTF8<char_type> it(*this);
		return(it.seek(s));
	}

	constexpr void trimLeading(predicate q) noexcept
	{
		while (skip_if(q)) {};
	}

	constexpr void trimTrailing(predicate q) noexcept
	{
		auto ncp = size();
		const char_type* p = _p2 - 1;
		while (ncp)
		{
			if (!is_trailing(*p))
			{
				charser_base_UTF8<char_type> tmp(p, _p2);
				if (!q(tmp.getc())) return;
				_p2 = p;
			}
			--p;
			--ncp;
		}
	}

	///@}

};

/// \brief Safe string parsing iterator.
/// \param CharT Character type
/// \param E Encoding
/// \param R True if it is a reverse iterator
/// \details  Encapsulates two poiters: one tracks current position and one 
/// guards the end of the sequence. Character getting and parsing functions 
/// advance current position pointer until it approaches the end one, thus 
/// decreasing the range.
///
/// UTF8/16 implementatiions provide no deep error handling. Any leading code
/// point corresponds to a character; extra supplementary code points are
/// ignored; incomplete sequences are processed as if the missing supplementary 
/// code points had zero values.
///
/// For a reverse iterator, current position is the end of string and the 
/// end-pointer is the address of the first code point of the sequence. 

template<typename CharT, class Impl>
class basic_charser : public Impl
{
public:
	using char_type = CharT;
	using base_type = Impl;
	using self_type = basic_charser<char_type, Impl>;
	using stl_string_view = std::basic_string_view<char_type, std::char_traits<char_type> >;
	template<typename A>
	using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
	
	/// Construct
	///@{

	basic_charser() = default;

	/// \fn basic_charser() noexcept
	/// \brief  Constructs empty.

	using Impl::Impl;
	/// \fn basic_charser(const char_type* begin, const char_type* end) noexcept
	/// \brief Constructs with bounds - the physical left and right one.

	/// \brief Constructs with a compatible charser's data.
	template<class I>
	constexpr explicit basic_charser(const basic_charser<char_type, I>& it)
		noexcept : base_type(it._p1, it._p2)
	{
	}

	/// \brief Assigns a compatible charser's data.
	template<class I>
	constexpr basic_charser& operator=(const basic_charser<char_type, I>& it)
		noexcept
	{
		this->assign(it._p1, it._p2);
		return *this;
	}

	/// \brief Casts to compatible charser.
	template<class I>
	operator basic_charser<char_type, I>() const noexcept
	{
		return basic_charser<char_type, I>(this->_p1, this->_p2);
	}

	/// \brief Constructs with a string_view.
	basic_charser(stl_string_view s) noexcept :
		self_type(s.data(), s.data() + s.size())
	{
	}

	/// \brief Assigns a string_view.
	basic_charser& operator=(stl_string_view s) noexcept
	{
		this->assign(s.data(), s.data() + s.size());
		return *this;
	}

	/// \fn void setBegin(const char_type* p) noexcept
	/// \brief Sets the left bound in memory.
	
	/// \fn void setEnd(const char_type* p) noexcept
	/// \brief Sets the right bound in memory.

	/// \fn void assign(const char_type* begin, const char_type* end) noexcept
	/// \brief Assigns new bounds - the physical left and right one.
	
	/// \fn void clear() noexcept
	/// \brief Assigns an empty range.

	using base_type::operator stl_string_view;
	/// \fn operator stl_string_view() const noexcept
	/// \brief Casts to string_view.

	
	/// \fn friend std::basic_ostream<char_type, std::char_traits<char_type> >&
	/// operator<<(std::basic_ostream<char_type, std::char_traits<char_type> >&
	///	os, const charser_base<char_type>& it)
	/// \brief Outputs to stream.


	///@}

	/// Bounds
	///@{
	
	/// \fn std::size_t size() const noexcept
	/// \brief Sequence size, in code points

	/// \fn bool empty() const noexcept 
	/// \brief True if size == 0
	
	/// \fn operator bool() const noexcept
	/// \brief True if size != 0
	
	/// \fn std::size_t length() const noexcept
	/// \brief Sequence length, in characters

	/// \fn const char_type* begin() const noexcept
	/// \brief The left physical bound of the sequence.

	/// \fn const char_type* end() const noexcept 
	/// \brief The right physical bound of the sequence.

	/// \fn const char_type* get() const noexcept 
	/// \brief CCurrent position. For a forward charser, is same as begin(),
	/// for a reverse one, is same as end().

	/// \fn bool operator==(const stl_string_view& s) const noexcept
	/// \brief Compares data with a string_view 

	///@}

	/// Increment and value get 
	///@{
	
	/// \fn void unchecked_skip(std::size_t n = 1) noexcept
	///\brief Increments the pointer without bounds check, unsafe
	
	/// \fn bool skip(std::size_t n = 1) noexcept
	///\brief Increments the pointer
	///\return False if the end of text has been reached
	
	/// \fn UChar peek() const noexcept 
	///\brief Gets current char without increment

	using base_type::getc;
	/// \fn UChar getc() noexcept
	///\brief Gets current char and increments the pointer

	/// \fn bool skip_if(predicate q) noexcept
	///\brief Increments the pointer if the current character satisfies
	/// the given condition; otherwise does nothing. 
	///\return True if the same and the pointer was incremented

	/// \fn std::size_t skip_while(stl_string_view s) noexcept
	///\brief Increments the pointer while the current sequence of characters
	/// matches the given string.
	/// \param s String to compare
	/// \return Number of characters matched
	
	/// \fn bool skip_if(stl_string_view s) noexcept
	///\brief If the sequence of characters, starting from the current 
	/// position, matches to the given string, sets the pointer to the 
	/// end of this sequence; otherwise does nothing.
	/// \param s String to compare
	/// \return True if the string was passed through
	
	/// \fn UChar seek(predicate q, bool skipFound = false) noexcept
	///\brief Searches for a character with advancing the pointer.
	/// \param predicate Character to search for.
	/// \param skipFound If true and q is found, the pointer is set to the next 
	/// character after it.
	/// \return THe character found or 0 if the end of data was reached.
	
	
	
	/// \fn bool seek(stl_string_view s, bool skipFound = false) noexcept
	///\brief Searches for a substring with advancing the pointer.
	/// \param s Substring to search
	/// \param skipFound If true and s is found, the pointer is set to end of s.
	/// \return True if s was found, false if the end of text was reached.


	/// \fn bool startsWith(predicate q) const noexcept
	///\brief Returns true if the sequence starts with the given char.
	
	/// \fn bool startsWith(stl_string_view s) const noexcept
	///\brief Returns true if the sequence starts with the given string.
	
	/// \fn  bool endsWith(predicate q) const noexcept
	///\brief Returns true if the sequence ends with the given char.
	
	/// \fn  bool endsWith(stl_string_view s) const noexcept
	///\brief Returns true if the sequence ends with the given string.
	
	/// \fn  bool contains(predicate q) const noexcept
	///\brief Returns true if the sequence contains the given char.

	/// \fn  bool contains(stl_string_view s) const noexcept
	///\brief Returns true if the sequence contains the given string.

	/// \fn  void trimLeading(predicate q) noexcept
	///\brief Same as seek(q, false).

	/// \fn  void trimTrailing(predicate q) noexcept
	///\brief If any trailing characters satisfy the given condition,
	/// sets the end-pointer to the character preceding those.

	///\brief Calls both trimLeading() and trimTrailing().
	constexpr void trim(predicate q) noexcept
	{
		base_type::trimLeading(q); base_type::trimTrailing(q);
	}

	///@}

	///@{
	/// Search with getting the span of search

	///\brief Same as seek(q, skipFound) but also returns the span of search
	///\param span Received the span of search
	/// \param appendFound If true, the found char is added to the span as well.
	/// \param appendSpan If true, the given span is not rewritten but extended.
	UChar seek_span(predicate q, bool skipFound, self_type& span,
		bool appendFound = false) noexcept // redefine only for the span type 
	{
		return base_type::seek_span(q, skipFound, span, appendFound);
	}

	///\brief Same as seek(s, skipFound) but also returns the span of search
	///\param span Received the span of search
	/// \param appendFound If true, the found substring is added to the span as well.
	/// \param appendSpan If true, the given span is not rewritten but extended.
	UChar seek_span(stl_string_view s, bool skipFound, self_type& span,
		bool appendFound = false) noexcept // redefine only for the span type 
	{
		return base_type::seek_span(s, skipFound, span, appendFound);
	}


	/// \brief Appends current char to a string
	///\return Current char or 0 at the end of text.
	template<typename A>
	constexpr UChar appendc(stl_string<A>& dst) noexcept
	{
		auto c = base_type::getc();
		if (c) dst += c;
		return c;
	}

	/// \brief Same as appendc() but clears the string.
	template<typename A>
	constexpr UChar getc(stl_string<A>& dst) noexcept
	{
		dst.clear();  return  appendc(dst);
	}

	/// \brief Searches for a character and appends the span of search
	/// to a string.
	/// \param dst String to which to append the span.
	/// \param appendFound If true, appends the found char to the dst as well.
	template<typename A>
	constexpr UChar seek_append(predicate q, bool skipFound, stl_string<A>& dst,
		bool appendFound = false) noexcept
	{
		self_type span;
		auto c = seek_span(q, skipFound, span, appendFound);
		dst.append(span);
		return c;
	}

	///\brief Clears the string and calls seek_append(q, true, dst, false).
	template<typename A>
	constexpr bool getline(stl_string<A>& dst, predicate q = '\n') noexcept
	{
		dst.clear(); return seek_append(q, true, dst, false);
	}

	///@}
	
};


//
//constexpr static bool is_trailing(char_type c) noexcept
//{
//	return (c & FC00) == 0xDC00;
//}
//
//template<>
//constexpr static bool is_leading<Encoding::UTF16>(char_type c) noexcept
//{
//	return (c & 0xFC00) == 0xD800;
//}
//
//
//template<>
//constexpr UChar getc<Encoding::UTF8, true>()  noexcept
//{
//	auto ncp = size();
//	if (ncp)
//	{
//		UChar c = static_cast<uint8_t>(*--_p2) & 0xFF;
//		if (c < 0x80) return c;
//		if (c < 0xC0 && --ncp)
//		{
//			c = ((c & 0x3f) + (static_cast<uint8_t>(*--_p2) << 6));
//			auto mask = c & 0x3800;
//			c &= 0x7ff;
//			if (mask == 0x3000) return c; // 2 bytes
//			if (mask == 0x2000 && --ncp) // trailing
//			{
//				c += (static_cast<uint8_t>(*--_p2) << 12);
//				mask = c & 0xF0000;
//				c &= 0xffff;
//				if (mask == 0xE0000) return c; // 3 bytes
//				if (mask == 0x80000 && --ncp) // trailing
//				{
//					c += (static_cast<uint8_t>(*--_p2) << 18);
//					mask = c & 0x3E00000;
//					c &= 0x1fffff;
//					if (mask == 0x3C00000) return c; // 4 bytes
//					if (mask == 0x2000000) // trailing
//					{
//						while (--ncp && (((*--_p2) & 0xC0) == 0x80))
//						{
//						} // scroll to the leading one or to the begin
//					}
//				}
//			}
//		}
//		return 0xFFFD; // invalid char		
//	}
//	return 0;
//}

// peek()



// skip_if(UChar)







using charser = basic_charser<char, charser_base_A<char> >;
using u8charser = basic_charser<char, charser_base_UTF8<char> >;
//using u16charser = basic_charser<wchar_t, charser_base_UTF16<wchar_t> >;
using u32charser = basic_charser<uint_least32_t, charser_base_A<uint_least32_t> >;

	

	
	

	//constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept
	//{
	//	// TODO reversed
	//	charser_impl_fixed<T> it(this->get(), this->end());
	//	bool b = it.seek(s, skipFound);
	//	if (!b && !it.empty())
	//	{
	//		while (it.get() != this->_ptr && _isTrailing(*it.get())) { it.setPointer(it.get() - 1); }
	//	}
	//	this->_ptr = it.get();
	//	return  b;





	//constexpr bool skip_if(stl_string_view s) noexcept
	//{	
	//	return static_cast<>(*this).skip_if;
	//}


	//constexpr UChar seek_span(predicate q, bool skipFound,
	//	self_type& span, bool appendFound = false) noexcept
	//{
	//	// TODO reversed
	//	auto p1 = this->get();
	//	auto c = seek(q, false);
	//	auto p2 = this->_ptr;
	//	if (c)
	//	{
	//		if (skipFound || appendFound)
	//		{
	//			this->skip(1);
	//			auto p3 = this->_ptr;
	//			if (!skipFound) this->_ptr = p2;
	//			if (appendFound) p2 = p3;
	//		}
	//	}
	//	span._ptr = p1;
	//	span._end = p2;
	//	return c;
	//}
	//constexpr bool seek_span(stl_string_view s, bool skipFound,
	//	self_type& span, bool appendFound = false) noexcept
	//{
	//	// TODO reversed
	//	auto p1 = this->get();
	//	auto b = seek(s, false);
	//	auto p2 = this->_ptr;
	//	if (b)
	//	{
	//		p2 += (appendFound ? s.size() : 0);
	//		this->_ptr += (skipFound ? s.size() : 0);
	//	}
	//	span._ptr = p1;
	//	span._end = p2;
	//	return b;
	//}
	//constexpr bool endsWith(predicate q) const noexcept
	//{
	//	// TODO reversed
	//	for (auto p = this->_end; p != this->_ptr; --p)
	//	{
	//		if (_isLeading(*this->_ptr))
	//		{
	//			charser_impl_utf8<T> it(p, this->_end);
	//			return q(it.peek());
	//		}
	//	}
	//	return false;

	//}

	//constexpr void trimTrailing(predicate q) noexcept
	//{
	//	// TODO reversed
	//	for (auto p = this->_end; p != this->_ptr; --p)
	//	{
	//		if (_isLeading(*this->_ptr))
	//		{
	//			charser_impl_utf8<T> it(p, this->_end);
	//			if (!q(it.peek())) return;
	//			this->_end = p;
	//		}
	//	}
	//}








/// Safe stream-like string searcher/iterator that reloades the buffer.
///\param CharT Character type.
///\param D Derived class; should implement reload() returning bool. 
///\detail Unlike charser which stops at the end of buffer, chunk_charser calls
/// loadNextChunk(), which should load a next chunk into the buffer, reassign pointers
/// if needed, and return false in case if no more data is avalable (e.g. eof).
/// This class deals with code points only, i.e. fixed-size values.

template<class D>
class chunk_charser : protected charser_base_A<char>
{
   auto reload() noexcept { return static_cast<D*>(this)->loadNextChunk(); }

   using charser_base_A<char>::seek;
   using charser_base_A<char>::seek_span;
   using charser_base_A<char>::skip_while;

   public:
    using base_type = charser_base_A<char>;
    using typename base_type::char_type;
    using typename base_type::stl_string_view;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
	
	using base_type::base_type;
	using base_type::assign;
	using base_type::get;
	using base_type::size;
	using base_type::empty;
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;
    

	
	///@{
	/// Construct

		///\fn basic_charser() noexcept 
		///\brief Constructs empty.

		///\fn basic_charser(const char_type* start, std::size_t n) noexcept
		/// Constructs with a range.

		///\fn void assign(const char_type* start, std::size_t n) noexcept 
		/// Assigns a new range.

	///@}

	///@{
	/** Bounds */

		///\fn const char_type* get() const noexcept 
		///\brief Gets pointer to the current position.

		///\fn const char_type* end() const noexcept 
		///\brief Get pointer to the end of sequence.

		///\fn auto size() const noexcept 
		///\brief Gets the size avalable, in code points.

		///\fn bool empty() const noexcept  
		///\brief /// Returns true if size == 0.

		///\fn operator bool() const noexcept  
		///\brief Returns true if size != 0.

	///@}

	///@{
	/// Increment; get value 

	///\brief  skips the pointer; returns false at the EOF. 
	constexpr bool skip(std::size_t n) noexcept
	{
		do
		{
			this->_ptr += n;
			auto d = (this->_end - this->_ptr);
			if (d >= 0) return true;
			n += d;
		} while (reload());
		return false;
	}
	constexpr bool skip() noexcept {return  skip(1);	}

	///\brief Increment the pointer if current char is the same as the given one
	bool skip_if(predicate q)  noexcept
	{
		auto p = this->get();
		auto c = getc();
		if (c)
		{
			if (q(c)) return true;
			this->_ptr = p;
			return false;
		}
		return  reload() ? skip(q) : false;
	}


	///\brief skips pointer while current data matches to a given string.
	/// \param s String to compare with.
	/// \return True if the whole string was passed through.
	constexpr bool skip_while(stl_string_view s) noexcept
	{
		auto p = s.data();
		std::size_t nLeft = s.size();
		do {
			auto n = base_type::skip(stl_string_view{ p, nLeft });
			p += n;
			nLeft -= n;
		} while (nLeft || (empty() && reload()));
		return !nLeft;
	}

	///\brief Gets current char or 0 at the EOF; no increment.
    UChar peek() noexcept 
    { 
		if (!empty()) return *this->_ptr;
		return reload()? peek() :0;
    }

	///\brief Gets current char and increments the pointer, returns 0 at the EOF.
    constexpr UChar getc() noexcept  
    {  
		if (!empty()) return base_type::getc();
		return reload() ? getc() : 0;
    }

	///\brief Appends current char to a string
	///\return Current char or 0 at the EOF.
	template<typename A>
	constexpr UChar appendc(stl_string<A>& dst) noexcept
	{
		auto c = getc();
		if (c) dst += c;
		return c;
	}

	/// Same as append but clears the string.
	template<typename A>
	constexpr UChar getc(stl_string<A>& dst) noexcept
	{
		dst.clear();  return  appendc(dst);
	}



	///@{ 
	/// Search-and-skip

	///\brief Searches for a character with advancing the pointer.
	/// \param predicate Character to search for.
	/// \param skipFound If true, the pointer will be set to the next character.
	/// \return Character found or 0 if the EOF was reached.
    constexpr UChar seek(predicate q, bool bSkipFound = false) noexcept 
    { 
        do {
            auto c = base_type::seek(q, bSkipFound); 
			if (c) return c;
        } while (reload());
        return 0;
    }

	///\brief Same as seek(q, skipFound) but also appends the span of 
	/// search to a string.
	/// \param dst String to which to append the span.
	/// \param appendFound If true, appends the found substring to the dst
	/// as well.
	template<typename A>
	constexpr UChar seek_append(predicate q, bool skipFound,
		stl_string<A>& dst, bool appendFound = false) noexcept
	{

		charser it;
		do
		{
			auto c = base_type::seek_span(q, skipFound, it, appendFound);
			dst.append(it);
	
			if (c) return c;
		} while (reload());

		return 0;
	}

	///\brief Clears the string and calls seek_append(q, true, dst, false).
	template<typename A>
	constexpr UChar getline(stl_string<A>& dst, predicate q = '\n') noexcept
	{
		dst.clear(); return seek_append(q, true.dst, false);
	}



	///\brief Same as skip_while(s) but appends the skipped chars to a string.
    template <typename A>
    constexpr bool skip_append_while(stl_string<A>& dst, const stl_string_view s) noexcept
    {
		auto p = s.data();
		auto nLeft = s.size();
        do {
            auto p = get();
			auto n = base_type::skip_while(stl_string_view{ p, nLeft });
            dst.append(p, n);
            p += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return !nLeft;
    }

};


//template<> 
// constexpr charser_encoding<kUtf16>::UChar basic_charser<kUtf16>::peek() noexcept 
// {
//     if(!empty())
//     {
//         UChar c = (UChar) _ptr[0]; 
//         if (c < 0xD800 || c >= 0xE000) return c;
//         if(size() > 1) return (((c & 0x3FF) << 10)
//             | (((UChar) _ptr[1]) & 0x3FF)) + 0x10000;
//     }
//     return 0;
// };

// template<> 
// constexpr charser_encoding<kUtf16>::UChar 
//     basic_charser<kUtf16>::getc() noexcept 
// {
//     if(!empty())
//     {
//         UChar c = (UChar) *_ptr++; 
//         if (c < 0xD800 || c >= 0xE000) return c;
//         if(!empty())  return (((c1 & 0x3FF) << 10) 
//             | (((UChar) *_ptr++) & 0x3FF)) + 0x10000;
//     }
//     return 0;
// };

}; // end namespace



  
	