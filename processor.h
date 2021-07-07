#pragma once
#pragma once

#include <cassert>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <vector>
#include "charsor.h"




class XmlParser {



    public:
    /** The result of processing. Bits are combined. */
    enum ExitCode 
    {
        kErrOk = 0,            /**< no errors */
        kErrOpen = 1,       /**< can't open file */
        kErrRead = 2,       /**< error while reading from file */
        kErrClose = 4,      /**< error while closing file. */
        kErrData = 8        /**< tag unmatch etc. */
    };

    /** Atribute of an element. */
    struct  Attribute  {
        std::string_view name; 
        std::string_view value;
    };

    /** Starting tag of an element which is kept as is in the path vector;
        the tag's name and attributes can be extracted when needed
    */
    struct STag: std::string
    { 
        public:
        using std::string::string;
        std::string_view getName() const noexcept;  // extract tag name
        std::vector<Attribute> getAttributes() const noexcept; // extract attributes names / values
    };

    /**  Context of the current element */
    

    class Element
    { 
        public:
        // this structure is only needed to 1) provide better semantics and scope for callbacks;
        // 2) put together those variables which have to be zeroed between two calls to process().

        /** Current element name; calls path.back().getName() */
        std::string_view getName() const noexcept {return path.back().getName();}  
        /** Current element attributes; calls path.back().getAttributes() */
        std::vector<Attribute> getAttributes() const noexcept {return path.back().getAttributes();};  
        /** Nesting level; 0 = document level, 1 = the first element, etc. */ 
        size_t level() noexcept {return path.size();} 
        /** Sequence of start-tags preceding and including current one - which is the last one.*/
        const std::vector<STag>& getPath() const noexcept {return path;}
        /** Get current element start-tag text; calls path.back()*/
        const std::string& getStartTagString() const noexcept {return path.back();} 
        /** Get current element end-tag text; if the actual end-tag is not reached yet, formed 
         artifically by adding '/' to the start-tag's name 
         */
        const std::string& getEndTagString()  noexcept; 

       protected:
        friend class XmlParser;
        

         // stack of tags, the last one is the current one 
        std::vector<STag> path;   
        std::string endTag;  // lazy initialization

        // settings TODO
        bool keepCDataTags = false;     // do not erase "<![CDATA"
        bool keepCharReferences = false; // do not replace "&#xxx" with actual values
        // TODO: the same for token references; for now, nothing related is implemented

        // parsed tag type; set by data getters when they reach the next tag and parse it
        // !!! The order matters !!! to check if >= kCData to skip subitems
        enum TagType{ kNone, kSTag, kETag, kCData, kComment, kPI, kDTD};
        TagType parsedTagType = TagType::kNone;  

        // set only when passing through contents; prevents access to buffer from other callbacks
        bool contentAvailable = false;         
        // set while passing through cData; to take its terminator into account
        bool cData = false;   

    };
    


    private:

    static const size_t buffer_gran = 0x10000;  // read buffer alignment
    static const size_t default_buffer_size = buffer_gran * 4; 
    static const size_t max_tag_length = 0x10000;  // tags that are longer produce error
    
    FILE* input;
    char* buffer;   
    charsor cpos;      // read position
    std::string tagBuffer;     // for tag parsing
    
    size_t exitCode;
    std::size_t nReadTotal;

    Element xml = {};       
    charsor cdpos; // cdata pos
    std::string cDataBuffer;

    size_t read() noexcept;
    template <typename P>
    char parseSeek(P pred) noexcept;
    char parseGetc() noexcept;
    bool parseSkip() noexcept;
    bool parseSeekEndOfCData() noexcept;
    char parsePeekc() noexcept;
    char parseAppendc() noexcept;
    template <typename P>
    char parseAppendEndingWith(P pred) noexcept;
    size_t parseAppendSkipStr(const char* cstr) noexcept;
    bool parseAppendRestOfPI() noexcept;
    bool parseAppendRestOfComment() noexcept;
    Element::TagType parseTag() noexcept;
    void processContent(bool cData) noexcept;
    void processSTag() noexcept;
    void processETag() noexcept;
    bool processTag(Element::TagType t) noexcept;


