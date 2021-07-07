
#include "pch.h"
#include "Processor.h"

#define _CRT_SECURE_NO_WARNINGS


using namespace std;


//bool Processor::skip(int n)
//{
//  do {
//    n -= currentTextLength();
//    if(n > 0) {
//      cpos = endPos;
//      return true;
//    }
//  } while(readToBuffer());
//  return false;
//}
//
//
////  PER-FILE SEARCH SKIPPING DATA; advances endPos = cpos
//
//bool Processor::skipTo(const char* s, int n) noexcept {
//  do {
//    if(moveTo(s, n)) {
//      cpos = endPos;
//      return true;
//    }
//  } while(readToBuffer());
//  return false;
//}
//
//
//template<int N>
//bool Processor::skipTo(const char (&s)[N]) noexcept {
//  Processor::skipTo((const char*)s, N-1);
//}
//
//bool Processor::skipTo (char c) noexcept {
//  do {
//      if(moveTo(c)) {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}
//
//bool Processor::skipToNonBlank() noexcept {
//   do {
//      if(moveToNonBlank()) {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}
//
//bool Processor::skipToBlank() noexcept {
//  do {
//      if(moveToBlank()) {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}
////
////
//
////  WRITE; doesn't affect pointers
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
//bool Processor::writeToNonBlank(size_t iFile) noexcept {
//  do {
//      bool b = moveToNonBlank();
//      Processor::write(iFile, cpos, (int)(endPos - cpos));
//      if(b)  {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}
//
//bool Processor::writeToBlank(size_t iFile) noexcept {
//  do {
//      bool b = moveToBlank();
//      Processor::write(iFile, cpos, (int)(endPos - cpos));
//      if(b)  {
//        cpos = endPos;
//        return true;
//      }
//    } while(readToBuffer());
//  return false;
//}



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


// reads a chunk from file to buffer, setting position pointer 
// to the beginning of the buffer, returns n bytes read; 0 = eof

size_t XmlParser::read() noexcept
{
  size_t nRead = _fread_nolock(buffer, 1, cpos.end() - buffer, input);
  nReadTotal += nRead; 
  if(nRead != (cpos.end() - buffer)) 
  {
      if(ferror(input)) exitCode |= kErrRead;
  }
  cpos.assign(buffer, buffer + nRead);
  return nRead;
}

// searches for char satisfying a condition;
// increments position pointer, stops on the char found or eof 
// returns the char found or 0: eof 
template <typename P>
char XmlParser::parseSeek(P pred) noexcept 
{
    do { auto c = cpos.seek_if(pred); if(c) return c; }
    while(read());
    return 0;
}

// safely gets current char or 0 on eof; no increment
char XmlParser::parsePeekc() noexcept
{
    auto c = cpos.peekc(); 
    if(c) return c;
    read(); 
    return cpos.peekc(); 
}

// safely gets current char or 0 on eof;  increment
char XmlParser::parseGetc() noexcept
{
    auto c = cpos.getc(); 
    if(c) return c;
    read(); 
    return cpos.getc(); 
}

bool XmlParser::parseSkip() noexcept 
{
    auto b = cpos.skip();
    if(b) return b;
    read();
    return cpos.skip();
}


// appends one char to tagBuffer and increments position;
// returns the appended char or 0 on eof
char XmlParser::parseAppendc() noexcept {

    auto c = cpos.appendc(tagBuffer);
    if(c) return c;
    read();
    return cpos.appendc(tagBuffer);
}

// appends chars to tagBuffer until meets a char that satisfies a condition
// appends that char either
// returns that last char or 0 when either eof or size limit is reached
template <typename P>
char XmlParser::parseAppendEndingWith(P pred) noexcept
{   
    do {
        auto c = cpos.seek_append_if(pred, tagBuffer);
        if (c) 
        {
            tagBuffer += c;  
            cpos.skip();
            return c;
        }
    } while (tagBuffer.size() < max_tag_length && read());

    return 0;
}


size_t XmlParser::parseAppendSkipStr(const char* cstr) noexcept
{   
    assert (*cstr);
    auto n = 0;
    do {
        n += cpos.skipstr_append(cstr, tagBuffer);
        if(!cpos.empty()) break;
        cstr += n;
     } while (read());  
    return n;
}

 
bool XmlParser::parseAppendRestOfComment() noexcept 
{   
    while(auto c = parseAppendEndingWith(charsor::eq<'-'>)) 
    {
        while ((c = parseAppendc())  == '-') {} // skip potential "-----..."
        if (c == '>') return true; 
        if (!c) break;
    } 
    return false;    
}

