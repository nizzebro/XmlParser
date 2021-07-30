#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <functional>

namespace char_parsers
{

using UChar = uint_least32_t;

/// Ccharacter-checking unary predicates.

struct eq
{
	constexpr eq(UChar c) noexcept: ch(c){}
	constexpr bool operator() (UChar c) noexcept { return c == ch; }
    UChar ch;
};

struct gt
{
	constexpr gt(UChar c) noexcept: ch(c){}
	constexpr bool operator() (UChar c) noexcept { return c > ch; }
    UChar ch;
};

struct lt
{
	constexpr lt(UChar c) noexcept: ch(c){}
	constexpr bool operator() (UChar c) noexcept { return c < ch; }
    UChar ch;
};

struct gt_eq
{
	constexpr gt_eq(UChar c) noexcept: ch(c){}
	constexpr bool operator() (UChar c) noexcept { return c >= ch; }
    UChar ch;
};

struct lt_eq
{
	constexpr lt_eq(UChar c) noexcept: ch(c){}
	constexpr bool operator() (UChar c) noexcept { return c <= ch; }
    UChar ch;
};


template<typename T>
struct any_of
{
	constexpr any_of(std::initializer_list<T> l) noexcept : list(l) {}
	constexpr bool operator() (UChar c) noexcept
	{
		for (auto & ch : list)
		{
			if (check<std::is_integral_v<T> >(c, ch)) return true;
		}
		return false;
	}
	std::initializer_list<T> list;
	template<bool IsCharT = false>
	constexpr static bool check(UChar c, T t) noexcept {return (t(c));}
	template<>
	constexpr static bool check<true>(UChar c, T t) noexcept { return (c == t);}
};

template<typename T>
struct all_of: any_of<T>
{
	using any_of<T>::any_of;
	constexpr bool operator() (UChar c) noexcept
	{
		for (auto & ch : any_of<T>::list)
		{
			if (!any_of<T>::check<std::is_integral_v<T> >(c, ch)) return false;
		}
		return true;
	}
};

template<typename T>
struct not_of : any_of<T>
{
	using any_of<T>::any_of;
	constexpr bool operator() (UChar c) noexcept
	{
		for (auto & ch : any_of<T>::list)
		{
			if (any_of<T>::check<std::is_integral_v<T> >(c, ch)) return false;
		}
		return true;
	}
};


auto constexpr is_space  =  [](UChar ch){return std::isspace(ch);};
auto constexpr is_punct  =  [](UChar ch){return std::ispunct(ch);};
auto constexpr is_blank  =  [](UChar ch){return std::isblank(ch);};
auto constexpr is_alnum =  [](UChar ch){return std::isalnum(ch);};
auto constexpr is_print  =  [](UChar ch){return std::isprint(ch);};
auto constexpr is_graph =  [](UChar ch){return std::isgraph(ch);};
auto constexpr is_cntrl =  [](UChar ch){return std::iscntrl(ch);};
auto constexpr is_digit  =  [](UChar ch){return std::isdigit(ch);};


struct predicate : public std::function<bool(UChar)>
{
	using std::function<bool(UChar)>::function;
	using std::function<bool(UChar)>::operator=;
	predicate(UChar ch) : std::function<bool(UChar)>(eq(ch)) {}
	template<typename T>
	predicate(std::initializer_list<T> l) :
		std::function<bool(UChar)>(any_of<T>(l)) {}
};



/// Base class for safe text iterators.
///\param T Character type
/// \details  Encapsulates two poiters - to the current position and to the end of the sequence.
/// Contains only common logic; encoding-dependent functions are parts of derived implementations.  
template<typename T>
class charser_base
{ 

    protected:

    const T* _ptr;
    const T* _end;

    

    public:

    using char_type = T;

    using stl_string_view = std::basic_string_view<char_type, std::char_traits<char_type> >;
	

    /// Constructs empty.
    charser_base() noexcept: _ptr(0), _end(0) {}

    /// Constructs with a range.
    charser_base(const char_type* p, std::size_t n) noexcept: _ptr(p), _end(p + n)
    {}

    /// Assigns a new range.
    void assign(const char_type* p, std::size_t n) noexcept 
    {  _ptr = p;  _end = p + n; }

	void clear() noexcept { assign(0, 0); }

	void setBegin(const char_type* p) noexcept {_ptr = p;}
	void setEnd(const char_type* p) noexcept { _end = p; }
	void setSize(std::size_t n) noexcept { _end = _ptr + n;}

