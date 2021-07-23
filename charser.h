#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <functional>

namespace char_parsers
{

using UChar = uint_least32_t;

/// Common character-checking unary predicates.

constexpr struct eq
{
    eq(UChar c) noexcept: ch(c){}
    bool operator() (UChar c) noexcept { return c == ch; }
    UChar ch;
};

constexpr struct gt
{
    gt(UChar c) noexcept: ch(c){}
    bool operator() (UChar c) noexcept { return c > ch; }
    UChar ch;
};

constexpr struct lt
{
    lt(UChar c) noexcept: ch(c){}
    bool operator() (UChar c) noexcept { return c < ch; }
    UChar ch;
};

constexpr struct gt_eq
{
    gt_eq(UChar c) noexcept: ch(c){}
    bool operator() (UChar c) noexcept { return c >= ch; }
    UChar ch;
};

constexpr struct lt_eq
{
    lt_eq(UChar c) noexcept: ch(c){}
    bool operator() (UChar c) noexcept { return c <= ch; }
    UChar ch;
};


auto constexpr is_space  =  [](UChar ch){return std::isspace(ch);};
auto constexpr is_punct  =  [](UChar ch){return std::ispunct(ch);};
auto constexpr is_blank  =  [](UChar ch){return std::isblank(ch);};
auto constexpr is_alnum =  [](UChar ch){return std::isalnum(ch);};
auto constexpr is_print  =  [](UChar ch){return std::isprint(ch);};
auto constexpr is_graph =  [](UChar ch){return std::isgraph(ch);};
auto constexpr is_cntrl =  [](UChar ch){return std::iscntrl(ch);};
auto constexpr is_digit  =  [](UChar ch){return std::isdigit(ch);};

constexpr struct any_of
{
    constexpr any_of(std::initializer_list<UChar> l) noexcept: _l(l){}
    constexpr bool operator() (UChar c) noexcept 
    { 
        for(auto & ch : _l)
        {
            if (c == ch) return true;
        }
        return false;
    }
    std::initializer_list<UChar> _l;
};


template <typename T1, typename T2>
struct either
{
    constexpr either(T1 t1, T2 t2) noexcept: _t1(t1),_t2(_t2) {}
    constexpr bool operator() (UChar c) noexcept 
    { 
        if (_t1(c)) return true;
        return _t2(c);
    }
    T1 _t1;
    T2 _t2;
};

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

    using stl_string_view = std::basic_string_view<char_type, std::char_traits<char_type> >;

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
    operator bool() const noexcept {return !empty();}  
    operator stl_string_view() const noexcept {return stl_string_view(get(), size());}  

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
    using stl_string_view = base_type::stl_string_view;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::_ptr;
    using base_type::_end;
    using base_type::size;
    using base_type::empty;
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;
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

    constexpr UChar seek_if(predicate q, bool skipFound) noexcept
    { 
        for(auto n = size(); n != 0; ++_ptr, --n) 
        {
            auto c = *_ptr;
            if (!q(c)) continue;
            if (skipFound) ++_ptr;
            return c;
        }
        return  0;
    }

    constexpr std::size_t skip(const char_type* s, std::size_t len) noexcept  
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

    constexpr bool seek(const char_type* s, std::size_t len, bool skipFound) noexcept 
    { 
        do {   
            auto n = skip(s, len);
            if (len == n) 
            {
                if(!skipFound) _ptr -= n;
                return true;
            }
            _ptr -= n;
        } while (skip());
        return false;
    }

};

template<typename T>
class charser_impl_utf8: public charser_base<T> {
    public:
    using base_type = charser_base<T>;
    using base_type::char_type;
    using stl_string_view = base_type::stl_string_view;
    template<typename A> 
    using stl_string = std::basic_string<char_type, std::char_traits<char_type>, A>;
    using base_type::_ptr;
    using base_type::_end;
    using base_type::size;
    using base_type::empty;
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;
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
    

    constexpr UChar seek_if(predicate q, bool skipFound) noexcept
    { 
        //TODO: impl
        return  0;
    }

    constexpr std::size_t skip(const char_type* s, std::size_t len) noexcept  
    {   
        //TODO: impl
        return  0;
    }
    constexpr bool seek(const char_type* cstr, std::size_t n, bool skipFound) noexcept 
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
    using base_type::operator typename base_type::stl_string_view;
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
    /// Increment; get value 

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
    /// Search 

