#pragma once

#include "charser.h"
#include <algorithm>
#include <vector>


class XmlParser: private char_parsers::chunk_charser<char,XmlParser> {

    friend class char_parsers::chunk_charser<char, XmlParser>; 
    static const int buffer_gran = 0x10000;  // read buffer alignment

    public:
   
    static const int default_chunk_size = buffer_gran * 4; 

    /// Constructor. Allocates memory for the file buffer.
    ///\ param bufferSize Size of file buffer.
    XmlParser(std::size_t bufferSize = default_chunk_size) noexcept;

    /// Opens a file for processing. A previous file will be closed.  
    ///\ param path Full path to the file.
    /// \return True if opened succesfully, false otherwise
    bool openFile(const char* path) noexcept;

    /// Closes the opened file. This is done automatically by openFile 
    /// and destructor.
    void closeFile() noexcept;

    /// Destructor. Closes any opened file and frees the buffer.
    ~XmlParser();


    /// Values for processing options, bits are combined.
    struct Options
    {
        enum
        {
            kReplaceMnenonics = 1,  /// Replace mnemonics with actual values 
            kStripCDATA = 2,        /// Remove CDATA tags
            kDefault = kReplaceMnenonics + kStripCDATA
        };
    };

    /// Get and set currently used options.
    int getOptions() const noexcept { return _options; }
    void setOptions(int v) noexcept {_options = v;}

    /// The result of processing
    enum struct ErrorCode 
    {
        kErrOk = 0,             /// No errors 
        kErrOpenFile = 1,
        kErrReadFile  = 2,        /// Can't read from file.
        kErrTagUnclosed = 4,    /// A tag without the closing brace met.
        kErrTagUnmatch = 8     /// An end-tag missing.
    };

    /// Entity type
    enum struct EntityType 
    {
        kNone = 0,              /// Initial state; start of processing 
        kPrefix = 1,            /// Start-tag of an element
        kSuffix = 2,            /// End-tag of an element
        kSelfClosing = 4    ,   /// Self-closing element
        kEscapedText = 8,              /// Text block not inside CDATA 
        kCData = 16,            /// Text block inside CDATA
        kPI = 32,               /// Processing instruction
        kComment = 64,          /// Comment
        kDTD = 128,             /// Document Type Declaration
        kEnd = 256             /// End-of-file or error  
    }; 

    /// Atribute of element
    struct  Attribute  {
        std::string_view name; 
        std::string_view value;
    };


    ///@{ Current state

    /// True: end-of-file, can't continue.
    bool eof() const noexcept {return _eof;}  

    /// True: error, processing cannot be continued. 
    bool error() const noexcept {return getErrorCode() != ErrorCode::kErrOk;}  

    ErrorCode getErrorCode() const noexcept { return _errorCode;}

    /// Returns !error() && !eof() 
    bool good() const noexcept {return !error() && !eof();}

    /// Gets the number of bytes processed at this moment. 
    std::size_t getFilePos() const noexcept 
    { return _nReadTotal - size(); }
 
    ///}

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

    ///}@

    ///@{
    /** Type of current entity */

    /// Checks if the entity is either a start-tag or a self-closing tag.
    bool isElement() const noexcept 
    {return ((int)_entityType & ((int)EntityType::kPrefix | (int)EntityType::kSelfClosing));}

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
    {return ((int)_entityType & ((int)EntityType::kEscapedText | (int)EntityType::kCData));}

    /// Checks if the entity is a CDATA block.
    bool isCDATA() const noexcept 
    {return (_entityType == EntityType::kCData);}

    /// Same as ! isCDATA().
    bool isEscapedText() const noexcept 
    {return (_entityType == EntityType::kEscapedText);}

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
    EntityType getType() const noexcept {return _entityType; }

    ///}@

    ///@{
    /** Current element's properties. */

    /// Current element's name
    std::string_view getName() const noexcept {return getName(level());}

    ///  Current element's attributes.
    std::vector<Attribute> getAttributes() const noexcept {return getAttributes(level());};

    ///  Current element's start-tag string.
    std::string_view getSTagString() const noexcept
    {return getSTagString(level());};

    /// The text of current entity, whether a text block or a tag;
    /// For a tag, gives its full string including angle brackets. 
    /// For EntityType::kEnd, the text is either empty or contains 
    /// an incomplete tag which produced the error.
    const std::string& getText() const noexcept {return _text;}

    ///}@

    ///@{
    /** Current path, level, and parent elements's properties. */

    /// Path depth; 0: the document level, 1: the first element, etc. 
    std::size_t level() const noexcept { return _path.size() - 2;}

    /// Name of any element which is a part of current path;
    ///\param index should be <= level(); if 0, returns an empty string
    std::string_view getName(std::size_t idx) const noexcept;

    /// Attributes of an element which is a part of current path; index <= level();
    ///\param index should be <= level(); if 0, returns an empty string
    std::vector<Attribute> getAttributes(std::size_t idx) const noexcept;

    /// Start-tag string of any element which is a part of current path;
    ///\param index should be <= level(); if 0, returns an empty string
    std::string_view getSTagString(std::size_t idx) const noexcept;

    ///}@

    ///@{ Extras

    /// Skips current element until the next sibling, the parent end-tag, or eof. 
    void skipElement() noexcept 
    {    auto i = level();
         while(next() && level() >= i) {}
    }

    /// Interface intended to copy the data e.g. to write to a file
    struct IWriter
    {
        /// Callback for data writing functions 
        /// \param data Poiter to the block to write
        /// \param n Size of data, bytes
        /// \param userIndex Can be used e.g. to choose a particular file or data container. 
        /// \return True if operation was succesful, false if error occured.
        virtual void write(const char* data, std::size_t n, std::size_t userIndex) = 0;
    };

    /// Passes current entity's text to IWriter and performs next().
    void writeEntity(IWriter& writer, std::size_t userIndex)
    {
        writer.write(_text.data(), _text.size(), userIndex);
        next();
    }

    /// Passes current element's text, including all tags, to IWriter and
    /// walks to the next element.
    void writeElement(IWriter& writer, std::size_t userIndex)
    {
        do { writer.write(_text.data(), _text.size(), userIndex); } 
        while (next());
    }

    /// A simple ready-to-use IWriter implementation to write to files
    class FileWriter
    {
        public:
        bool openFiles(const char* path, std::initializer_list<const char*>filenames) noexcept;
        void closeFiles();
        ~FileWriter();

        virtual void write(const char* data, std::size_t n, std::size_t iFile) noexcept;
        private:
        std::vector<FILE*> outputs; 
    };


    ///}@



    private:
    

    //static const int max_tag_length = 0x100000;  // unused yet
   
    char* _buffer;
    int _chunkSize;
    FILE* _input;
    ErrorCode _errorCode;
    std::size_t _nReadTotal;
    bool _eof;
    int _options;
  
    std::string _tags; // stack of start-tags appended in one string 
    std::vector<std::size_t> _path;  // Stack of tag offsets in _tags
    
    EntityType _entityType;

    std::string _text;   
    bool _keepEscaped;
     
    bool loadNextChunk() noexcept;
    bool appendRestOfPI() noexcept;
    bool appendRestOfComment() noexcept;
    bool appendRestOfCDATA() noexcept;
    EntityType loadTag() noexcept;
    EntityType loadText() noexcept;
    void pushPrefix() noexcept;
    void popPrefix() noexcept;
};


