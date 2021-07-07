#pragma once
//#include <string>
//#include <algorithm>
#include <functional>
#include <type_traits>
#include <string>
#include <memory>
#include <cctype>



/// Safe string searcher/iterator base which uses encodings of a fixed size.
/// \details   Encapsulates two poiters - one to the current position and one to the end of the sequence.
/// \param T Character type.
/// \param U A type that can hold all values of T.
/// \param R True if it is a reverse iterator.

template<typename T, typename U, bool R>
class basic_charsor {

    protected:

    const T* _ptr;
    const T* _end;

    public:

    using size_t = std::size_t;
    using stl_string_view = std::basic_string_view<T, std::char_traits<T> >;

    template<typename A> 
    using stl_string = std::basic_string<T, std::char_traits<T>, A>;


    private:

    //   helpers    ==========================================================================

  

    template<bool B = false>
    constexpr static void _add(const T*& p, size_t n = 1) noexcept
    { p += n; }
    template<>
    constexpr static void _add<true>(const T*& p, size_t n) noexcept
    {p -= n; }
    template<bool B = false>
    constexpr static void _add(size_t& n1, size_t n2) noexcept
    { n1 += n2; }
    template<>
    constexpr static void _add<true>(size_t& n1, size_t n2) noexcept
    {n1 -= n2; }

    template<bool B = false> constexpr const T* _head() noexcept
    {return _ptr; }
    template<> constexpr const T* _head<true>() noexcept
    {return _end + 1; }
    template<bool B = false> constexpr const T* _tail() noexcept
    {return _end; }
    template<> constexpr const T* _tail<true>() noexcept
    {return _ptr + 1; }
    template<bool B = false> constexpr const auto _size() const noexcept
    {return _end - _ptr; }
    template<> constexpr const auto _size<true>() const noexcept
    {return _ptr - _end; }

    template<bool B = false> constexpr const T* _head(const T* p, size_t n) noexcept
    {return p; }
    template<> constexpr const T* _head<true>(const T* p, size_t n) noexcept
    {return p + n - 1; }
    template<bool B = false> constexpr const T* _tail(const T* p, size_t n) noexcept
    {return p + n; }
    template<> constexpr const T* _tail<true>(const T* p, size_t n) noexcept
    {return p - 1; }

    template<bool B = false>  constexpr void _assign(const T* pLeast, const T* pLast) noexcept
    { _ptr = pLeast; _end  = pLast;}
    template<>  constexpr void _assign<true>(const T* pLeast, const T* pLast) noexcept
    {    _ptr = pLast - 1; _end = pLeast - 1; }



     constexpr basic_charsor& operator++() noexcept {++_ptr; return *this; }
    constexpr basic_charsor& operator--() noexcept {--_ptr; return *this; }
    constexpr basic_charsor operator++(int) noexcept 
    { basic_charsor it(*this); operator++(); return it;}
    constexpr basic_charsor operator--(int) noexcept 
    { basic_charsor it(*this); operator--(); return it;}
    constexpr basic_charsor& operator+=(std::size_t n) noexcept
    {_ptr += n; return *this;}
    constexpr basic_charsor& operator-=(std::size_t n) noexcept
    {_ptr -= n; return *this;}
    constexpr friend basic_charsor operator+(std::size_t d, basic_charsor it) noexcept
    {return it.operator+(d);}
    constexpr friend basic_charsor operator-(std::size_t d, basic_charsor it) noexcept
    {return it.operator-(d);}

    constexpr U operator*() noexcept {return (*_ptr);} 

    // =================================================================================

    public:

    ///@{
    /// Construct

    /// Construct an empty iterator.
    constexpr basic_charsor() noexcept: _ptr(0), _end(0) {}

    /// Construct an iterator with pointers to the data start/end.
    /// \param start Should point to the first character in the sequence; for a reverse iterator - to the last char.
    /// \param end Should point to the end of the sequence; for a reverse iterator - to the char preceding the first one.
     constexpr basic_charsor(const T* start, const T* end) noexcept: _ptr(start), _end(end)
    {assert(_size() >= 0);}