    /// Advances the pointer until s given char is found or the end of data
    /// is reached. If skipFound = true, then, if the char is found, the pointer
    /// is set the position next to that char.
    /// \return The char found or 0 at the end of data.
    constexpr UChar seek(predicate q, bool skipFound = false) noexcept 
    { 
        return base_type::seek_if(q, skipFound); 
    }
    constexpr UChar seek(UChar ch, bool skipFound = false) noexcept 
    {  
        return seek(eq(ch), skipFound); 
    }
    constexpr UChar seek(std::initializer_list<UChar> l, bool skipFound = false) noexcept 
    {  
        return seek(char_parsers::any_of(l), skipFound); 
    }

    /// Same as seek_if but also returns the span lasting from the start of search
    /// to the char found or the end of data.This span is not affected by skipFound.
    constexpr bool get_seek(predicate q, self_type& dst, bool skipFound = false) noexcept 
    { 
       dst._ptr = get();
       auto c = seek(q, false);
       dst._end = get();
       if(skipFound) unchecked_skip();
       return c;
    }
    constexpr UChar get_seek(UChar ch, self_type& dst, bool skipFound = false) noexcept 
    { 
         return get_seek(eq(ch), dst, skipFound); 
    }
    constexpr UChar get_seek(std::initializer_list<UChar> l, self_type& dst, bool skipFound = false) noexcept 
    { 
         return get_seek(char_parsers::any_of(l), dst, skipFound); 
    }
    /// Same as seek_if but appends the span of search to a stl string. Setting
    /// appendFound to true results in both skipping the char and appending it to dst. 
    template<typename A>
    constexpr UChar append_seek(predicate q, stl_string<A>& dst, bool appendFound = false) noexcept 
    { 
       auto p = get();
       auto c = seek(q, appendFound);
       dst.append(p, get() - p);
       return c;
    }
    template<typename A>
    constexpr UChar append_seek(UChar ch, stl_string<A>& dst, bool appendFound = false) noexcept 
    { 
         return append_seek(eq(ch), dst, appendFound); 
    }
    template<typename A>
    constexpr UChar append_seek(std::initializer_list<UChar> l, stl_string<A>& dst, bool appendFound = false) noexcept 
    { 
         return append_seek(char_parsers::any_of(l), dst, appendFound); 
    }



     /// Advances the pointer until a substring is found or the end of data
    /// is reached. If skipFound = true, then, if the substring is found, 
    /// the pointer is set the position next to it.
    /// \return True: found, false otherwise.
    constexpr bool seek(const char_type* s, std::size_t len, 
        bool skipFound = false) noexcept 
    { 
       return base_type::seek(s, len, skipFound); 
    }
    constexpr bool seek(stl_string_view s, bool skipFound = false) noexcept  
    {
        return seek(s.data(), s.size(), skipFound);
    }

     /// Same as seek but also returns the span lasting from the start of search
    /// to the substr found or the end of data. This span is not affected by skipFound.
    constexpr bool get_seek(const char_type* s, std::size_t len, 
        self_type& dst, bool skipFound = false) noexcept 
    { 
       dst._ptr = get();
       auto b = seek(s, len, false);
       dst._end = get();
       if(skipFound) unchecked_skip(len);
       return b;
    }
    constexpr bool get_seek(stl_string_view s, self_type& dst, 
        bool skipFound = false) noexcept 
    { 
        return get_seek(s.data(), s.size(),dst, skipFound);
    }

     /// Same as seek but appends the span of search to a stl string. Setting
    /// appendFound to true results in both skipping the substring
    /// and appending it to dst. 
    template<typename A>
    constexpr bool append_seek(const char_type* s, std::size_t len, 
        self_type& dst,  bool appendFound = false) noexcept 
    { 
       auto p = get();
       auto b = seek(s, len, appendFound);
       dst.append(p, get() - p);
       return b;
    }
    template<typename A>
    constexpr bool append_seek(stl_string_view s, stl_string<A>& dst, 
        bool appendFound = false) noexcept 
    { 
        return append_seek(s.data(), s.size(), dst, appendFound);
    }


     /** Advances pointer while current data matches to a given string.
     /// \param s A string to compare with.
     /// \param n Length of the string.
     /// \return Number of characters matched.
     ///\details Compares characters of the string and buffer, incrementing 
         both pointers until either the string or the buffer ends.
    */

