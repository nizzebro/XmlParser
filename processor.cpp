#include "processor.h"
#include <cassert>
#define _CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace char_parsers;




bool XmlParser::loadNextChunk() noexcept
{
    // no file checks here; rely on ItemType::kEnd which prevents next()
    size_t nRead = _fread_nolock(_buffer, 1, _chunkSize, _input);
    assign(_buffer, nRead);
    _nReadTotal += nRead; 

    if (nRead) return true;

    _eof = true;
    if(ferror(_input)) _errorCode = ErrorCode::kErrReadFile;
    return false;
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


bool XmlParser::appendRestOfPI() noexcept 
{   
    while (auto c = append_seek_if(_text, is_eq<'?'>, true))
    {
        if ((c =  appendc(_text)) == '>') return true;
    }
    return false;    
}


XmlParser::ItemType XmlParser::loadTag() noexcept 
{
   _text = '<'; 
   unchecked_skip();
   auto c = appendc(_text);
   if(c == '/') // End-tag: "</" + (any chars except '>')  + '>' 
   {
       if(append_seek_if(_text, is_eq<'>'>, true))
           return ItemType::kSuffix;
   }
   else if(c == '?') // PI (Processor Instruction): "<?" + (any chars except "?>") + "?>" 
   {
       if(appendRestOfPI()) return ItemType::kPI; 
   }
   else if(c == '!') // comment, or DTD, or CData: "<!" + (any chars, comments or PI's ) + '>' 
   {
       // comments are: "<!--"  + (any chars except "-->") +  "-->"
       // cData's are: "<[CDATA["  + (any chars except "]]>") +  "]]>"
       // DTD's are: "<!"  + (any chars except '>' and, besides, nested DTD's, PI's or comments) +  ">"

       // check if it is a comment, CData or DTD

        auto n = append_skip_while(_text, "--");

        if(n == 2)  
        {
            if (appendRestOfComment()) return ItemType::kComment;
            return ItemType::kEnd;
        }

        else if(getLevel() && append_skip_while(_text, "[CDATA[") == 7)    
        {  
            if(_options & Options::kKeepCDATAtags) _text.clear();

            if (appendRestOfCDATA())
            {
                if(_options & Options::kKeepCDATAtags) _text.erase(_text.size() - 3); 
                return  ItemType::kCData; 
            }
            return ItemType::kEnd;
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
                    if(append_skip_while(_text, "--") == 2) 
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
                if((--iNested) == 0) return ItemType::kDTD;
                // // TODO or not : if getLevel() error - DTD shouldn't appear inside elements.

            } // end if '<' or '>'
        } 
   } 
   else if(c) // start-tag
   {
       // append until '>' but check if it appended already; "<>" is odd but anyway...
       // still, TODO: better checks for first character maybe
        if(c != '>' && append_seek_if(_text, (is_eq<'>'>), true))  
        {
            if(_text[_text.size() - 2] != '/') return ItemType::kPrefix;
            return ItemType::kSelfClosing;
        }
   }

   return  ItemType::kEnd;   
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


XmlParser::ItemType XmlParser::loadText() noexcept
{
    if (_options & Options::kKeepMnenonics) 
    {
        auto c = append_seek(_text, '<');
        return c ? ItemType::kEscapedText : ItemType::kEnd;
    }
    while(auto c = append_seek_of(_text, "<&"))
    {
        if (c == '<') return ItemType::kEscapedText;
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

    return ItemType::kEnd;
}



bool XmlParser::next() noexcept
{
    if(!isEnd()) 
    {
        // clear buffer for append() and also to be empty at eof - for loop-copy while next()
        _text.clear(); 
        // check if current one is a suffix or a self-closing element
        // (self-closing ones are pushed too, for uniformity) and pop it
        
        if(isElementEnd())  _path.popItem(); 

        // skip ascii blanks
        auto c = seek_if(is_gt<' '>); 

        if(c == '<') // tag
        {
            _itemType = loadTag(); 
            if(isElement()) 
            {
                _path.pushItem(_text) ; // self-closing ones are pushed too, for uniformity
                return true;
            }

            if (isEnd()) _errorCode =  ErrorCode::kErrTagUnclosed;

            if (getLevel()) return true;

            // document level: handle end-tags alone 
            if (isSuffix())
            {
                _itemType = ItemType::kEnd; 
                _errorCode = ErrorCode::kErrTagUnmatch;
                return true;
            }
            // another odd thing for the document level would be CDATA, but loadTag() checks
            // for getLevel() - so that if it appears here, it is handled as DTD...
        }

        if(c) // some characters 
        {
            _itemType = loadText();

            if (getLevel())  return true;

            // document level; skip anything outside elements as BOM or garbage 
            if (seek('<')) return next(); 
            // eof, no errors
            _itemType = ItemType::kEnd; 
            return true;
        }

        // eof
        _itemType = ItemType::kEnd;
        if(getLevel()) _errorCode = ErrorCode::kErrTagUnmatch;
        return true;
    }

    return false;
}



//=====================     Initialization    ==================================//

XmlParser::XmlParser(std::size_t bufferSize) noexcept : 
    _input(0),
    _errorCode(ErrorCode::kErrOk),
    _nReadTotal(0),     
    _eof(false),
    _options(Options::kDefault),
    _itemType(ItemType::kEnd),  // the most important; prevents next()
    _path(),
    _text()
{
    _chunkSize = bufferSize + (buffer_gran - 1) & ~(buffer_gran - 1);
    _buffer = new char[_chunkSize];
    assign(_buffer, 0);
}


bool XmlParser::openFile(const char* path) noexcept
{
    closeFile();  // if open, closes and resets context

    _input = fopen(path, "rb");
    if(setvbuf(_input,  nullptr, _IONBF, 0 ) == 0)
    {
        _itemType = ItemType::kBegin; // allows parsing
        return true;
    }
    _input = 0;
    _errorCode = ErrorCode::kErrOpenFile;
    return false;
}

void XmlParser::closeFile() noexcept
{
    if(_input) 
    {
        fclose(_input);
        _errorCode = ErrorCode::kErrOk;
        _nReadTotal = 0;
        _eof = false;
        _itemType = ItemType::kEnd;  // prevents next()
        _path.clear();
        _text.clear();
        assign(_buffer, 0);
    }
}

XmlParser::~XmlParser()
{
    if(_input) fclose(_input);
    delete[] _buffer;
}

//=====================    Path  and tag components  ===========================//


std::string_view XmlParser::Path::reference::getName() const noexcept
{
    charser it(*this);
    if(it.getc() == '<')
    {
        auto p = it.get();  
        it.seek_if([](UChar c) {return is_lt_eq<' '>(c) || is_eq<'>'>(c) || is_eq<'/'>(c); });
        return std::string_view(p, it.get() - p);
    }
    return std::string_view();
}

bool XmlParser::Path::reference::hasAttributes() const noexcept
{
    charser it(*this);
    return it.seek_if(is_lt_eq<' '>) && it.seek_if(is_gt<' '>)
        && it.seek('=') && it.seek('"') && it.skip() && it.seek('"');
}

std::vector<XmlParser::Attribute> XmlParser::Path::reference::getAttributes() const noexcept
{
    charser it(*this);
    std::vector<Attribute> v;
    while(it.seek_if(is_lt_eq<' '>)) // "<tagname[blank]"
    {
        if(!it.seek_if(is_gt<' '>)) break; // "<tagname[blank][non-blank]"
        auto pName = it.get();
        if(!it.seek('=')) break; // "<tagname[blank]attrname="
        std::string_view name(pName, it.get() - pName); // attrname
        it.unchecked_skip();   // skip '='
        if(it.getc() != '"') break;
        auto pVal = it.get();
        if(!it.seek('"')) break;
        std::string_view value(pVal, it.get() - pVal); 
        it.unchecked_skip(); // skip '"'
        v.push_back(Attribute{name, value});
    }
    return std::move(v);
}

XmlParser::Path::reference XmlParser::Path::operator[](std::size_t n) const noexcept
{
    n = std::min(n, _offsets.size());
    auto oEnd = n >= 1? _offsets[n - 1] : 0;
    auto oStart = n >= 2? _offsets[n - 2] : 0;
    return reference(_tags.data() + oStart, oEnd - oStart);
}

void XmlParser::Path::pushItem(const std::string& s) noexcept
{
    _tags.append(s);   
    _offsets.push_back(_tags.size());
}

void XmlParser::Path::popItem() noexcept
{
    _offsets.pop_back(); 
    _tags.erase(_offsets.back());
}


//=====================     Write    ==================================//

bool XmlParser::writeItem(IWriter & writer, std::size_t userIndex) 
{
    writer.write(_text, userIndex);
    return next();
}

void XmlParser::writeElement(IWriter & writer, std::size_t userIndex)
{
    auto lvl = getLevel();
    while (!(isSuffix() && getLevel() == lvl) && writeItem(writer, userIndex)) 
    {
    } 
}

bool XmlParser::FileWriter::openFiles(std::string dir, 
    std::initializer_list<const char*>filenames) noexcept
{
    if(dir.back() != '\\') dir += ('\\');
    auto pos = dir.size();
    for(auto & fname : filenames)
    {
        dir.append(fname);
        dir += ('\0');
        auto f = fopen(dir.data(), "wb");
        if(!f) 
        {
            closeFiles();
            return false;
        }
        outputs.push_back(f);
        dir.erase(pos);
    }
    return true;
}

void XmlParser::FileWriter::write(const char * data, std::size_t n, 
    std::size_t userIndex)  noexcept
{
  _fwrite_nolock(data, 1, n, outputs[userIndex]);
}

void XmlParser::FileWriter::closeFiles() noexcept
{
    for(auto & f : outputs) { fclose(f);}
    outputs.clear();
}

XmlParser::FileWriter::~FileWriter()
{
    closeFiles();
}