    /// Gets pointer to the current position
    const char_type* get() const noexcept { return  _ptr;} 
	const char_type* begin() const noexcept { return  _ptr; }
    /// Gets pointer to the end of sequence
    const char_type* end() const noexcept {return  _end;} 
	/// Imcrement ops; unsafe; provided only for range-based loop support
	charser_base<T>& operator++() const noexcept
	{ ++_ptr; return *this; }
	charser_base<T> operator++(int) const noexcept 
	{ auto it = *this; ++_ptr; return it; }

    /// Gets the size avalable, in code points
    std::size_t size() const noexcept  {return end() - get();} 
    /// Returns true if size == 0
    bool empty() const noexcept {  return end() == get();} 
    /// Returns true if size != 0
    operator bool() const noexcept {return !empty();}  
    operator stl_string_view() const noexcept {return stl_string_view(get(), size());}  
	friend std::basic_ostream<char_type, std::char_traits<char_type> >& 
		operator<<(std::basic_ostream<char_type, std::char_traits<char_type> >& 
			os, const charser_base<T>& it)
	{
		os << static_cast<const stl_string_view&>(it);
		return os;
	}

    /// Moves data to a buffer which should be at least of size().
    char_type* move(char_type* dst) const noexcept  // spec 
    {
        return std::char_traits<char_type>::move(dst, get(), size());
    }
    /// Copies data to a buffer which should be at least of size().
    char_type* copy(char_type* dst) const  noexcept  // spec 
    {  
        return std::char_traits<char_type>::copy(dst, base_type::get(), size()); 
    }
    /// Copies data to a buffer which should be at least of size() + 1, appending 0.
    char_type* c_str(char_type* dst) const noexcept  // spec 
    {   
        const char_type* src = get(); char_type c;
        for( ptrdiff_t i = size(); ; ) 
        {c = *src++; if(--i <= 0) break; *dst++ = c;} 
        *dst = 0;  return dst;
    }

    constexpr bool operator==(const stl_string_view& s) const noexcept
        {return  (static_cast<const stl_string_view&>(*this)) == s;}
    constexpr bool operator!=(const stl_string_view& s) const noexcept
        {return  !operator==(s);}
    friend bool operator==(const stl_string_view& s, const charser_base<T>& it)  noexcept
    {      return it == s;  }
    friend bool operator!=(const stl_string_view& s, const charser_base<T>& it)  noexcept
    {      return it != s;  }
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

   

template<typename T = char>
class charser_impl_fixed: public charser_base<T>
{
    public:
    using base_type = charser_base<T>;
    using base_type::char_type;
	using self_type = charser_impl_fixed<T>;
    using typename base_type::stl_string_view;


	using base_type::base_type;
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;

	void unchecked_advance(std::size_t n = 1) noexcept { this->_ptr += n;}

    bool advance(std::size_t n) noexcept  
    {   
		unchecked_advance(n);
		if (this->_end > this->_ptr) return true;
		this->_ptr = this->_end;
		return false;
    }

    constexpr UChar peek() const noexcept 
	{ 
		if(!this->empty()) return  *this->_ptr;
		return 0;
	}

	constexpr UChar getc() noexcept 
	{ 
		auto c = peek();
		if(c)this->_ptr++;
		return c; 
	}

	constexpr UChar seek(predicate q, bool skipFound = false) noexcept
	{
		for (auto n = this->size(); n != 0; ++this->_ptr, --n)
		{
			auto c = *this->_ptr;
			if (!q(c)) continue;
			if (skipFound) ++this->_ptr;
			return c;
		}
		return  0;
	}

    constexpr std::size_t skip_chars(stl_string_view s) noexcept
    {
		auto p = s.data();
		auto len = std::min(this->size(), s.size());
		if(len)
        {
            auto nLeft = len; 
            while(*p == *this->_ptr)
            {
                ++this->_ptr; ++p;
                if(--nLeft == 0) break; 
            } 
            len -= nLeft;
        }
        return len;
    } 

	constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept
	{
		if (!skipFound)
		{
			do {
				auto n = skip_chars(s);
				this->_ptr -= n;
				if (s.size() == n) return true;
			} while (advance(1));
			return false;
		}
		else
		{
			do {
				auto n = skip_chars(s);
				if (s.size() == n) return true;
				this->_ptr -= n;
			} while (advance(1));
			return false;
		}
	}

