/** Non-resizable heap-Aated string.*/
 
/**Acts similar to unique_ptr..*/


template<typename T>
class basic_cstring: public std::unique_ptr<T[]> {
  public:
  using std::unique_ptr<T[]>::get;


  constexpr basic_cstring(const T* s, size_t n) noexcept:
    std::unique_ptr<T[]> (new T[n + 1])
  {
    auto d = get();
    for ( ;n; ++d, ++s, --n) {*d = *s;} 
    *d = 0;
  };

  constexpr explicit basic_cstring(const T* s) noexcept: 
    basic_cstring(s, std::char_traits<T>::length(s)){
  }
   

  template<typename Iterator> 
  constexpr basic_cstring(const Iterator& first, const Iterator& last) noexcept: 
    std::unique_ptr<T[]>(new T[last - first + 1])
  {
    std::copy<Iterator, T*>(first, last, get());
    get()[last - first] = 0;
  };

  template<typename Container> 
  constexpr basic_cstring(const Container & ctr) noexcept: 
    basic_cstring(ctr.begin(),ctr.end())
  { 
  };

  constexpr basic_cstring<T>& operator=(const T* s) noexcept
  {
    return operator=(std::move(basic_cstring<T> {s}));
  };

  template<typename Container> 
  constexpr basic_cstring<T>& operator=(const Container & ctr) noexcept
  {
    return operator=(std::move(basic_cstring<T> {ctr}));
  };

  ///@{
  /** Compare strings */ 

  constexpr bool operator==(const T* s) const noexcept
  {   
    for(auto p = get(); *p == *s && *s; ++p, ++s){}
    return !*s;
  }

  constexpr bool operator!=(const T* s) const noexcept {return !operator==(s);}
  friend constexpr bool operator==(const T* x, const basic_cstring& y) noexcept {return y.operator==(x);}
  friend constexpr bool operator!=(const T* x, const basic_cstring& y) noexcept {return y.operator!=(x);}

  constexpr bool operator==(const basic_cstring& s) const noexcept {return operator==(s.get());}
  constexpr bool operator!=(const basic_cstring& s) const noexcept {return operator!=(s.get());}

  size_t size() const noexcept {return char_traits<T>::length(get());}

  ///@}
};

using cstring    = basic_cstring<char>;
using cstring16 = basic_cstring<char16_t>;
using cstring32 = basic_cstring<char32_t>;
using wcstring   = basic_cstring<wchar_t>;#pragma once
