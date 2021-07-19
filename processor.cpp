#include "processor.h"
#include <cassert>
#define _CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace char_parsers;




bool XmlParser::loadNextChunk() noexcept
{
    // no file checks here; rely on EntityType::kEnd which prevents next()
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
// returns tag code (on error, Element::TagType::kBegin = 0)


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

        if(n == 2)  
        {
            if (appendRestOfComment()) return EntityType::kComment;
            return EntityType::kEnd;
        }

        else if(level() && append_skip_while(_text, "[CDATA[") == 7)    
        {  
            if(_options & Options::kKeepCDATAtags) _text.clear();

            if (appendRestOfCDATA())
            {
                if(_options & Options::kKeepCDATAtags) _text.erase(_text.size() - 3); 
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
    if (_options & Options::kKeepMnenonics) 
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




void XmlParser::pushElement() noexcept
{
    _tags.append(_text);   
    _path.push_back(_tags.size());
}

void XmlParser::popElement() noexcept
{
    _path.pop_back(); 
    _tags.erase(_path.back());
}

bool XmlParser::next() noexcept
{
    if(!isEnd()) 
    {
        // clear buffer for append() and also to be empty at eof - for loop-copy while next()
        _text.clear(); 
        // check if current one is a suffix or a self-closing element
        // (self-closing ones are pushed too, for uniformity) and pop it
        
        if(isElementEnd())  popElement(); 

        // skip ascii blanks
        auto c = seek_if(is_gt<' '>); 

        if(c == '<') // tag
        {
            _entityType = loadTag(); 
            if(isElement()) 
            {
                pushElement(); // self-closing ones are pushed too, for uniformity
                return true;
            }

            if (isEnd()) _errorCode =  ErrorCode::kErrTagUnclosed;

            if (level()) return true;

            // document level: handle end-tags alone 
            if (isSuffix())
            {
                _entityType = EntityType::kEnd; 
                _errorCode = ErrorCode::kErrTagUnmatch;
                return true;
            }
            // another odd thing for the document level would be CDATA, but loadTag() checks
            // for level() - so that if it appears here, it is handled as DTD...
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



//=====================     Initialization    ==================================//

XmlParser::XmlParser(std::size_t bufferSize) noexcept : 
    _input(0),
    _errorCode(ErrorCode::kErrOk),
    _nReadTotal(0),     
    _eof(false),
    _options(Options::kDefault),
    _entityType(EntityType::kEnd),  // the most important; prevents next()
    _tags(),
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
        _entityType = EntityType::kBegin; // allows parsing
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
        _entityType = EntityType::kEnd;  // prevents next()
        _tags.clear();
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

//=====================     Tag parts    ==================================//

std::string_view XmlParser::getNameFromTag(std::string_view s) noexcept
{
    charser it(s);
    if(it.getc() == '<')
    {
        auto p = it.get();  
        it.seek_if([](UChar c) {return is_lt_eq<' '>(c) || is_eq<'>'>(c) || is_eq<'/'>(c); });
        return std::string_view(p, it.get() - p);
    }
    return std::string_view();
}

bool XmlParser::hasTagAttributes(std::string_view s) noexcept
{
    charser it(s);
    return it.seek_if(is_lt_eq<' '>) && it.seek_if(is_gt<' '>)
        && it.seek('=') && it.seek('"') && it.skip() && it.seek('"');
}

std::vector<XmlParser::Attribute> XmlParser::getAttributesFromTag(std::string_view s) noexcept
{
    charser it(s);
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


std::string_view XmlParser::getStartTag() const noexcept
{
    return getStartTag(level());
}

std::string_view XmlParser::getName() const noexcept
{
    return getName(level());
}

bool XmlParser::hasAttributes() const noexcept
{
    return hasAttributes(level());
}

std::vector<XmlParser::Attribute> XmlParser::getAttributes() const noexcept
{
    return getAttributes(level());
}

std::string_view XmlParser::getParentStartTag() const noexcept
{
    auto i = level();
    return getStartTag(i? i - 1 : 0);
}

std::string_view XmlParser::getParentName() const noexcept
{
    auto i = level();
    return getName(i? i - 1 : 0);
}

bool XmlParser::hasParentAttributes() const noexcept
{
    auto i = level();
    return hasAttributes(i? i - 1 : 0);
}

std::vector<XmlParser::Attribute> XmlParser::getParentAttributes() const noexcept
{
    auto i = level();
    return getAttributes(i? i - 1 : 0);
}

std::string_view XmlParser::getStartTag(std::size_t lvl) const noexcept
{
    assert(lvl <= level());
    auto oEnd = lvl >= 1? _path[lvl - 1] : 0;
    auto oStart = lvl >= 2? _path[lvl - 2] : 0;
    return std::string_view(_tags.data() + oStart, oEnd - oStart);
}

std::string_view XmlParser::getName(std::size_t lvl) const noexcept
{
    return getNameFromTag(getStartTag(lvl));
}

bool XmlParser::hasAttributes(std::size_t lvl) const noexcept
{
    return hasTagAttributes(getStartTag(lvl));
}

std::vector<XmlParser::Attribute> XmlParser::getAttributes(std::size_t lvl) const noexcept
{
    return getAttributesFromTag(getStartTag(lvl));
}


//=====================     Write    ==================================//

bool XmlParser::writeEntity(IWriter & writer, std::size_t userIndex) 
{
    writer.write(_text.data(), _text.size(), userIndex);
    return next();
}

void XmlParser::writeElement(IWriter & writer, std::size_t userIndex)
{
    auto lvl = level();
    while (!(isSuffix() && level() == lvl) && writeEntity(writer, userIndex)) 
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
