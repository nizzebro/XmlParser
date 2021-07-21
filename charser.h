#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <functional>

namespace char_parsers
{

using UChar = uint_least32_t;

/// Common character-checking unary predicates.

template <UChar c>
auto constexpr is_eq =  [](UChar ch){return c == ch;};
template <UChar c>
auto constexpr is_lt = [](UChar ch){return ch < c;};
template <UChar c>
auto constexpr is_gt =  [](UChar ch){
    return ch > c;
};
template <UChar c>
auto constexpr is_lt_eq =  [](UChar ch){return ch <= c;};
template <UChar c>
auto constexpr is_gt_eq  =  [](UChar ch){return ch >= c;};
auto constexpr is_space  =  [](UChar ch){return std::isspace(ch);};
auto constexpr is_punct  =  [](UChar ch){return std::ispunct(ch);};
auto constexpr is_blank  =  [](UChar ch){return std::isblank(ch);};
auto constexpr is_alnum =  [](UChar ch){return std::isalnum(ch);};
auto constexpr is_print  =  [](UChar ch){return std::isprint(ch);};
auto constexpr is_graph =  [](UChar ch){return std::isgraph(ch);};
auto constexpr is_cntrl =  [](UChar ch){return std::iscntrl(ch);};
auto constexpr is_digit  =  [](UChar ch){return std::isdigit(ch);};


using predicate =   std::function<bool(UChar)>;


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

    // actual start and end in memory; for e.g. reversed iterator implementation

    public:

    using char_type = T;


    using stl_string_view = std::basic_string<char_type, std::char_traits<char_type> >;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;

    /// Constructs empty.
    charser_base() noexcept: _ptr(0), _end(0) {}

    /// Constructs with a range.
    charser_base(const char_type* p, std::size_t n) noexcept: _ptr(p), _end(p + n)
    {}

    /// Assigns a new range.
    void assign(const char_type* p, std::size_t n) noexcept 
    {  _ptr = p;  _end = p + n; }

    /// Gets pointer to the current position
    const char_type* get() const noexcept { return  _ptr;} 
    /// Gets pointer to the end of sequence
    const char_type* end() const noexcept {return  _end;} 
    /// Gets the size avalable, in code points
    std::size_t size() const noexcept  {return end() - get();} 
    /// Returns true if size == 0
    bool empty() const noexcept {  return end() == get();} 
    /// Returns true if size != 0
    operator bool() const noexcept {  return !empty();}  

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
    using stl_string_view = std::basic_string<char_type, std::char_traits<char_type> >;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::_ptr;
    using base_type::_end;
    using base_type::size;
    using base_type::empty;
        
    using base_type::base_type;

    void unchecked_skip(std::size_t n = 1) noexcept  {_ptr += n; } 
    bool skip(std::size_t n = 1) noexcept  
    {   
        _ptr += n;
        auto b = (size() >= 0);
        _ptr = b ? _ptr : _end;
        return b; 
    }
    constexpr UChar unchecked_peek() const noexcept  {return *_ptr; }
    constexpr UChar peek() const noexcept { return !empty()? unchecked_peek() :0; }
    constexpr UChar unchecked_getc() noexcept  {return *_ptr++; }
    constexpr UChar getc() noexcept  { return !empty()? unchecked_getc() :0;}

   
    constexpr UChar seek_if(predicate q) noexcept
    { 
        auto n = size();
        if (n) 
        { 
            while (!q(*_ptr)) { ++_ptr; if ((--n) == 0) break; }
        }
        return  n? *_ptr : 0;
    }

    constexpr UChar seek(UChar c) noexcept
    { 
        auto n = size();
        if(n) 
        { 
            while (c != *_ptr) { ++_ptr; if ((--n) == 0) break; }
        }
        return  n? *_ptr : 0;
    }


    constexpr UChar seek_of(const char_type* cstr) noexcept
    {   
        auto n = size();
        if(n) 
        {
            for(auto s = cstr; ;)
            {
                if (*s != 0) 
                { 
                    if (*s == *_ptr) break;
                    ++s; 
                    continue; 
                }
                ++_ptr;
                if(--n == 0) break;
                s = cstr;
            } 
        }
        return  n? *_ptr : 0; 
    }