	constexpr UChar seek_span(predicate q, bool skipFound,
		self_type& span, bool appendFound = false) noexcept
	{
		auto p1 = this->get();
		auto c = seek(q, false);
		auto p2 = this->_ptr;
		if (c)
		{
			p2 += (appendFound ? 1 : 0);
			this->_ptr += (skipFound ? 1 : 0);
		}
		span._ptr = p1;
		span._end = p2;
		return c;
	}

	constexpr bool seek_span(stl_string_view s, bool skipFound,
		self_type& span, bool appendFound = false) noexcept
	{
		auto p1 = this->get();
		auto b = seek(s, false);
		auto p2 = this->_ptr;
		if (b)
		{
			p2 += (appendFound ? s.size() : 0);
			this->_ptr += (skipFound ? s.size() : 0);
		}
		span._ptr = p1;
		span._end = p2;
		return b;
	}
	constexpr bool endsWith(predicate q) noexcept
	{
		auto c = 0;
		if (!this->empty()) return q(*(this->end() - 1));
		return false;
		
	}

	constexpr void trimTrailing(predicate q) noexcept
	{
		while (endsWith(q)) { --this->_end; }
	}

};

template<typename T>
class charser_impl_utf8: public charser_base<T> {

	constexpr UChar _getc1() noexcept
	{
		if (!this->empty())
		{
			UChar c = *this->_ptr++;
			return c & 0x3f;
		}
		return 0;
	}
	constexpr UChar _getc2() noexcept
	{
		UChar c = _getc1() << 6;
		if (!this->empty()) return c + _getc1();
		return 0;
	}
	constexpr UChar _getc3() noexcept
	{
		UChar c = _getc1() << 12;
		if (!this->empty())
		{
			c += _getc1() << 6;
			if (!this->empty()) return c + _getc1();
		}
		return 0;
	}
	constexpr UChar _getc4() noexcept
	{
		UChar c = _getc1() << 18;
		if (!this->empty())
		{
			c += _getc1() << 12;
			if (!this->empty())
			{
				c += _getc1() << 6;
				if (!this->empty()) return c + _getc1();
			}
		}
		return 0;
	}

	constexpr UChar _getc() noexcept
	{
		for (;; ++this->_ptr) // skip extra trailing cp-s, if any
		{
			if (this->empty()) return 0;
			if ((*this->_ptr & 0xC0) != 0x80) break;
		}
		auto c = *this->_ptr;
		if (c < 0x80) return _getc1;
		if (c < 0xE0)  return _getc2;
		if (c < 0xF0) return _getc3;
		return  _getc4;
		return 0;
	}

    public:
	using base_type = charser_base<T>;
	using base_type::char_type;
	using self_type = charser_impl_utf8<T>;
	using typename base_type::stl_string_view;

	using base_type::base_type;
	using base_type::operator bool;
	using base_type::operator typename base_type::stl_string_view;

	// The approach used here always respects leading octets (< 0x80 || >= 0xC0).
	// These are considered chars even if there are not enough trailing octets 
	// after them (i.e. 10xxxxxx ones), in such case, the char value is arbitrary.
	// Extra trailing octets, if any, are just ignored and skipped.

	void unchecked_advance(std::size_t n = 1) noexcept 
	{ 
		for (; ((*this->_ptr & 0xC0) == 0x80); ++this->_ptr) {} // skip extra trailing cp-s, if any
		while (n)
		{
			++this->_ptr; // skip leading cp 

			for (; ((*this->_ptr & 0xC0) == 0x80); ++this->_ptr) {} // skip extra trailing cp-s, if any
			--n;
		}
	}

	constexpr bool advance(std::size_t n = 1) noexcept
	{
		for (;;++this->_ptr) // skip extra trailing cp-s, if any
		{ 
			if (this->empty()) return false;
			if ((*this->_ptr & 0xC0) != 0x80) break;
		}
		while (n)
		{
			++this->_ptr; // skip leading cp 

			for (;; ++this->_ptr) // skip extra trailing cp-s, if any
			{
				if (this->empty()) return !(--n);
				if ((*this->_ptr & 0xC0) != 0x80) break;
			}
			--n;
		}
		return !n; // n != 0 when it failed to skip as the end reached
	}

    constexpr UChar peek() const noexcept 
    {
		auto p = this->get();
		auto c = _getc();
		this->_ptr = p;
		return c;
    }

    UChar getc() noexcept 
    {
		return _getc();
    } 
    

	constexpr UChar seek(predicate q, bool skipFound = false) noexcept
    { 
        //TODO: impl
        return  0;
    }

	constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept
	{
		//TODO: impl
		return  0;
	}
	constexpr UChar seek_span(predicate q, bool skipFound,
		self_type& span, bool appendFound = false, bool appendSpan = false) noexcept
	{
		//TODO: impl
		return  0;
	}
	constexpr bool seek_span(stl_string_view s, bool skipFound,
		self_type& span, bool appendFound = false, bool appendSpan = false) noexcept
	{
		//TODO: impl
		return  0;
	}
};

/// Safe string searcher/iterator.
///\param Base Base class that implements encoding-specific logic.
/// \details  Encapsulates two poiters - one to the current position and one to the end of the sequence.

template<typename Impl = charser_impl_fixed<char> >
class basic_charser: public Impl {

    public:
    using base_type = typename Impl;
    using self_type = basic_charser<Impl>;
    using typename base_type::char_type;
    using typename base_type::stl_string_view ;

	template<typename A>
	using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;


    ///@{
    /// Construct
	basic_charser() :base_type() {}
	using base_type::base_type;

    /// Assigns a null-terminated string.
    constexpr basic_charser& operator=(const char_type* cstr) noexcept
    {
		base_type::assign(cstr, std::char_traits<char_type>::length(cstr));
        return *this;
    }

    /// Constructs with a stl string view data.
    constexpr basic_charser(stl_string_view s) noexcept: 
         basic_charser(s.data(), s.size()){}

    /// Assigns a stl string view data.
    constexpr basic_charser& operator=(stl_string_view s) noexcept
    {  
		base_type::assign(s.data(), s.size());
        return *this;
    }

    /// Constructs with a compatible charser's data.
    template<typename B, std::enable_if_t
        <sizeof(typename B::char_type)==sizeof(char_type)> > 
    constexpr basic_charser(const basic_charser<B>& it) noexcept
     : basic_charser(it.get(), it.size())
    {
    }

    /// Assigns a compatible charser's data..
    template<typename B, std::enable_if_t
        <sizeof(typename B::char_type)==sizeof(char_type)> > 
    constexpr basic_charser& operator=
        (const basic_charser<B>& it) noexcept  
    {  
		assign(it.get(), it.size());
        return *this; 
    }
    ///@}

    ///@{
    /// Increment; get value 

	void unchecked_advance(std::size_t n) noexcept
	{ return base_type::unchecked_advance(n); }

	///\brief Increments the pointer
	///\return False if the end of text has been reached
	bool advance(std::size_t n) noexcept{return base_type::advance(n); }

	bool skip() noexcept { return advance(1); }

	///\brief Increment the pointer only if the current char is the same as the given one
	///\return True if chars were the same and the pointer was inctremented
	bool skip(predicate q) noexcept
	{
		if (!this->empty())
		{
			auto p = base_type::get();
			if (q(getc())) return true;
			this->_ptr = p;
		}
		return false;
	}
	///\brief Advances the pointer while data is matching to a string.
	/// \param s String to compare
	/// \return true if the whole string passed through
	constexpr bool skip(stl_string_view s) noexcept
	{ return base_type::skip_chars(s) == s.size();}

	///\brief Gets current char without increment
	constexpr UChar peek() const noexcept 
	{ return base_type::peek();	}

	///\brief Gets current char and increments the pointer
	constexpr UChar getc() noexcept
	{ return base_type::getc(); }

	///@}

	///@{
	/// Search and advance 

	///\brief Searches for a character with advancing the pointer.
	/// \param predicate Character to search for.
	/// \param skipFound If true and q is found, the pointer is set to the next char.
	/// \return Character found or 0 if the end of data was reached.
	constexpr UChar seek(predicate q, bool skipFound = false) noexcept
	{ return base_type::seek(q, skipFound);}

	///\brief Same as seek(q, skipFound) but also returns the span of search
	///\param span Received the span of search
	/// \param appendFound If true, the found char is added to the span as well.
	/// \param appendSpan If true, the given span is not rewritten but extended.
	constexpr UChar seek_span(predicate q, bool skipFound, self_type& span,
		bool appendFound = false) noexcept
	{
		return base_type::seek_span(q, skipFound, span, appendFound);
	}

	///\brief Searches for a substring with advancing the pointer.
	/// \param s Substring to search
	/// \param skipFound If true and s is found, the pointer is set to end of s.
	/// \return True if s was found, false if the end of text was reached.

	///\brief = seek(s.data(), s.size(), skipFound). 
	constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept
	{
		return base_type::seek(s, skipFound);
	}