// appends chars to tagBuffer ending with "?>"
// the curr. position should be next after "<?"
// returns false on eof

bool XmlParser::parseAppendRestOfPI() noexcept 
{   
    while (auto c = parseAppendEndingWith(charsor::eq<'?'>))
    {
        if ((c = parseAppendc()) == '>') return true;
        if(!c) break;
    }
    return false;    
}

// reads tag to tagBuffer, advancing position to the next after it
// the curr. position should be next on "<"
// returns tag code (on error, Element::TagType::kNone = 0)

XmlParser::Element::TagType XmlParser::parseTag() noexcept 
{
   tagBuffer.clear();
   parseAppendc(); // '<'; 
   auto c = parseAppendc();
   if(c == '/') // End-tag: "</" (any chars except '>') '>' 
   {
       if(parseAppendEndingWith(charsor::eq<'>'>))
           return Element::TagType::kETag;

   }
   else if(c == '?') // PI (Processor Instruction): "<?" (any chars except "?>") "?>" 
   {
       if(parseAppendRestOfPI()) return Element::TagType::kPI;  
       
   }
   else if(c == '!') // comment or DTD or CData: "<!" (any chars, comments or PI's ) '>' 
   {
       // comments are: "<!--" (any chars except "-->") "-->"
       // cData's are: "<[CDATA[" (any chars except "]]>") "]]>"
       // DTD's are: "<!" (not only any chars except '>', but also nested DTD's, PI's and comments) ">"

       // check if it is a comment, CData or DTD

        auto c = parsePeekc(); // lookup to avoid removing next '<' from queque (can be DTD's nested elements)

        if (c == '-') // "<!-" check for a comment
        {
            parseSkip();
            c = parsePeekc(); // lookup again
            if (c == '-') // "<!--" yup, comment
            {
                parseSkip();
                if(parseAppendRestOfComment()) 
                    return Element::TagType::kComment;
            }
        } 
        else if(c == '[') // load all cdata in buffer
        {
            if(parseAppendSkipStr("CDATA[") == sizeof("CDATA["))
            {
                //if (!xml.keepCDataTags)
                    tagBuffer.clear();
                while (parseAppendEndingWith(charsor::eq<']'>))
                {
                    if(parseAppendSkipStr("]>") == sizeof("]>"))
                    {
                        //if (!xml.keepCDataTags) 
                            tagBuffer.erase(tagBuffer.length() - 3);
                        return  Element::TagType::kCData; 
                    }
                }
                
            }
                
        }
        else
        {
            int iNested = 1;        // count matching '< >'
            while (c = parseAppendEndingWith (charsor::eq<'<','>'>))
            {
                if (c == '<')  // "...<"
                {   
                    c = parseAppendc(); 

                    if(c == '!') // "...<!"
                    {
                        auto c = parsePeekc();
                        if(c == '-') // "...<!-" nested comment?
                        {
                            parseSkip();
                            c = parsePeekc(); 
                            if (c == '-') // "...<!--" yup, nested comment
                            {
                                parseSkip();
                                if(!parseAppendRestOfComment()) break;   
                            }
                        } 
                    }

                    else if(c == '?') // "...<?"
                    {
                        if(!parseAppendRestOfPI()) break;
                    }

                    else if(!c) break;

                    else ++iNested;          
                }
                else // '...>'
                {  
                    if((--iNested) == 0) return Element::TagType::kDTD;

                } // end if '<' or '>'

            } // end while is_of('<', '>')
        } // end else (dispatch the char next after "<!")
   } 
   else if(c)
   {
        if(parseAppendEndingWith(charsor::eq<'>'>)) return Element::TagType::kSTag;
   }

    return  Element::TagType::kNone;   
}



void XmlParser::processContent(bool cData) noexcept 
{
    xml.contentAvailable = true;
    onContent(xml);
    xml.cData = cData;
    if (cData) cdpos = tagBuffer;

    if (xml.contentAvailable)
    {
        parseSeek(charsor::eq<'<'>);
        xml.contentAvailable = false;
    }

}

void XmlParser::processSTag() noexcept
{
    bool isSelfClosing = tagBuffer[tagBuffer.size() - 1] == '/';
    xml.path.push_back(STag(std::move(tagBuffer)));
    onPrefix(xml, isSelfClosing);
    if(isSelfClosing) xml.path.pop_back();
}


void XmlParser::processETag() noexcept
{
    if(xml.level())
    {
        xml.endTag = std::move(tagBuffer);
        onSuffix(xml);
        xml.path.pop_back();
    }
}
 