    constexpr std::size_t skip_while(const char_type* s, std::size_t len) noexcept  
    {
        if(len = std::min(size(), len))
        {
            auto nLeft = len; 
            while(*s == *_ptr)
            {
                ++_ptr; ++s;
                if(--nLeft == 0) break; 
            } 
            len -= nLeft;
        }
        return len;
    } 


    constexpr bool seek(const char_type* cstr, std::size_t len) noexcept 
    { 
        do {   
            auto n = skip_while(cstr, len);
            _ptr -= n;
            if (len == n) return true;
        } while (skip());
        return false;
    }

};

template<typename T>
class charser_impl_utf8: public charser_base<T> {
    public:
    using base_type = charser_base<T>;
    using base_type::char_type;
    using stl_string_view = std::basic_string<char_type, std::char_traits<char_type> >;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::_ptr;
    using base_type::_end;
    using base_type::size;
    using base_type::empty;

    using base_type::base_type;

    // prototyped
    constexpr void unchecked_skip(std::size_t n = 1) noexcept
    {
        while(n){unchecked_peek();  --n;}
    }
    constexpr bool skip(std::size_t n = 1) noexcept  
    {   
        while (n && peek()) { --n; }
        return !n;
    }

    constexpr UChar _peek2(UChar c) noexcept
    {
        c = (c << 6) & 0x7ff;
        return c + (_ptr[1] & 0x3f);
    }
    constexpr UChar _peek3(UChar& c) noexcept
    {
        c = (c << 12) & 0xffff;
        c += ((0xff & _ptr[1]) << 6) & 0xfff;
        return c + (_ptr[2] & 0x3f);
    }
    constexpr UChar _peek4(UChar& c) noexcept
    {
        c = (c << 18) & 0x1fffff;
        c += ((0xff & _ptr[1]) << 12) & 0x3ffff;  
        c += ((0xff & _ptr[2]) << 6) & 0xfff;
        return c + (_ptr[3] & 0x3f);
    }

    constexpr UChar unchecked_peek() const noexcept
    {
        UChar c = (_ptr[0] & 0x3f);
        if (*_ptr < 0x80) return c; // 1 byte
        if (c < 0xE0)  return _peek2(c);
        if (c < 0xF0) return _peek3(c);
        return _peek4(c);
    }

    constexpr UChar peek() const noexcept 
    {
        auto n = size();
        if(n)
        {
          UChar c = (_ptr[0] & 0x3f);
          if (*_ptr < 0x80) return c; 
          if(--n)
          {
            if (c < 0xE0)  return _peek2(c);
            if(--n)
            {
                if (c < 0xF0) return _peek3(c);
                if(--n) return _peek4(c);
            }
          }
        }
        return 0;
    }
    constexpr UChar unchecked_getc() noexcept  
    {
        UChar c = (_ptr[0] & 0x3f);
        if (*_ptr < 0x80) 
        {
            ++_ptr;
            return c; 
        }
        if (c < 0xE0) 
        {
            c = _peek2(c);
           _ptr += 2;
            return c; 
        }
        if (c < 0xF0) 
        {
            c = _peek3(c);
           _ptr += 3;
            return c; 
        }
         c = _peek4(c);
         _ptr += 4;
         return c; 
    }
    
    UChar getc() noexcept 
    {
        auto n = size();
        if(n)
        {
            UChar c = (_ptr[0] & 0x3f);
            if (*_ptr < 0x80) 
            {
                ++_ptr;
                return c; 
            }
            if(--n)
            {
                if (c < 0xE0) 
                {
                    c = _peek2(c);
                   _ptr += 2;
                    return c; 
                }
                if(--n)
                {
                    if (c < 0xF0) 
                    {
                        c = _peek3(c);
                       _ptr += 3;
                        return c; 
                    }
                    if(--n)
                    {
                        c = _peek4(c);
                        _ptr += 4;
                        return c; 
                    }
                }
            }
        }
        return 0;
    } 
    

