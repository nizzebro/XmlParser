#pragma once

#include "charser.h"
#include <algorithm>
#include <vector>


class XmlParser: private char_parsers::chunk_charser<char,XmlParser> {

    friend class char_parsers::chunk_charser<char, XmlParser>; 

    public:

    /// The result of processing.
    struct ExitCode 
    {
        enum 
        {
            kErrOk = 0,             /// No errors 
            kErrOpenFile = 1,       /// Can't open the file 
            kErrReadFile = 2,       /// Error while reading from the file
            kErrTagUnclosed = 4,    /// A tag without the closing brace met.
            kErrTagUnmatch = 8,    /// An end-tag missing.
            kErrCloseFile = 16      /// Error while closing the file
        };
    };

    /// Entity type. 
    struct EntityType 
    {
        enum
        {
            kNone = 0,              /// Initial state; start of processing 
            kPrefix = 1,            /// Start-tag of an element
            kSuffix = 2,            /// End-tag of an element
            kSelfClosing = 4    ,   /// Self-closing element
            kText = 8,              /// Text block not inside CDATA 
            kCData = 16,            /// Text block inside CDATA
            kPI = 32,               /// Processing instruction
            kComment = 64,          /// Comment
            kDTD = 128,             /// Document Type Declaration
            kEnd = 256,             /// End-of-file or error  

            kElementStartMask   = kPrefix | kSelfClosing,
            kElementEndMask     = kSuffix | kSelfClosing,
            kTextEntityMask     = kText | kCData
        };
    }; 

    ///  Atribute of an element
    struct  Attribute  {
        std::string_view name; 
        std::string_view value;
    };

    /// A class that accepts the extracted data e.g. to write it to a file
    struct IWriter
    {
        /// Is called from data writing functions; userIndex can be used e.g.
        /// to choose a particular file or data container.
        virtual void write(const char* data, std::size_t n, std::size_t userIndex) = 0;
    };


    /// Opens file; if open, calls process() and returns the exit code. 
    int process(const char* path, std::size_t bufferSize = default_chunk_size) noexcept;

    /// Callback to implement; all processing is done from here */
    virtual void process() = 0;

    ///@{
    /** Current state */

    /// True: end-of-file, can't continue.
    bool eof() const noexcept {return _eof;}  
    /// True: error; can't continue. Can be casted to bool.
    int error() const noexcept {return _exitCode;}  
    /// Returns !error() && !eof() 
    bool good() const noexcept {return !error() && !eof();}

    /// Gets the number of bytes processed at this moment. 
    std::size_t getNumBytesProcessed() const noexcept 
    { return _nReadTotal - size(); }

    /// If false (default), escaped symbols are replaced with their actual
    ///  values and CDATA tags are removed, otherwise, all text goes as is.
    void setKeepEscaped(bool keep) noexcept { _keepEscaped = keep; }

    ///}@

    ///@{
    /** Traversing entities */

    /// Loads next entity.
    /// \return False if the end of file is reached or data error occured -
    /// in either case furher processing is not possible.
    /// \detail 
    /// The iterated items are: all element's tags (start, end, and 
    /// self-closing ones), text blocks, CDATA text blocks, processing 
    /// instuctions (PI), comments, type declarations (DTD). Text blocks may 
    /// come more than once if interleaved with nested entities or enclosed 
    /// by CDATA-s. 
    

    bool next() noexcept;

    /// Skips current element until the next sibling, the parent end-tag, or eof. 
    void skipElement() noexcept;

    /// Passes current entity's text to IWriter and performs next().
    void writeEntity(IWriter& writer, std::size_t userIndex);
    /// Passes current element's text, including all tags, to IWriter and
    /// walks to the next element.
    /// \param keepEscaping  If false, escaped symbols are replaced with their
    /// actual values and CDATA tags are removed, otherwise, all text goes as is.
    void writeElement(IWriter& writer, bool keepEscaped, std::size_t userIndex);
    /// Same as above but handles escaped data according to the global settings.
    void writeElement(IWriter& writer, std::size_t userIndex) 
    {
        writeElement(writer, userIndex, _keepEscaped);
    }

    ///}@

    ///@{
    /** Type of the current entity */

    /// Checks if the entity is either a start-tag or a self-closing tag.
    bool isElement() const noexcept 
    {return (_entityType & EntityType::kElementStartMask);}

    /// Checks if the entity is a self-closing tag.
    bool isSelfClosing() const noexcept 
    {return (_entityType == EntityType::kSelfClosing);}

    /// Same as ! isSelfClosing().
    bool isPrefix() const noexcept 
    {return (_entityType == EntityType::kPrefix);}