    /// Assign another data.
    /// \param start Should point to the first character in the sequence; for a reverse iterator - to the last char.
    /// \param end Should point to the end of the sequence; for a reverse iterator - to the char preceding the first one.
    constexpr void assign(const T* start, const T* end)  noexcept 
    {  _ptr = start;  _end = end; assert(_size() >= 0); }

    /// Construct with a null-terminated string.
    /// \param cstr Should point to the begin of the string for both direct and reverse iterators.
    constexpr basic_charsor(const T* cstr) noexcept 
    {  _assign<R>(cstr, cstr + std::char_traits<T>::length(cstr)); }

    /// Assign a null-terminated string.
    /// \param cstr Should point to the begin of the string for both direct and reverse iterators.
    constexpr basic_charsor& operator=(const T* cstr) noexcept
    {  _assign<R>(cstr, cstr + std::char_traits<T>::length(cstr)); return *this; }


    /// Construct with a stl string view data.
    constexpr basic_charsor(const stl_string_view& s) noexcept 
    {   _assign<R>(s.data(), s.data() + s.size());  }

    /// Assign a stl string view data.
    constexpr basic_charsor& operator=(const stl_string_view& s) noexcept
    {   _assign<R>(s.data(), s.data() + s.size());  return *this; }


    /// Construct with stl string data.
    template<typename A> 
    constexpr basic_charsor(const stl_string<A>& s) noexcept
     {   _assign<R>(s.data(), s.data() + s.size());  }

    /// Assign a stl string data.
    template<typename A> 
    constexpr basic_charsor& operator=(const stl_string<A>& s) noexcept  
    {   _assign<R>(s.data(), s.data() + s.size());  return *this; }
    
    ///@}

    ///@{
    /** Get bounds*/ 

    /// Get current position.
    constexpr const T* get() const noexcept { return  _ptr;}

    /// Get current value, unsafe.
    constexpr explicit operator const T*() const noexcept { return get();}

    /// Get a stl string view based on the current range.
    constexpr explicit operator stl_string_view() const noexcept
    { 
        return std::basic_string_view<T>(_head<R>(), size());
    }
    /// Get the end of processed sequence.
    constexpr const T* end() const noexcept {return  _end;}  

    /// Get the length of processed sequence.
    constexpr std::size_t size() const noexcept  {return  _size<R>();}  

    /// True if the length of the processed sequence is 0.
    constexpr bool empty() const noexcept 
    {
        return _end == _ptr;
    }
    /// True if the length of the processed sequence is not 0.
    constexpr operator bool() const noexcept {return !empty();}
   
    ///@}

    ///@{
    /** Compare pointers */ 

    constexpr bool operator<(const T* p) const noexcept {return get() < p;}
    constexpr bool operator>(const T* p) const noexcept {return get() > p;}
    constexpr bool operator<=(const T* p) const noexcept {return get() <= p;}
    constexpr bool operator>=(const T* p) const noexcept {return get() >= p;}
    constexpr bool operator==(const T* p) const noexcept {return get() == p;}
    constexpr bool operator!=(const T* p) const noexcept {return get() != p;}

     constexpr friend bool operator<(T* p, const basic_charsor& it) noexcept
        {return it.operator<(p);}
    constexpr friend bool operator>(T* p, const basic_charsor& it) noexcept
        {return it.operator>(p);}
    constexpr friend bool operator<=(T* p, const basic_charsor& it) noexcept
        {return it.operator<=(p);}
    constexpr friend bool operator>=(T* p, const basic_charsor& it) noexcept
        {return it.operator>=(p);}
    constexpr friend bool operator==(T* p, const basic_charsor& it) noexcept
        {return it.operator==(p);}
    constexpr friend bool operator!=(T* p, const basic_charsor& it) noexcept
        {return it.operator!=(p);}