    constexpr std::size_t skip(const char_type* s, std::size_t n) noexcept  
    {
        return base_type::skip(s,n);
    } 
 
    constexpr std::size_t skip(stl_string_view s) noexcept  
    {
        return skip(s.data(), s.size());
    }


    constexpr bool startsWith(stl_string_view s) noexcept  
    {
        self_type it(*this);
        return it.skip(s) == s.size();
    }

    constexpr bool endsWith(stl_string_view s) noexcept  
    {
        self_type it(end() - s.size(), s.size());
        return it.skip(s) == s.size();
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
    using base_type::operator bool;
    using base_type::operator typename base_type::stl_string_view;
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
     /** Search */

    /** Advances the pointer searching for a character.
    /// \param q Unary predicate that takes an UChar and returns a boolean.
    /// \return The character found, or 0 if the EOF was reached.
    */

    constexpr UChar seek(predicate q, bool bSkipFound = false) noexcept 
    { 
        char_type c;
        do {
            c = base_type::seek_if(q, bSkipFound); 
        } while (!c && reload());
        return c;
    }

    constexpr UChar seek(UChar ch, bool bSkipFound = false) noexcept 
    {  
        return seek(eq(ch), bSkipFound);
    }
    constexpr UChar seek(std::initializer_list<UChar> l, bool skipFound = false) noexcept 
    {  
        return seek(char_parsers::any_of(l), skipFound); 
    }

     /** Advances the pointer while while data matches to a given string.
     /// \param s A string to compare with.
     /// \param n Length of the string.
     /// \return Number of characters matched.
     ///\details Compares the first character of the string with the current one, if they are equal returns,
      otherwise increments both pointers and coniniues until either the string or the buffer ends.
    */

    constexpr std::size_t skip(const char_type* s, std::size_t len) noexcept  
    {
        std::size_t nLeft = len;
        do {
            auto n =  base_type::skip(s, nLeft);
            s += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return len - nLeft;
    } 

    constexpr std::size_t skip(const char_type* s) noexcept  
    {
        return skip(s, std::char_traits<char_type>::length(s));
    } 
    constexpr std::size_t skip(const stl_string_view s) noexcept  
    {
        return skip(s.data(), s.size());
    }

    template<typename A>
    constexpr std::size_t skip(const stl_string<A>& s) noexcept  
    {
        return skip(s.data(), s.size());
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

    /** Appends data to a stl string while searching for a character.
    /// \param dst String to append data to.
    /// \param q Unary predicate that takes an UChar and returns a boolean.
    /// \param bAppFound Whether the found character should be appened either.
    /// \return The character found, or 0 if the end of buffer was reached.
    */

    template<typename A>
    constexpr UChar append_seek(predicate q, stl_string<A>& dst, bool bAppFound = false) noexcept  
    {
        char_type c;
        do
        {
           auto p = _ptr;
           c = base_type::seek_if(q, false);
           dst.append(p, _ptr - p);
        } while (!c && reload());
        if(bAppFound && c) 
        {
            dst += c; unchecked_skip();
        }
        return c;
    }

    template<typename A>
    constexpr UChar append_seek(char_type ch, stl_string<A>& dst, bool bAppFound = false) noexcept  
    {
        return append_seek(eq(ch), dst, bAppFound);
    }
    template<typename A>
    constexpr UChar append_seek(std::initializer_list<UChar> l, stl_string<A>& dst, bool bAppFound = false) noexcept 
    {  
        return append_seek(char_parsers::any_of(l), dst, bAppFound); 
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
        append_seek(term, dst, false);
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
    constexpr std::size_t append_skip(const char_type* s, std::size_t len, stl_string<A>& dst) noexcept  
    {
        std::size_t nLeft = len;
        do {
            auto p = get();
            auto n = skip(s, nLeft);
            dst.append(p, n);
            s += n;
            nLeft -= n;
        } while (nLeft || (empty() && reload()));
        return len - nLeft;
    }


    //template <typename A>
    //constexpr std::size_t  append_skip(stl_string<A>& dst, const char_type* s) noexcept  
    //{ return append_skip(dst, s, std::char_traits<char_type>::length(s)); } 

    template <typename A>
    constexpr std::size_t  append_skip(const stl_string_view s, stl_string<A>& dst) noexcept 
    {
        return append_skip(s.data(), s.size(), dst);
    }

};



}; // end namespace



  
	