
#include "processor.h"
#include <cassert>
#define _CRT_SECURE_NO_WARNINGS


using namespace std;
using namespace char_parsers;

//
//void Processor::write(size_t iFile, const char * s, int n) noexcept
//{
//  writeToBuffer(iFile, s, n);
//}
//
//void Processor::write(size_t iFile, const char * s) noexcept
//{
//  writeToBuffer(iFile, s, strlen(s));
//}
//
//
//template <int N>
//void Processor::write(size_t iFile, const char (&s)[N]) noexcept {
//  writeToBuffer(iFile, (const char*)s, N-1);
//}
//
//
////  PER-FILE SEARCH WRITING DATA; PPOS = CPOS 
//
//bool Processor::writeTo(size_t iFile, const char* s, int n) noexcept {
//  do {
//    bool b = moveTo(s, n);
//    Processor::write(iFile, cpos, (int) (endPos - cpos));
//    if(b) {
//      cpos = endPos;
//      return true;
//    }
//  } while(readToBuffer());
//  return false;
//}
//
//
//template <int N>
//bool Processor::writeTo(size_t iFile, const char (&s)[N]) noexcept {
//  Processor::writeTo(iFile, (const char*)s, N-1);
//}

//bool Processor::writeTo (size_t iFile, char c) noexcept {
//  do {
//      bool b = moveTo(c);
//      Processor::write(iFile, cpos, (int)(endPos - cpos));
//      if(b)  {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}
//



//void Processor::openFiles(const char* path, int nOutputFiles, const char* const(&outputFileNames)[]) noexcept
//{
//  input = fopen( path, "rb" );
//  if(setvbuf(input,  nullptr, _IONBF, 0 ) == 0) 
//  {
//    string s (path);
//    int n = s.find_first_of("/\\") + 1; // if npos then 0
//    auto  p = outputFileNames;
//    outputs.reserve(nOutputFiles);
//    while(outputs.size() != nOutputFiles) 
//    {
//      s.replace(n,string::npos,*p, strlen(*p));
//      ++p;
//      FILE* f = fopen(&s[0], "wb");
//      if(setvbuf(f,  nullptr, _IONBF, 0 ) == 0) {
//        char* p = new char[write_buffer_size];
//        outputs.push_back({f, p, p, p + write_buffer_size});
//        continue;
//      }
//      err = true; break;
//    } 
//    return;
//  }
//  err = true;
//}

//void Processor::writeToBuffer(size_t iFile, const char * s, int n) noexcept
//{
//  assert((n > 0) && (iFile < outputs._size()));
//  Output o = outputs[iFile];
//  do {
//    const char* end = min(o.bufferEnd, o.cpos + n);
//    while(o.cpos != end) {*o.cpos = *s;  ++o.cpos; ++s; --n;}
//    if(end <= o.bufferEnd) return;
//    o.cpos = o.buffer;
//  }
//  while(_fwrite_nolock((void*) o.buffer, 1, write_buffer_size, o.file) == write_buffer_size);
//  err = true;
//}
//
//
//void Writer::openFiles(const char* path, int nOutputFiles, const char* const(&outputFileNames)[]) noexcept
//{
//  input = fopen( path, "rb" );
//  if(setvbuf(input,  nullptr, _IONBF, 0 ) == 0) 
//  {
//    string s (path);
//    int n = s.find_first_of("/\\") + 1; // if npos then 0
//    auto  p = outputFileNames;
//    outputs.reserve(nOutputFiles);
//    while(outputs._size() != nOutputFiles) 
//    {
//      s.replace(n,string::npos,*p, strlen(*p));
//      ++p;
//      FILE* f = fopen(&s[0], "wb");
//      if(setvbuf(f,  nullptr, _IONBF, 0 ) == 0) {
//        char* p = new char[write_buffer_size];
//        outputs.push_back({f, p, p, p + write_buffer_size});
//        continue;
//      }
//      err = true; break;
//    } 
//    return;
//  }
//  err = true;
//}
//void Processor::closeFiles() noexcept
//{
//  for (auto &o : outputs)
//  {
//    if(o.file && (fclose(o.file) !=0)) err = true; 
//    delete[] o.buffer;
//  }
//  outputs.clear();
//  if(input && (fclose(input) !=0)) err = true; 
//  input = nullptr; 
//}


// reads a chunk from file to buffer, returns n bytes read; 0 = eof

bool XmlParser::loadNextChunk() noexcept
{
  size_t nRead = _fread_nolock((void*)get(), 1, _chunkSize, _input);
  _nReadTotal += nRead; 
  if(nRead != _chunkSize) 
  {
      assign(get(), nRead);
      if (!nRead)
      {
        _eof = true;
        if(ferror(_input)) _exitCode |= ExitCode::kErrReadFile;
        return false;
      }
  }
  return true;
}

 
bool XmlParser::appendRestOfComment() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq('-'), true)) 
    {
        while ((c =  appendc(_text)) == '-') {}
        if (c == '>') return true; 
    } 
    return false;    
}