    constexpr UChar seek_if(predicate q) noexcept
    { 
        //TODO: impl
        return  0;
    }
    constexpr UChar seek_of(const char_type* cstr) noexcept
    {   
        //TODO: impl
        return  0;
    }

    constexpr bool seek(const char_type* cstr, std::size_t n, bool bSkip) noexcept 
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
    using typename base_type::char_type;
    using stl_string_view = std::basic_string_view<char_type, std::char_traits<char_type> >;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::base_type;
    using base_type::assign;
    using base_type::get;
    using base_type::end;
    using base_type::size;
    using base_type::empty;
    using base_type::operator bool;

    ///@{
    /// Construct

    ///\fn basic_charser() noexcept 
    ///\brief Constructs empty.
    ///\fn basic_charser(const char_type* start, std::size_t n) noexcept
    /// Constructs with a range.
    ///\fn void assign(const char_type* start, std::size_t n) noexcept 
    /// Assigns a new range.


    /// Assign a null-terminated string.
    constexpr basic_charser& operator=(const char_type* cstr) noexcept
    {
        assign(cstr, std::char_traits<char_type>::length(cstr)); 
        return *this;
    }

    /// Construct with a stl string view data.

    constexpr basic_charser(stl_string_view s) noexcept: 
         basic_charser(s.data(), s.size()){}

    /// Assign a stl string view data.
    constexpr basic_charser& operator=(stl_string_view s) noexcept
    {  
        assign(s.data(), s.size()); 
        return *this;
    }

    /// Construct with a compatible charser's data.
    template<typename B, std::enable_if_t
        <sizeof(typename B::char_type)==sizeof(char_type)> > 
    constexpr basic_charser(const basic_charser<B>& it) noexcept
     : basic_charser(it.get(), it.size())
    {
    }

    /// Assign a compatible charser's data..
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
    /// Returns true if size != 0.

    ///@}

     ///@{
    /** Increment; get value */ 

    /// Advances the pointer withound a bound check. 
    void unchecked_skip(std::size_t n = 1) noexcept  {return base_type::unchecked_skip(n);} 

    /// Advances the pointer; returns false at the end of buffer. 
    bool skip(std::size_t n = 1) noexcept  {return base_type::skip(n);} 

    /// Gets current character withound a bound check. 
    UChar unchecked_peek() const noexcept  {return base_type::unchecked_peek(); } 
    /// Gets current character without incremant; returns 0 at the end of buffer. 

    UChar peek() const noexcept   {return base_type::peek(); } 

    /// Gets current character and increments the pointer withound a bound check. 
    UChar unchecked_getc() noexcept {return base_type::unchecked_getc();} 

    /// Gets current character and increments the poiter; returns 0 if is at the end.
    UChar getc() noexcept   {return base_type::getc(); } 

    /// Moves data to a buffer which should be at least of size().
    char_type* move(char_type* dst) const noexcept { return base_type::move(dst); }

    /// Copies data to a buffer which should be at least of size().
    char_type* copy(char_type* dst) const  noexcept {return base_type::copy(dst);}

    /// Copies data to a buffer which should be at least of size() + 1, appending 0.
    char_type* c_str(char_type* dst) const noexcept {return base_type::c_str(dst);}

    ///@}

    ///@{
    /** Compare pointers */ 

    constexpr bool operator<(const char_type* p) const noexcept {return  get() < p;}
    constexpr bool operator>(const char_type* p) const noexcept {return  get() > p;}
    constexpr bool operator<=(const char_type* p) const noexcept {return  get() <= p;}
    constexpr bool operator>=(const char_type* p) const noexcept {return  get() >= p;}
    constexpr bool operator==(const char_type* p) const noexcept {return  get() == p;}
    constexpr bool operator!=(const char_type* p) const noexcept {return  get() != p;}

     constexpr friend bool operator<(char_type* p, const basic_charser& it) noexcept
        {return it.operator<(p);}
    constexpr friend bool operator>(char_type* p, const basic_charser& it) noexcept
        {return it.operator>(p);}
    constexpr friend bool operator<=(char_type* p, const basic_charser& it) noexcept
        {return it.operator<=(p);}
    constexpr friend bool operator>=(char_type* p, const basic_charser& it) noexcept
        {return it.operator>=(p);}
    constexpr friend bool operator==(char_type* p, const basic_charser& it) noexcept
        {return it.operator==(p);}
    constexpr friend bool operator!=(char_type* p, const basic_charser& it) noexcept
        {return it.operator!=(p);}