    template <bool B>
    constexpr bool operator<(const basic_charsor<T, U, B>& it) const noexcept
        {return get() < it.get();}
    template <bool B>
    constexpr bool operator>(const basic_charsor<T, U, B>& it) const noexcept
        {return get() > it.get();}
    template <bool B>
    constexpr bool operator<=(const basic_charsor<T, U, B>& it) const noexcept
        {return get() <= it.get();}
    template <bool B>
    constexpr bool operator>=(const basic_charsor<T, U, B>& it) const noexcept
        {return get() >= it.get();}
    template <bool B>
    constexpr bool operator==(const basic_charsor<T, U, B>& it) const noexcept
        {return get() == it.get();}
    template <bool B>
    constexpr bool operator!=(const basic_charsor<T, U, B>& it) const noexcept
        {return get() != it.get();}

    ///@}

    ///@{
    /** Unsafe increment and get value */


    ///@}

    ///@{
    /** Safe increment and get value */ 

    /// Get current value without incremant; returns 0 if is at the end. 
    constexpr U peekc() noexcept
    { 
        return !empty()? *_ptr : 0;
    }

    /// Safe advance the pointer.  
    /// \param n Number of characters
    /// \return False if not possible (positioned at the end curently).
    constexpr bool skip() noexcept { _add<R>(_ptr, !empty()? 1 : 0); return !empty(); }
    constexpr bool skip(size_t n) noexcept  
    {  auto b = n <= size();  _add<R>(_ptr,  b? n : size());  return b; }

    /// Get current value and increment; returns 0 if is at the end.
    constexpr U getc() noexcept
    { 
        auto c = peekc();
        if (c) _add<R>(_ptr, 1);
        return c; 
    }

    /// Append current value to a string and increment; returns 0 if is at the end.
    template <typename A>
    constexpr U appendc(stl_string<A>& dst) noexcept
    { 
        auto c = getc(); 
        if(c) dst += c;  
        return c; 
    }

    /// Replace a string with current value and increment; returns 0 if is at the end.
    template <typename A>
    constexpr U getc(stl_string<A>& dst) noexcept  { dst.clear(); return copyc(dst);}

    /// Move data to a buffer which should be at least of size().
    T* move(T* dst) const noexcept  // spec 
    {   return std::char_traits<T>::move(dst, _head<R>(), size()); }
    /// Copy data to a buffer which should be at least of size().
    T* copy(T* dst) const  noexcept  // spec 
    {  return std::char_traits<T>::copy(dst, _head<R>(), size()); }

    /// Copy data to a buffer which should be at least of size() + 1, appending 0.
    T* c_str(T* dst) const noexcept  // spec 
    {   
        const T* src = _head<R>(); T c;
        for( ptrdiff_t i = size(); ; ) 
        {c = *src++; if(--i <= 0) break; *dst++ = c;} 
        *dst = 0;  return dst;
    }

    ///@}

    ///@{
    /** Basic unary predicates */ 

    

    template<U ch> static constexpr bool eq(U c) noexcept {return c == ch;}
    template<U ch> static constexpr bool lt(U c) noexcept {return c < ch;}
    template<U ch> static constexpr bool gt(U c) noexcept {return c > ch;}
    template<U ch> static constexpr bool lt_eq(U c) noexcept {return c <= ch;}
    template<U ch> static constexpr bool gt_eq(U c) noexcept {return c >= ch;}
    static constexpr bool is_space(U c) noexcept {return std::isspace(c);}
    static constexpr bool is_punct(U c) noexcept {return std::ispunct(c);}
    static constexpr bool is_blank(U c) noexcept {return std::isblank(c);}
    static constexpr bool is_alnum(U c) noexcept {return std::isalnum(c);}
    static constexpr bool is_print(U c) noexcept {return std::isprint(c);}
    static constexpr bool is_graph(U c) noexcept {return std::isgraph(c);}
    static constexpr bool is_cntrl(U c) noexcept {return std::iscntrl(c);}
    static constexpr  bool is_digit(U c) noexcept {return std::isdigit(c);}