    protected:


    /** Set global options (can be changed during processing */

   
    /** Should "<![CDATA[", "]]>" be kept (default: false, remove them) */
    void setKeepCDataTags(bool keep) {xml.keepCDataTags = keep;}
    /** Should "&#xxx>" be kept (default: false, replace them with actual values) */
    void setKeepCharReferences(bool keep) {xml.keepCharReferences = keep;}

    bool processNextEnity() noexcept;

    /** Starts processing, calls virtual handlers in a loop and returns ExitCode. */
    int process(const char* path, std::size_t bufferSize = default_buffer_size) noexcept;

    /** Get the result of processing. */
    int getExitCode() const noexcept {return exitCode;} // after processing

    /**  Handlers */ 

    /**  Called on start-tag or self-closing tag; no content is available at this moment */ 
    virtual void onPrefix(const Element& elem, bool isSelfClosing)= 0;
    /**  Called after onPrefix on text contents - potentially a number of times if there are
    text blocks, interleaved with nested tags */ 
    virtual void onContent(const Element& elem)= 0;
    /**  Called after onContent on end-tag  */ 
    virtual void onSuffix(const Element& elem)= 0;
    /**  Called on a <!DTD...> */
    virtual void onDTD(const std::string& text)= 0;
    /**  Called on a <?PI..?> - either in the prolog (then, path is empty) or a nested one. */
    virtual void onPI(const std::string& text, const std::vector<STag>& path) = 0;
    /**  Called on a <!-- comment --> - either in the prolog (then, path is empty) or a nested one. */
    virtual void onComment(const std::string& text, const std::vector<STag>& path) = 0;

    /** Get the number of bytes read from file at this moment. */
    size_t getNumBytesRead() const noexcept {return nReadTotal;}

   
    /** Content getters. These return false or '\0' if the end of available block is reached */
   
    char getChar() noexcept;
    char getChar(std::string& s) noexcept;
    char appendChar(std::string& s) noexcept;
    bool getChars(std::string& s, size_t n = SIZE_MAX) noexcept; 
    bool appendChars(std::string& s, size_t n = SIZE_MAX) noexcept;
    bool getLine(std::string& s, char delim = '\n') noexcept; 
    bool appendLine(std::string& s, char delim = '\n') noexcept; 
};

class Writer {
  //void writeToBuffer(size_t iFile , const char * p, int n) noexcept;
  //void write(int iFile) noexcept;
  //void write(int iFile, int pos, int n) noexcept;
  //void write(int iFile, const char * s, int n) noexcept;
  //template<int N>
  //void write(int iFile, const char(&)[N]) noexcept;

    // write

  /*static const int write_buffer_size = 1024UL * 4;*/

  //struct Output {
  //  FILE* file;
  //  char* buffer; 
  //  char* cpos = buffer;    // points at the free space to write into
  //  char* bufferEnd = buffer; // smth needed anyway for alignment; but bufferEnd will reduce calculations
  //};

 
  //std::vector<Output> outputs;

 /* void closeFiles() noexcept;*/

  //void write(size_t iFile, const char * s, int n) noexcept;

  //void write(size_t iFile, const char*) noexcept;
  //template<int N>
  //void write(size_t iFile, const char(&)[N]) noexcept;

  //bool writeTo(size_t iFile, const char*, int) noexcept;
  //template<int N>
  //bool writeTo(size_t iFile, const char(&)[N]) noexcept;

  //bool writeTo(size_t iFile, char) noexcept;

  //bool writeToNonBlank(size_t iFile) noexcept;
  //bool writeToBlank(size_t iFile) noexcept;

};