	///\brief Same as seek(s, skipFound) but also returns the span of search
	///\param span Received the span of search
	/// \param appendFound If true, the found substring is added to the span as well.
	/// \param appendSpan If true, the given span is not rewritten but extended.
	constexpr bool seek_span(stl_string_view s, bool skipFound, self_type& span,
		bool appendFound = false) noexcept
	{
		return base_type::seek_span(s, skipFound, span, appendFound);
	}

	///\brief Returns true if the sequence starts with a given char.
	constexpr bool startsWith(predicate q) const noexcept
	{
		auto c = peek();
		return (c? q(c) : 0);
	}

	///\brief Returns true if the sequence starts with a given string.
	constexpr bool startsWith(stl_string_view s) const noexcept
	{
		self_type it(*this);
		return it.skip(s);
	}
	///\brief Returns true if the sequence ends with a given char.
	constexpr bool endsWith(predicate q) const noexcept
	{
		return base_type::endsWith(q);
	}


	///\brief Returns true if the sequence ends with a given string.
	constexpr bool endsWith(stl_string_view s) const noexcept
	{
		if(s.size() > this->size()) return false;
		self_type it(this->end() - s.size(), s.size());
		return it.skips(s, s.size());
	}

	constexpr bool contains(predicate q) const noexcept
	{
		self_type it(*this);
		return(it.seek(q));
	}

	constexpr bool contains(stl_string_view s) const noexcept
	{
		self_type it(*this);
		return(it.seek(s));
	}

	constexpr void trimLeading(predicate q) noexcept
	{
		while (skip(q)) {}
	}
	constexpr void trimTrailing(predicate q) noexcept
	{
		base_type::trimTrailing(q);
	}
	constexpr void trim(predicate q) noexcept
	{
		trimLeading(q); trimTrailing(q);
	}

	///@}


	///@{
	/// Search, advance and append to string 

    ///\brief Appends current char to a string
	///\return Current char or 0 at the end of text.
	template<typename A>
	constexpr UChar appendc(stl_string<A>& dst) noexcept
	{
		auto c = base_type::getc();
		if(c) dst += c;
		return c;
	}

	/// Same as appendc() but clears the string.
	template<typename A>
	constexpr UChar getc(stl_string<A>& dst) noexcept
	{
		dst.clear();  return  appendc(dst);
	}

	///\brief Searches for a character and appends the span of search
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


using charser = basic_charser<>;
using u8charser =  basic_charser<charser_impl_utf8<char> >;
//using u16charser =  basic_charser<utf16charser_base<> >;
//using wcharser =  basic_charser<utf16charser_base<wchar_t> >;
using u32charser =  basic_charser<charser_base<uint_least32_t> >;


/// Safe stream-like string searcher/iterator that reloades the buffer.
///\param T Character type.
///\param D Derived class; should implement reload() returning bool. 
///\detail Unlike charser which stops at the end of buffer, chunk_charser calls
/// loadNextChunk(), which should load a next chunk into the buffer, reassign pointers
/// if needed, and return false in case if no more data is avalable (e.g. eof).
/// This class deals with code points only, i.e. fixed-size values.

template<typename T, class D>
class chunk_charser : public charser_impl_fixed<char>
{
   auto reload() noexcept { return static_cast<D*>(this)->loadNextChunk(); }

   using charser_impl_fixed<char>::seek;
   using charser_impl_fixed<char>::seek_span;
   using charser_impl_fixed<char>::skip_chars;

   public:
    using base_type = charser_impl_fixed<char>;
    using typename base_type::char_type;
    using typename base_type::stl_string_view;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;
    using base_type::base_type;

	
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

	///\brief  Advances the pointer; returns false at the EOF. 
	constexpr bool advance(std::size_t n) noexcept
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
	constexpr bool skip() noexcept {return  advance(1);	}

	///\brief Increment the pointer if current char is the same as the given one
	bool skip(predicate q)  noexcept
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


	///\brief Advances pointer while current data matches to a given string.
	/// \param s String to compare with.
	/// \return True if the whole string was passed through.
	constexpr bool skip(stl_string_view s) noexcept
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
		if (!empty()) return *this->_ptr++;
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
	/// Search-and-advance

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



	///\brief Same as skip_if(s, len, dst) but appends the skipped chars to a string.
    template <typename A>
    constexpr bool skip_append(stl_string<A>& dst, const stl_string_view s) noexcept
    {
		auto p = s.data();
		auto nLeft = s.size();
        do {
            auto p = get();
			auto n = base_type::skip_chars(stl_string_view{ p, nLeft });
            dst.append(p, n);
            p += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return !nLeft;
    }

};


}; // end namespace



  
	