    template<typename P> 
    static constexpr bool is_not(U c) noexcept { return !P(c); }

    template<U c1, U c2> static constexpr bool eq(U c) noexcept
    {return c == c1 || c == c2;}
     template<U c1, U c2, U c3> static constexpr bool eq(U c) noexcept
    {return c == c1 || c == c2  || c == c3;}
    
    ///@}

    

    ///@{
    /** Search for a character advancing the pointer until found or the end is reached */
    /// \return Character found or 0.

    static constexpr auto char_handler_stub = [](U){};
  
    template <typename P, typename H = decltype(char_handler_stub)>
    constexpr U seek_if(P q, H h = char_handler_stub) noexcept
    { 
        auto p = _ptr; auto n = size();
        if(n) 
        {
            while (!q(*p)) 
            {
                h(*p); 
                _add<R>(p, 1);
                if (--n == 0) break; 
            }
            _ptr = p;
        }
        return  n? *p : 0;
    }

    template<typename H = decltype(char_handler_stub)>
    constexpr U seek(U ch, H h = char_handler_stub) noexcept 
    {  
        auto p = _ptr; auto n = size();
        if(n) 
        {
            while (*p != ch) 
            {
                h(*p); 
                _add<R>(p, 1);
                if (--n == 0) break; 
            }
            _ptr = p;
        }
        return  n? *p : 0;
    }

    template<typename H = decltype(char_handler_stub)>
    constexpr U seek_of(const T* cstr, H h = char_handler_stub) noexcept
    {   
        auto n = size();
        if(n) 
        {
            auto p = _ptr; 
            for(auto s = cstr; ;)
            {
                if (*s != 0) 
                { 
                    if (*s == *p) break;
                    ++s; 
                    continue; 
                }
                h(*p);
                _add<R>(p, 1); if(--n == 0) break;
                s = cstr;
            } 
            _ptr = p;
        }
        return  n? *_ptr : 0; 
    }

    ///@}

    ///@{
    /** Advance the pointer while data matches to a given string */
    /// \return Number of characters matched.
    template<typename H = decltype(char_handler_stub)>
    constexpr std::size_t skipstr(const T* substr, size_t n, H h = char_handler_stub) noexcept  
    {
        n = std::min(size(), n);
        if(n)
        {
            auto p = get(); 
            auto s = _head<R>(substr, n);
            while(*s == *p)
            {
                h(*p);
               _add<R>(p, 1);
                if(--n == 0) break; 
                _add<R>(s, 1);
            } 
            n = p - _ptr;
           _ptr = p;
        }
        return n;
    } 
    template<typename H = decltype(char_handler_stub)>
    constexpr std::size_t skipstr(const T* substr, H h = char_handler_stub) noexcept  
    {
        return skipstr(substr, std::char_traits<T>::length(substr), h);
    } 
    template<typename H = decltype(char_handler_stub)>
    constexpr std::size_t skipstr(const stl_string_view& substr, H h = char_handler_stub) noexcept  
    {
        return skipstr(substr.data(), substr.size(), h);
    }

    template<typename A, typename H = decltype(char_handler_stub)>
    constexpr std::size_t skipstr(const stl_string<A>& substr, H h = char_handler_stub) noexcept  
    {
        return skipstr(substr.data(), substr.size(), h);
    }

    ///@}