bool XmlParser::appendRestOfCDATA() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq(']'), true)) 
    {
        while ((c =  appendc(_text)) == ']') {}
        if (c == '>') return true; 
    } 
    return false;    
}

// appends chars to textBuffer ending with "?>"
// the curr. position should be next after "<?"
// returns false on eof

bool XmlParser::appendRestOfPI() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq('?'), true))
    {
        if ((c =  appendc(_text)) == '>') return true;
    }
    return false;    
}

// reads tag to textBuffer, advancing position to the next after it
// the curr. position should be next on "<"
// returns tag code (on error, Element::TagType::kNone = 0)


int XmlParser::loadTag() noexcept 
{
   _text = '<'; 
   unchecked_skip();
   auto c = appendc(_text);
   if(c == '/') // End-tag: "</" + (any chars except '>')  + '>' 
   {
       if(append_seek_if(_text, (is_eq('>')), true))
           return EntityType::kSuffix;
   }
   else if(c == '?') // PI (Processor Instruction): "<?" + (any chars except "?>") + "?>" 
   {
       if(appendRestOfPI()) return EntityType::kPI;  
   }
   else if(c == '!') // comment, or DTD, or CData: "<!" + (any chars, comments or PI's ) + '>' 
   {
       // comments are: "<!--"  + (any chars except "-->") +  "-->"
       // cData's are: "<[CDATA["  + (any chars except "]]>") +  "]]>"
       // DTD's are: "<!"  + (any chars except '>' and, besides, nested DTD's, PI's or comments) +  ">"

       // check if it is a comment, CData or DTD

        auto n = append_skip_while(_text, "--");

        if(n == sizeof("--"))  
        {
            return appendRestOfComment()? EntityType::kComment :
            EntityType::kEnd;
        }

        else if(level() && append_skip_while(_text, "[CDATA[") == sizeof("[CDATA["))    
        {  
            if(!_keepEscaped) _text.clear();

            if (appendRestOfCDATA())
            {
                if(!_keepEscaped) _text.erase(_text.size() - sizeof("]>")); 
                return  EntityType::kCData; 
            }
            return EntityType::kEnd;
        }

        // DTD

        int iNested = 1;        // count matching '< >'

        while (c = append_seek_if(_text, is_of('<','>'), true))
        {
            if (c == '<')  // "...<"
            {   
                c = appendc(_text); 

                if (c == '!') // "<!-" comment?
                {
                    if(append_skip_while(_text, "--") == sizeof("--")) 
                    {
                        if (!appendRestOfComment()) break;
                    } 
                } 

                else if(c == '?') // "...<?"
                {
                    if(!appendRestOfPI()) break; 
                }

                else if(!c) break;
                else ++iNested;          
            }
            else // '...>'
            {  
                if((--iNested) == 0) return EntityType::kDTD;
                // // TODO or not : if level() error - DTD shouldn't appear inside elements.

            } // end if '<' or '>'
        } 
   } 
   else if(c) // start-tag
   {
       // append until '>' but check if it appended already; "<>" is odd but anyway...
       // still, TODO: better checks for allowed first character
        if(c != '>' && append_seek_if(_text, (is_eq('>')), true))  
        {
            if(_text[_text.size() - 1] != '/') return EntityType::kPrefix;
            return EntityType::kSelfClosing;
        }
   }

   return  EntityType::kEnd;   
}



void append_utf8(UChar c, std::string& dst)
{
    if (c < 0x80)                       
        dst += static_cast<char>(c);
    else if (c < 0x800) {               
        dst += static_cast<char>((c >> 6)          | 0xc0);
        dst += static_cast<char>((c & 0x3f)        | 0x80);
    }
    else if (c < 0x10000) {              
        dst += static_cast<char>((c >> 12)         | 0xe0);
        dst += static_cast<char>(((c >> 6) & 0x3f) | 0x80);
        dst += static_cast<char>((c & 0x3f)        | 0x80);
    }
    else {                               
        dst += static_cast<char>((c >> 18)         | 0xf0);
        dst += static_cast<char>(((c >> 12) & 0x3f)| 0x80);
        dst += static_cast<char>(((c >> 6) & 0x3f) | 0x80);
        dst += static_cast<char>((c & 0x3f)        | 0x80);
    }
}



int XmlParser::loadText() noexcept
{
    _text.clear();
    if (_keepEscaped) 
    {
        auto c = append_seek(_text, '<');
        return c ? EntityType::kText : EntityType::kEnd;
    }
    while(auto c = append_seek_if(_text, is_of('<', '&')))
    {
        if (c == '<') return EntityType::kText;
        unchecked_appendc(_text); 
        auto offs = _text.size(); // after "...&"
        if (peek() == '#') 
        {
            unchecked_skip();
            int radix = 10;
            if (peek() == 'x') 
            {
                radix = 16; 
                unchecked_skip();
            }
            c = append_seek(_text, ';');
            if(!c) continue;
            _text += '\0';
            auto val = strtoul(_text.data() + offs, 0, radix);
            _text.erase(--offs);
            append_utf8(val, _text);
            continue;
        }
        c = append_seek(_text, ';');
        if (!c) break;
        std::string_view s (_text.data() + offs, _text.size() - offs);
        char cc = 0;
        if (s == "quot") cc = 0x22;
        else if (s == "amp") cc = 0x26;
        else if (s == "apos") cc = 0x27;
        else if (s == "lt") cc = 0x3C;
        else if (s == "gt") cc = 0x3E;
        else 
        {
            _text += ';';
            continue;
        }
        _text.erase(--offs);
        _text += cc;
    }

    return EntityType::kEnd;
}