    template <int I, bool R, typename F>
    constexpr bool operator<(const basic_charser<base_type>& it) const noexcept
        {return  get() < it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator>(const  basic_charser<base_type>& it) const noexcept
        {return  get() > it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator<=(const  basic_charser<base_type>& it) const noexcept
        {return  get() <= it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator>=(const  basic_charser<base_type>& it) const noexcept
        {return  get() >= it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator==(const  basic_charser<base_type>& it) const noexcept
        {return  get() == it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator!=(const  basic_charser<base_type>& it) const noexcept
        {return  get() != it.get();}

    ///@}

    ///@{
     /** Search */

    /** Advances pointer until a character that satisfies condition is met.
    /// \param q Uunary predicate: takes UChar, returns boolean.
    /// \return The found character or 0 if the end of buffer was reached.
    */
    constexpr UChar seek_if(predicate q) noexcept 
    { 
        return base_type::seek_if(q); 
    }

    constexpr stl_string_view get_seek_if(predicate q) noexcept 
    { 
       auto p = get();
       return stl_string_view(p, base_type::seek_if(q)? get() - p : 0); 
    }

    /** Advances pointer until a given character is met.
    /// \param ch A character to search for.
    /// \return The found character or 0 if the end of buffer was reached.
    */

    constexpr UChar seek(UChar ch) noexcept 
    {  
        return  seek_if([&](UChar c) {return c == ch; });
    }

    constexpr stl_string_view get_seek(UChar ch) noexcept 
    { 
       auto p = get();
       return stl_string_view(p, base_type::seek(ch)? get()  - p: 0); 
    }

    /// Advances pointer until any character of a given string is met.
    /// \param cstr String that contains the characters to search for.
    /// \return The found character or 0 if the end of buffer was reached.
 

    constexpr UChar seek_of(const char_type* cstr) noexcept
    { 
        return base_type::seek_of(cstr); 
    }

    constexpr stl_string_view get_seek_of(const char_type* cstr) noexcept 
    { 
       auto p = get();
       return stl_string_view(p, base_type::seek_of(cstr)? get() - p : 0); 
    }

    /// Advances pointer until a given substring is found.
    /// \param cstr String to search for.
    /// \param n Length of the string.
    /// \param bSkip If true, the pointer will be set to the end of the substring.
    /// \return True if found, false otherwise.
 
    constexpr bool seek(const char_type* cstr, std::size_t len) noexcept 
    { 
       return base_type::seek(cstr, len); 
    }

    constexpr stl_string_view get_seek(const char_type* cstr, std::size_t len) noexcept 
    { 
       auto p = get();
       return stl_string_view(p, base_type::seek(cstr, len)? get() - p : 0); 
    }


    constexpr bool seek(stl_string_view s) noexcept  
    {
        return seek(s.data(), s.size());
    }

    constexpr stl_string_view get_seek(stl_string_view s) noexcept 
    { 
       return get_seek(s.data(), s.size()); 
    }

     /** Advances pointer while current data matches to a given string.
     /// \param s A string to compare with.
     /// \param n Length of the string.
     /// \return Number of characters matched.
     ///\details Compares characters of the string and buffer, incrementing 
         both pointers until either the string or the buffer ends.
    */

    constexpr std::size_t skip_while(const char_type* s, std::size_t n) noexcept  
    {
        return base_type::skip_while(s,n);
    } 
 
    constexpr std::size_t skip_while(stl_string_view s) noexcept  
    {
        return skip_while(s.data(), s.size());
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

   public:
    using base_type = charser_impl_fixed<char>;
    using typename base_type::char_type;
    using typename base_type::stl_string_view;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;

    using base_type::base_type;

    ///@{
    /** Increment and value get*/ 

    /// Gets current character without incremant; returns 0 at EOF. 
    UChar peek() noexcept 
    { 
        return (!empty() || reload())? base_type::unchecked_peek() :0;
    }

    /// Advances the pointer; returns false at eof. 
    constexpr bool skip(std::size_t n = 1) noexcept   
    {   
        do
        {
            unchecked_skip(n);
            auto d = size();
            if (d >= 0) return true;
            n += d;
        } while (reload());
        return false;
    }


    /// Gets current character and increments the poiter; returns 0 if is at EOF.

    constexpr UChar getc() noexcept  
    {  
        auto c = peek();
        unchecked_skip((c || reload())? 1 : 0); 
        return c;  
    }

   
    ///@{
    /** Compare pointers */ 

    constexpr bool operator<(const char_type* p) const noexcept {return get() < p;}
    constexpr bool operator>(const char_type* p) const noexcept {return get() > p;}
    constexpr bool operator<=(const char_type* p) const noexcept {return get() <= p;}
    constexpr bool operator>=(const char_type* p) const noexcept {return get() >= p;}
    constexpr bool operator==(const char_type* p) const noexcept {return get() == p;}
    constexpr bool operator!=(const char_type* p) const noexcept {return get() != p;}

     constexpr friend bool operator<(char_type* p, const base_type& it) noexcept
        {return it.operator<(p);}
    constexpr friend bool operator>(char_type* p, const base_type& it) noexcept
        {return it.operator>(p);}
    constexpr friend bool operator<=(char_type* p, const base_type& it) noexcept
        {return it.operator<=(p);}
    constexpr friend bool operator>=(char_type* p, const base_type& it) noexcept
        {return it.operator>=(p);}
    constexpr friend bool operator==(char_type* p, const base_type& it) noexcept
        {return it.operator==(p);}
    constexpr friend bool operator!=(char_type* p, const base_type& it) noexcept
        {return it.operator!=(p);}

    template <int I, bool R, typename F>
    constexpr bool operator<(const basic_charser<base_type>& it) const noexcept
        {return get() < it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator>(const  basic_charser<base_type>& it) const noexcept
        {return get() > it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator<=(const  basic_charser<base_type>& it) const noexcept
        {return get() <= it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator>=(const  basic_charser<base_type>& it) const noexcept
        {return get() >= it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator==(const  basic_charser<base_type>& it) const noexcept
        {return get() == it.get();}
    template <int I, bool R, typename F>
    constexpr bool operator!=(const  basic_charser<base_type>& it) const noexcept
        {return get() != it.get();}

    ///@}

    ///@{
     /** Search */

    /** Advances the pointer while searching for a character satisfying the given condition.
    /// \param q Unary predicate that takes an UChar and returns a boolean.
    /// \return The character found, or 0 if the end of buffer was reached.
    */

    constexpr UChar seek_if(predicate q) noexcept 
    { 
        char_type c;
        do {
            c = base_type::seek_if(q); 
        } while (!c && reload());
        return c;
    }

    /** Advances the pointer while searching for a character.
    /// ch A character to search for.
    /// \return The character found, or 0 if the end of buffer was reached.
    */

    constexpr UChar seek(UChar ch) noexcept 
    {  
        char_type c;
        do {
            c = base_type::seek(ch); 
        } while (!c && reload());
        return c;
    }

    /** Advances the pointer while searching for any character of the string.
    /// \param ch A character to search for.
    /// \return The character found, or 0 if the end of buffer was reached.
    */

    constexpr UChar seek_of(const char_type* cstr) noexcept
    { 
        char_type c;
        do {
            c = base_type::seek_of(cstr); 
        } while (!c && reload());
        return c;
    }

    

     /** Advances the pointer while while data matches to a given string.
     /// \param s A string to compare with.
     /// \param n Length of the string.
     /// \return Number of characters matched.
     ///\details Compares the first character of the string with the current one, if they are equal returns,
      otherwise increments both pointers and coniniues until either the string or the buffer ends.
    */

    constexpr std::size_t skip_while(const char_type* s, std::size_t len) noexcept  
    {
        std::size_t nLeft = len;
        do {
            auto n =  base_type::skip_while(s, nLeft);
            s += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return len - nLeft;
    } 

    constexpr std::size_t skip_while(const char_type* s) noexcept  
    {
        return skip_while(s, std::char_traits<char_type>::length(s));
    } 
    constexpr std::size_t skip_while(const stl_string_view s) noexcept  
    {
        return skip_while(s.data(), s.size());
    }

    template<typename A>
    constexpr std::size_t skip_while(const stl_string<A>& s) noexcept  
    {
        return skip_while(s.data(), s.size());
    }

    ///@}

    ///@{
     /** Search-and-copy */

    /// Appends current character to a string and increments the poiter; returns 0 at the end.
    template<typename A>
    constexpr UChar appendc(stl_string<A>& dst) noexcept  
    {
        auto c = getc();
        dst += (c? c : 0);
        return c;
    }

    /// Replaces a string with current character and increments the poiter; returns 0 at the end.
    template<typename A>
    constexpr UChar getc(stl_string<A>& dst) noexcept  
    {
        dst.clear();  return  appendc(dst);
    }

    /** Appends data to a stl string while searching for a character satisfying the given condition.
    /// \param dst String to append data to.
    /// \param q Unary predicate that takes an UChar and returns a boolean.
    /// \param bAppFound Whether the found character should be appened either.
    /// \return The character found, or 0 if the end of buffer was reached.
    */

    template<typename A>
    constexpr UChar append_seek_if(stl_string<A>& dst, predicate q, bool bAppFound = false) noexcept  
    {
        char_type c;
        do
        {
           auto p = _ptr;
           c = base_type::seek_if(q);
           dst.append(p, _ptr - p);
        } while (!c && reload());
        if(bAppFound && c) 
        {
            dst += c; unchecked_skip();
        }
        return c;
    }

    template<typename A>
    constexpr UChar append_seek(stl_string<A>& dst, char_type ch, bool bAppFound = false) noexcept  
    {
        char_type c;
        do
        {
           auto p = _ptr;
           c = base_type::seek(ch);
           dst.append(p, _ptr - p);
        } while (!c && reload());
        if(bAppFound && c) 
        {
            dst += c; unchecked_skip();
        }
        return c;
    }

    template<typename A>
    constexpr UChar append_seek_of(stl_string<A>& dst, const char_type* cstr, bool bAppFound = false) noexcept  
    {
        char_type c;
        do
        {
           auto p = _ptr;
           c = base_type::seek_of(cstr);
           dst.append(p, _ptr - p);
        } while (!c && reload());
        if(bAppFound && c) 
        {
            dst += c; unchecked_skip();
        }
        return c;
    }


     /** Appends data to a stl string until the specified terminating character.
    /// \param dst String to append data to.
    /// \param term Terminating character.
    /// \return True if terminator was found, false if the end of data was reached.
    */

     ///@{
    /** Appends characters to stl string until the given terminator or the end of data. 
        If the terminator was found, skips it and returns true; otherwise returns false.
    */

    template <typename A>
    constexpr bool appendline(stl_string<A>& dst, char_type term = '\n') noexcept
    {
        append_seek(dst, term, false);
        return skip();
    }

    template <typename A>
    constexpr bool getline(stl_string<A>& dst, char_type c = '\n') noexcept
    {
        dst.clear();  return appendline(dst, c);
    }

    //@}

    ///@{


    /** Appends characters to stl string while data matches to a given string, returns number of chars matched */

    template <typename A>
    constexpr std::size_t append_while(stl_string<A>& dst, const char_type* s, std::size_t len) noexcept  
    {
        std::size_t nLeft = len;
        do {
            auto p = get();
            auto n = skip_while(s, nLeft);
            dst.append(p, n);
            s += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return len - nLeft;
    }


    //template <typename A>
    //constexpr std::size_t  append_skip_while(stl_string<A>& dst, const char_type* s) noexcept  
    //{ return append_skip_while(dst, s, std::char_traits<char_type>::length(s)); } 

    template <typename A>
    constexpr std::size_t  append_while(stl_string<A>& dst, const stl_string_view s) noexcept 
    {
        return append_while(dst, s.data(), s.size());
    }

};



}; // end namespace



  
	