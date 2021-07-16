#include "processor.h"
#include <cassert>
#define _CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace char_parsers;


bool XmlParser::loadNextChunk() noexcept
{
  size_t nRead = _fread_nolock(_buffer, 1, _chunkSize, _input);
  assign(_buffer, nRead);
  _nReadTotal += nRead; 

    if (!nRead)
    {
    _eof = true;
    if(ferror(_input)) _errorCode = ErrorCode::kErrReadFile;
    return false;
    }

  return true;
}

bool XmlParser::appendRestOfComment() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq<'-'>, true)) 
    {
        while ((c =  appendc(_text)) == '-') {}
        if (c == '>') return true; 
    } 
    return false;    
}

bool XmlParser::appendRestOfCDATA() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq<']'>, true)) 
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
    while (auto c = append_seek_if(_text, is_eq<'?'>, true))
    {
        if ((c =  appendc(_text)) == '>') return true;
    }
    return false;    
}

// reads tag to textBuffer, advancing position to the next after it
// the curr. position should be next on "<"
// returns tag code (on error, Element::TagType::kNone = 0)


XmlParser::EntityType XmlParser::loadTag() noexcept 
{
   _text = '<'; 
   unchecked_skip();
   auto c = appendc(_text);
   if(c == '/') // End-tag: "</" + (any chars except '>')  + '>' 
   {
       if(append_seek_if(_text, is_eq<'>'>, true))
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
            if (appendRestOfComment()) return EntityType::kComment;
            return EntityType::kEnd;
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

        while (c = append_seek_of(_text, "<>", true))
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
        if(c != '>' && append_seek_if(_text, (is_eq<'>'>), true))  
        {
            if(_text[_text.size() - 2] != '/') return EntityType::kPrefix;
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


XmlParser::EntityType XmlParser::loadText() noexcept
{
    if (_keepEscaped) 
    {
        auto c = append_seek(_text, '<');
        return c ? EntityType::kEscapedText : EntityType::kEnd;
    }
    while(auto c = append_seek_of(_text, "<&"))
    {
        if (c == '<') return EntityType::kEscapedText;
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




void XmlParser::pushPrefix() noexcept
{
    _tags.append(_text);   
    _path.push_back(_tags.size());
}

void XmlParser::popPrefix() noexcept
{
    _path.pop_back(); 
    _tags.erase(_path.back());
}

bool XmlParser::next() noexcept
{
    auto t = _entityType;
    if(t != EntityType::kEnd) 
    {
        // clear buffer for append() and also to be empty at eof - for loop-copy while next()
        _text.clear(); 
        // check if current one is a suffix or a self-closing element
        // (self-closing ones are pushed too, for uniformity) and pop it
        
        if((int)t & ((int)EntityType::kSelfClosing | (int)EntityType::kSuffix)) 
        {
            popPrefix(); 
        }

        // skip ascii blanks
        auto c = seek_if(is_gt<' '>); 

        if(c == '<') // tag
        {
            _entityType = loadTag(); 
            if(isElement()) 
            {
                pushPrefix(); // self-closing ones are pushed too, for uniformity
                return true;
            }

            if (isEnd()) _errorCode =  ErrorCode::kErrTagUnclosed;

            if (level()) return true;

            // document level: handle end-tags alone or cdata-s
            if (isSuffix())
            {
                _entityType = EntityType::kEnd; 
                _errorCode = ErrorCode::kErrTagUnmatch;
                return true;
            }
            // another odd thing for the document level would be CDATA, but loadTag() checks
            // for level() - so that if it would appear, it would handled as DTD.
        }

        if(c) // some characters 
        {
            _entityType = loadText();

            if (level())  return true;

            // document level; skip anything outside elements as BOM or garbage 
            if (seek('<')) return next(); 
            // eof, no errors
            _entityType = EntityType::kEnd; 
            return true;
        }

        // eof
        _entityType = EntityType::kEnd;
        if(level()) _errorCode = ErrorCode::kErrTagUnmatch;
        return true;
    }

    return false;
}

bool XmlParser::process(const char* path, size_t bufferSize) noexcept
{
    _nReadTotal = 0;
    _eof = false;
    _tags.clear();
    _path.assign(2, 0); // to zeros for document level == 0
    _keepEscaped = false;
    _entityType = EntityType::kNone;

    _input = fopen(path, "rb");
    _errorCode = setvbuf(_input,  nullptr, _IONBF, 0 ) == 0 ? 
        ErrorCode::kErrOk : ErrorCode::kErrOpenFile;

    if(!error()) 
    {
        _chunkSize = (bufferSize + (buffer_gran - 1)) & (~(buffer_gran - 1));
        _buffer = new char[_chunkSize];

        assign(_buffer, 0);
        process();

        delete[] _buffer;     
    }

    fclose(_input);
    return !error();
}


std::string_view XmlParser::getName(std::size_t idx) const noexcept
{
    auto s = getSTagString(idx);
    auto p = s.data() + 1;
    charser it(p, s.size());
    it.seek_of(" >/");
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
    return std::string_view(_tags.data() + oStart, oEnd - oStart);
}

template<std::size_t N>
inline bool XmlParser::FileWriter::openFiles(const std::array<const char*, N>& filenames) noexcept
{
    closeFiles();
    for(auto & fn : filenames)
    {
        auto f = fopen(fn, "wb");
        if(!f) return false;
        outputs.push_back(f);
    }
    return true;
}

void XmlParser::FileWriter::write(const char * data, std::size_t n, std::size_t userIndex)  noexcept
{
  _fwrite_nolock(data, 1, n, outputs[userIndex]);
}

void XmlParser::FileWriter::closeFiles()
{
    for(auto & f : outputs) { fclose(f);}
    outputs.clear();
}

XmlParser::FileWriter::~FileWriter()
{
    closeFiles();
}