    /// Checks if the entity is an end-tag.
    bool isSuffix() const noexcept 
    {return (_entityType == EntityType::kSuffix);}

    /// Checks if the entity is a text block, including CDATA blocks.
    bool isText() const noexcept 
    {return (_entityType & EntityType::kTextEntityMask);}

    /// Checks if the entity is a CDATA block.
    bool isCDATA() const noexcept 
    {return (_entityType & EntityType::kCData);}

    /// Same as ! isCDATA().
    bool isEscapedText() const noexcept 
    {return (_entityType & EntityType::kText);}

    /// True if the entity  is a Processor Instruction.
    bool isPI() const noexcept 
    {return (_entityType == EntityType::kPI);}

    /// True if the entity  is a comment.
    bool isComment() const noexcept 
    {return (_entityType == EntityType::kComment);}

    /// True if the entity is a Document Type Declaration.
    bool isDTD() const noexcept 
    {return (_entityType == EntityType::kDTD);}

    /// True if eof or error, same effect as ! good().
    /// further processing is not available.
    bool isEnd() const noexcept 
    {return (_entityType == EntityType::kEnd); }
    
    /// Current item type, see EntityType.
    int getType() const noexcept {return _entityType; }

    ///}@

    ///@{
    /** Current element's properties. */

    /// Current element's name.
    std::string_view getName() const noexcept {return getName(level());}

    ///  Current element's attributes.
    std::vector<Attribute> getAttributes() const noexcept {return getAttributes(level());};

    ///  Current element's start-tag string.
    std::string_view getSTagString() const noexcept
    {return getSTagString(level());};

    /// Get text of currently loaded text block, end-tag, PI, DTD or comment.
    const std::string& getText() const noexcept {return _text;}

    ///}@

    ///@{
    /** Current path, level, and parent elements's properties. */

    /// Path depth; 0: the document level, 1: the first element, etc. 
    std::size_t level() const noexcept { return _path.size() - 2;}

    /// Name of any element which is a part of current path;
    ///\param index <= level() (if greater or 0, returns an empty string)
    std::string_view getName(std::size_t idx) const noexcept;

    /// Attributes of an element which is a part of current path; index <= level();
    ///\param index <= level() (if greater or 0, returns an empty string)
    std::vector<Attribute> getAttributes(std::size_t idx) const noexcept;

    /// Start-tag string of any element which is a part of current path;
    ///\param index <= level() (if greater or 0, returns an empty string) 
    std::string_view getSTagString(std::size_t idx) const noexcept;

    ///}@


    private:
    
    static const int buffer_gran = 0x10000;  // read buffer alignment
    static const int default_chunk_size = buffer_gran * 4; 
    //static const int max_tag_length = 0x100000;  // unused yet
   
    FILE* _input;
    int _chunkSize;
    std::size_t _nReadTotal;
    int _exitCode;
    bool _eof;

    //   Current path: stack of start-tags
    //   Tags are saved appended to each other in one single std::string
    //   (_tagStrings). The offset of each tag is _path[level()].
    //   The offset to the end of tag is _path[level() + 1].
    //   E.g.: "<root><cat><mouse>" - _tags are {0,0,4,7,12}.
    //   Two nulls, assigned in on start, are needed as there are no 
    //   tags on level 0; the level() method returns _path.size() - 2.
  

    std::vector<std::size_t> _path; 
    std::string _tagStrings; // start-tag strings 
    int _entityType;

    std::string _text;   
    bool _keepEscaped;
     
    bool loadNextChunk() noexcept;
    bool appendRestOfPI() noexcept;
    bool appendRestOfComment() noexcept;
    bool appendRestOfCDATA() noexcept;
    int loadTag() noexcept;
    int loadText() noexcept;
    void pushPrefix() noexcept;
    void popPrefix() noexcept;
};


class Writer {
  //void writeToBuffer(std::size_t iFile , const char * p, int n) noexcept;
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

  //void write(std::size_t iFile, const char * s, int n) noexcept;

  //void write(std::size_t iFile, const char*) noexcept;
  //template<int N>
  //void write(std::size_t iFile, const char(&)[N]) noexcept;

  //bool writeTo(std::size_t iFile, const char*, int) noexcept;
  //template<int N>
  //bool writeTo(std::size_t iFile, const char(&)[N]) noexcept;

  //bool writeTo(std::size_t iFile, char) noexcept;

  //bool writeToNonBlank(std::size_t iFile) noexcept;
  //bool writeToBlank(std::size_t iFile) noexcept;

};