bool XmlParser::processTag(XmlParser::Element::TagType t) noexcept 
{
    switch (t)
    {
        case Element::TagType::kSTag: 
            processSTag();
            return true;
        case Element::TagType::kETag:
            processETag();
            return true;
        case Element::TagType::kCData: 
            processContent(true);
            return true;
        case Element::TagType::kPI: 
            onPI(tagBuffer, xml.path); 
            return true; 
        case Element::TagType::kComment: 
            onComment(tagBuffer, xml.path); 
            return true; 
        case Element::TagType::kDTD: // should not appear here but anyway
            onDTD(tagBuffer);
            return true; 
    }

    return false;
}


bool  XmlParser::processNextEnity() noexcept 
{
    auto t = xml.parsedTagType;

    if(t != Element::TagType::kNone) 
    {
        xml.parsedTagType = Element::TagType::kNone;
        return processTag(t);
    }


    auto c = parseSeek(charsor::gt<' '>);

    if(c == '<') return processTag(parseTag());
    if(c) {
        processContent(false);
        return true;
    }

    return false;
}

int XmlParser::process(const char* path, size_t bufferSize) noexcept
{
    nReadTotal = 0;
    input = fopen(path, "rb");
    exitCode = setvbuf(input,  nullptr, _IONBF, 0 ) == 0 ? kErrOk : kErrOpen;
    if(exitCode == kErrOk) 
    {
        bufferSize = (bufferSize + (buffer_gran - 1)) & (~(buffer_gran - 1));
        buffer = new char [bufferSize];
        cpos.assign(buffer, buffer + bufferSize);
        if(read()) // cpos assignment is inside read()
        {
            while(processNextEnity()) {}  
        }
        delete [] buffer;
        xml = {};
    }
    if(input && (fclose(input) != 0)) { exitCode |= kErrClose; }
    return exitCode;
}


std::string_view XmlParser::STag::getName() const noexcept
{
    auto p = data() + 1;
    charsor it(p, data() + size());
    it.seek_if(charsor::eq<' ','>','/'>);
    return std::string_view(p, it.get() - p);
}

std::vector<XmlParser::Attribute> XmlParser::STag::getAttributes() const noexcept
{
    auto p = data() + 1;
    charsor it(p, data() + size());
    std::vector<Attribute> v;
    while(it.seek(' '))
    {
        it.skip();
        p = it.get();
        if(!it.seek('=')) break;
        std::string_view name(p, it.get() - p); 
        it.skip();
        if(it.getc() != '"') break;
        p = it.get();
        if(!it.seek('"')) break;
        std::string_view value(p, it.get() - p); 
        it.skip(); 
        v.push_back(Attribute{name, value});
    }
    return std::move(v);
}

const std::string & XmlParser::Element::getEndTagString() noexcept
{
    if(endTag.empty())
    {
       endTag += '<';
       endTag += '/';
       endTag.append(getName());
       endTag  += '>';
    }
    return endTag;
}

char XmlParser::getChar() noexcept
{
    if(xml.contentAvailable)
    {
        
        if(!xml.cData)
        {
            auto c = parsePeekc();

            if (c != '<') return parseGetc();
           
            auto t = parseTag();
            if(t == Element::TagType::kCData)
            {
                xml.cData = true;
                cdpos = tagBuffer;
                return getChar();
            }
                
            xml.contentAvailable = false; // end of block
            xml.parsedTagType = t;    // save 

            return 0;
        }
        else
        {
           auto c = cdpos.getc();
           if (c) return c;
           xml.cData = false;
           return getChar();
        }
    }
    return 0;
}

char XmlParser::getChar(std::string & s) noexcept
{
    auto c = getChar();
    if(c) s = c;
    return c;
}

char XmlParser::appendChar(std::string & s) noexcept
{
    auto c = getChar();
    if(c) s += c;
    return c;
}

bool XmlParser::getChars(std::string & s, size_t n) noexcept
{
    s.clear();
    return appendChars(s, n);
}

bool XmlParser::appendChars(std::string & s, size_t n) noexcept
{
    char c;
    while(n && (c = getChar())) {s += c;  --n;}
    return !n;
}

bool XmlParser::getLine(std::string & s, char delim) noexcept
{
    s.clear();
    return appendLine(s, delim);
}

bool XmlParser::appendLine(std::string & s, char delim) noexcept
{
    while(auto c = getChar())
    {
        if (c != delim)
        {
             s += c;
             continue;
        }
       return true;
    };
    return false;
}
