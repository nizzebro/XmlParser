#include "processor.h"

#define _CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace char_parsers;

bool XmlParser::loadNextChunk() noexcept
{
    // no file checks here; rely on ItemType::kEnd which prevents next()
    size_t nRead = _fread_nolock(_buffer, 1, _chunkSize, _input);
    assign(_buffer, _buffer + nRead);
    _nReadTotal += nRead; 
    if (nRead) return true;
    _eof = true;
    if(ferror(_input)) _errorCode = ErrorCode::kErrReadFile;
    return false;
}

bool XmlParser::appendRestOfComment() noexcept 
{  
    while (seek_append('-', true, _text, true))
    {
		UChar c;
        while ((c =  appendc(_text)) == '-') {}
        if (c == '>') return true; 
    } 
    return false;    
}

bool XmlParser::appendRestOfCDATA() noexcept 
{   
    while (seek_append(']',true, _text, true))
    {
		UChar c;
        while ((c =  appendc(_text)) == ']') {}
        if (c == '>') return true; 
    } 
    return false;    
}

bool XmlParser::appendRestOfPI() noexcept 
{   
    while (seek_append('?', true, _text, true))
    {
        if (appendc(_text) == '>') return true;
    }
    return false;    
}

XmlParser::ItemType XmlParser::loadTag()  noexcept
{
	getc(_text); // init with '<' and skip it;
	auto c = appendc(_text);
	if (c == '/') // End-tag: "</" + (any chars except '>')  + '>' 
	{
		if (seek_append('>', true, _text, true))
			return ItemType::kSuffix;
	}
	else if (c == '?') // PI (Processor Instruction): "<?" + (any chars except "?>") + "?>" 
	{
		if (appendRestOfPI()) return ItemType::kPI;
	}
	else if (c == '!') // comment, or DTD, or CData: "<!" + (any chars, comments or PI's ) + '>' 
	{
		// comments are: "<!--"  + (any chars except "-->") +  "-->"
		// cData's are: "<[CDATA["  + (any chars except "]]>") +  "]]>"
		// DTD's are: "<!"  + (any chars except '>' and, besides, nested DTD's, PI's or comments) +  ">"

		// check if it is a comment, CData or DTD

		if (skip_append_while(_text, "--"))
		{
			if (appendRestOfComment()) return ItemType::kComment;
			return ItemType::kEnd;
		}

		else if (getLevel() && skip_append_while(_text, "[CDATA["))
		{
			if (_options & Options::kKeepCDATAtags) _text.clear();

			if (appendRestOfCDATA())
			{
				if (_options & Options::kKeepCDATAtags) _text.erase(_text.size() - 3);
				return  ItemType::kCData;
			}
			return ItemType::kEnd;
		}

		// DTD

		int iNested = 1;        // count matching '< >'

		while (c = seek_append({ '<','>' }, true, _text, true))
		{
			if (c == '<')  // "...<"
			{
				c = appendc(_text);

				if (c == '!') // "<!-" comment?
				{
					if (skip_append_while(_text, "--"))
					{
						if (!appendRestOfComment()) break;
					}
				}

				else if (c == '?') // "...<?"
				{
					if (!appendRestOfPI()) break;
				}

				else if (!c) break;
				else ++iNested;
			}
			else // '...>'
			{
				if ((--iNested) == 0) return ItemType::kDTD;
				// // TODO or not : if getLevel() error - DTD shouldn't appear inside elements.

			} // end if '<' or '>'
		}
	}
	else if (c) // start-tag
	{
		// append until '>' but check if it appended already; "<>" is odd but anyway...
		// still, TODO: better checks for first character maybe
		if (c != '>' && seek_append('>', true, _text, true))
		{
			if (_text[_text.size() - 2] != '/') return ItemType::kPrefix;
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

void XmlParser::unescapeText() noexcept
{
    _tmp.clear();
    charser it(_text);
    while(it.seek_append('&', true, _tmp)) // search for '&'; skip it but do not append
    {
        charser tok; // mnemonic to resolve
		// search for ';'; skip it but do not append to the mnemonic 
        if(!it.seek_span(';', true, tok))
        {
			// end of text but no ';'? bad... but let's leave it as is
			_tmp += '&';
            _tmp.append(tok); 
			_tmp += ';';
            break;
        }
        if (tok.skip_if('#'))
        {
            int radix = tok.skip_if('x')? 16 :10;
            *const_cast<char*>(tok.end()) =  '\0'; // replace ';' with zero for strtoul
            auto val = strtoul(tok.get(), 0, radix);
            append_utf8(val, _tmp);
            continue;
        }
        char c = 0;
        if (tok == ("quot")) c = 0x22;
        else if (tok == ("amp")) c = 0x26;
        else if (tok == ("apos")) c = 0x27;
        else if (tok == ("lt")) c = 0x3C;
        else if (tok == ("gt")) c = 0x3E;
        else // unknown, write it as is 
        {
            _tmp += '&';
            _tmp.append(tok);
            _tmp += ';';
            continue;
        }
        _tmp += c;
        continue;
    }
    std::swap(_text, _tmp);
}

XmlParser::ItemType XmlParser::loadText() noexcept
{

    auto c = seek_append('<', false, _text);
    if (c && (_options & Options::kUnescapeText)) unescapeText();
    return c ? ItemType::kEscapedText : ItemType::kEnd;
}

bool XmlParser::next() noexcept
{
    if(!isEnd()) 
    {
		
		// if previous elem ended, remove it from stack

		if (isElementEnd()) _path.popItem();

        _text.clear();			// empty text buffer

        auto c = seek(gt(' ')); // skip ascii blanks

        if(c == '<') // a tag?
        {
			_itemType = loadTag();

            if(isElement()) 
            {
                _path.pushItem(_text) ; // self-closing ones are pushed too, for uniformity
                return true;
            }

			if (!isEnd())
			{
				if (getLevel()) return true;

				// document level
				if (!isSuffix()) return true;
				// end-tag alone:
				_errorCode = ErrorCode::kErrTagUnmatch;
				// another odd thing for the document level would be CDATA, but loadTag() checks
				// for getLevel() - so that if it appears here, it is handled as DTD..
			}
        
			return false;
        }

        if(c) // character data
        {

			_itemType = loadText();


            if (getLevel())  return true;
			
            // document level; skip anything outside elements as BOM or garbage 
            return next(); 
        }

        // eof
        _itemType = ItemType::kEnd;
		
		if (getLevel()) _errorCode = ErrorCode::kErrTagUnmatch;

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
    _text(),
    _tmp()
{
    _chunkSize = bufferSize + (buffer_gran - 1) & ~(buffer_gran - 1);
    _buffer = new char[_chunkSize];
    assign(_buffer, _buffer);
}

bool XmlParser::openFile(const char* path) noexcept
{
    closeFile();  // if open, closes and resets context
	_nReadTotal = 0;
	_errorCode = ErrorCode::kErrOk;
	_eof = false;

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
        _itemType = ItemType::kEnd;  // prevents next()
        _path.clear();
        _text.clear();
        assign(_buffer, _buffer);
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
    if(it.skip()) it.seek_span({' ','>','/'}, false, it);
    return it;
}

bool XmlParser::Path::reference::hasAttributes() const noexcept
{
    charser it(*this);
    return it.seek('=', true) && it.seek('"', true) && it.seek('"');
}

std::vector<XmlParser::Attribute> XmlParser::Path::reference::getAttributes() const noexcept
{
    charser it(*this);
    std::vector<Attribute> v;
    while(it.seek(lt_eq(' ')) && it.seek(gt(' '))) // "[blanks...][non-blank]"
    {
		charser name;
		if(!it.seek_span('=', true, name)) break;
        if (it.getc() != '"') break; // skip leaing quote
		charser value;
		if (!it.seek_span('"', true, value)) break;
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
	std::size_t i = !_offsets.empty() ? _offsets.back() : 0;
    _tags.erase(i);
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
        std::ofstream f(dir.data(), ios_base::out | ios_base::binary);
        if(!f) 
        {
            closeFiles();
            return false;
        }
        push_back(std::move(f));
        dir.erase(pos);
    }
    return true;
}

void XmlParser::FileWriter::write(const char * data, std::size_t n, 
    std::size_t userIndex)  noexcept
{
	at(userIndex).write(data, n);
}

void XmlParser::FileWriter::closeFiles() noexcept
{
    clear();
}

XmlParser::FileWriter::~FileWriter()
{
    closeFiles();
}