void XmlParser::skipElement() noexcept
{
    auto i = level();
    while(next() && level() >= i) {}
}

void XmlParser::writeEntity(IWriter & writer, std::size_t userIndex)
{
    writer.write(_text.data(), _text.size(), userIndex);
}

void XmlParser::writeElement(IWriter & writer, bool keepEscaping, std::size_t userIndex)
{
}


void XmlParser::pushPrefix() noexcept
{
    _path.push_back(_text.size());
    _tagStrings.append(_text);
}

void XmlParser::popPrefix() noexcept
{
    _path.pop_back(); 
    _tagStrings.erase(_path.back());
}

bool XmlParser::next() noexcept
{
    auto t = _entityType;
    if(t != EntityType::kEnd) 
    {
        // check if current one is a suffix or a self-closing element
        // (self-closing ones are pushed too, for uniformity) and pop it
        if(t & EntityType::kElementEndMask) 
        {
            popPrefix(); 
        }

        // skip ascii blanks
        auto c = seek_if(is_gt(' ')); 

        if(c == '<') // tag
        {
            auto t = loadTag(); 
            _entityType = t;
            if(t & EntityType::kElementStartMask) 
            {
                pushPrefix(); // self-closing ones are pushed too, for uniformity
                return true;
            }

            if (t == EntityType::kEnd) _exitCode |=  ExitCode::kErrTagUnclosed;

            if (level()) return true;

            // document level: handle end-tags alone or cdata-s
            if (t == EntityType::kSuffix)
            {
                _entityType = EntityType::kEnd; 
                _exitCode |= ExitCode::kErrTagUnmatch;
                return false;
            }
            // another odd thing for the document level would be CDATA, but loadTag() checks
            // for level() - so that if it would appear, it would handled as DTD.
        }

        if(c) // some characters 
        {
            _entityType = EntityType::kText;

            if(level())  return loadText();

            // document level; skip anything outside elements as BOM or garbage 
            if (seek('<')) return next(); 
            // eof, no errors
            _entityType = EntityType::kEnd; 
            return true;
        }

        // eof
        _entityType = EntityType::kEnd;
        if(level()) _exitCode |= ExitCode::kErrTagUnmatch;
        return true;
    }

    return false;
}


int XmlParser::process(const char* path, size_t bufferSize) noexcept
{
    _nReadTotal = 0;
    _eof = false;
    _tagStrings.clear();
    _path.assign(2, 0);
    _keepEscaped = false;
    _entityType = EntityType::kNone;

    _input = fopen(path, "rb");
    _exitCode = setvbuf(_input,  nullptr, _IONBF, 0 ) == 0 ? 
        ExitCode::kErrOk : ExitCode::kErrOpenFile;

    if(_exitCode == ExitCode::kErrOk) 
    {
        _chunkSize = (bufferSize + (buffer_gran - 1)) & (~(buffer_gran - 1));
        auto p = new char[_chunkSize];
        assign (p, 0);

        process();

        delete[] p;     
    }

    if(_input && fclose(_input) != 0) _exitCode |= ExitCode::kErrCloseFile;
    return _exitCode;
}



std::string_view XmlParser::getName(std::size_t idx) const noexcept
{
    auto s = getSTagString(idx);
    auto p = s.data() + 1;
    charser it(p, s.size());
    it.seek_if(is_of(' ', is_of('>','/')));
    return std::string_view(p, it.get() - p);
}

std::vector<XmlParser::Attribute> XmlParser::getAttributes(std::size_t idx) const noexcept
{
    auto s = getSTagString(idx);
    auto p = s.data() + 1; 
    charser it(p, s.size());
    std::vector<Attribute> v;
    while(it.seek(' '))
    {
        it.unchecked_skip();
        p = it.get();
        if(!it.seek('=')) break;
        std::string_view name(p, it.get() - p); 
        it.unchecked_skip();
        if(it.getc() != '"') break;
        p = it.get();
        if(!it.seek('"')) break;
        std::string_view value(p, it.get() - p); 
        it.unchecked_skip();
        v.push_back(Attribute{name, value});
    }
    return std::move(v);
}

std::string_view XmlParser::getSTagString(std::size_t idx) const noexcept
{
    assert(idx <= level());
    auto oStart = _path[idx];
    auto oEnd = _path[idx + 1];
    return std::string_view(_tagStrings.data() + oStart, oEnd - oStart);
}



//const std::string & XmlParser::Element::getEndTagString() noexcept
//{
//    if(_endTag.empty())
//    {
//       _endTag += '<';
//       _endTag += '/';
//       _endTag.append(getName());
//       _endTag  += '>';
//    }
//    return _endTag;
//}