    ///@{
    /** seek a given string. Returns true if found. 
        If not, the pointer is either at the end or at the last matching pattern. 
        E.g. if there's no "needle" but, there is a final "n", or "ne", or "need" etc, 
        left at the end of buffer, then, the pointer is set at that position.
    */
    template<typename H = decltype(char_handler_stub)>
    constexpr bool seekstr(const T* cstr, size_t n, H h = char_handler_stub) noexcept 
    { 
        auto nc; auto b;
        for(;;)
        {   
            nc = skipstr(cstr, n);
            auto b = nc == n;
            if (b || empty()) break;
            h(*_ptr);
            _add<R> (_ptr, -(nc - 1));
        }
        _add<R> (_ptr, -(nc));
        return b;
    }
    template<typename H = decltype(char_handler_stub)>
    constexpr bool seekstr(const T* s, H h = char_handler_stub) noexcept
    {
        return seekstr(s, std::char_traits<T>::length(s), h); // need to know str len in advance
    }
    template<typename H = decltype(char_handler_stub)>
    constexpr bool seekstr(const stl_string_view& s, H h = char_handler_stub) noexcept  
    {
        return seekstr(s.data(), s.size(), h);
    }
    template<typename A, typename H = decltype(char_handler_stub)>
    constexpr bool seekstr(const stl_string<A>& s, H h = char_handler_stub) noexcept  
    {
        return seekstr(s.data(), s.size(), h);
    }

    ///@}

    ///@{
    /** Append characters to stl string until a character is reached */

    template<typename P, typename A>
    constexpr U seek_append_if(P q, stl_string<A>& dst) noexcept  
    {
        return  seek_if(q, [&] (U c) {dst += c;});
    }

    template<typename A>
    constexpr U seek_append(T c, stl_string<A>& dst) noexcept  
    {
        return  seek(c, [&] (U c) {dst += c;});
    }

    template<typename A>
    constexpr U seek_append_of(const T* cstr, stl_string<A>& dst) noexcept  
    {
        return  seek_of(cstr, [&] (U c) {dst += c;});
    }


    ///@}

     ///@{
    /** Append characters to stl string until a terminating character and skip it */

    template <typename A>
    constexpr bool appendline(stl_string<A>& dst, T c = '\n') noexcept
    {
        append_seek(c, dst);
        return skip();
    }

    template <typename A>
    constexpr bool getline(stl_string<A>& dst, T c = '\n') noexcept
    {
        dst.clear();  return appendline(dst, c);
    }

    //@}

    ///@{
    /** Append characters to stl string while data matches to a given string, returns number of chars matched */

    template <typename A>
    constexpr std::size_t skipstr_append(const T* substr, std::size_t n, stl_string< A>& dst) noexcept  
    {
        return skipstr(substr, n, [&] (U c) {dst += c;});
    }


    template <typename A>
    constexpr std::size_t  skipstr_append(const T* substr, stl_string<A>& dst) noexcept  
    { return skipstr_append(substr, std::char_traits<T>::length(substr), dst); } 


    template <typename A>
    constexpr std::size_t  skipstr_append(const stl_string_view& substr, stl_string<A>& dst) noexcept 
    {
        return skipstr_append(substr.data(), substr.size(), dst);
    }

    template <typename A1, typename A2>
    constexpr std::size_t  skipstr_append(std::basic_string<A1>& substr, stl_string<A2>& dst) noexcept 
    {
        return skipstr_append(substr.data(), substr.size(), dst);
    }


    ///@}
};

using charsor = basic_charsor<char, unsigned char, false>;
using reverse_charsor  = basic_charsor<char, unsigned char, true>;

template<typename T, bool R>
class basic_u16charsor: public basic_charsor<T, uint_least16_t,  R>
{
    static_assert(sizeof(T) >= sizeof(uint_least16_t));

     // TODO: implement
};

 using u16charsor =  basic_u16charsor<uint_least16_t, false>;
 using reverse_u16charsor =  basic_u16charsor<uint_least16_t, true>;
 using wcharsor = basic_u16charsor<wchar_t, false>;
 using reverse_wcharsor = basic_u16charsor<wchar_t, true>;

template<typename T, bool R>
class basic_u8charsor: public basic_charsor<T, uint_least32_t, R>
 {
     // TODO: implement
 };
 using u8charsor =  basic_u8charsor<char, false>;
 using reverse_u8charsor =  basic_u8charsor<char, true>;














  
	