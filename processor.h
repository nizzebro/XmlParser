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


    /// Processing options, bits are combined.
    struct Options
    {
        enum
        {
            kKeepMnenonics = 1,  /// Keep mnemonics (otherwise replaced with actual values) 
            kKeepCDATAtags = 2,  /// Keep CDATA tags (otherwise removed)
            kDefault = 0
        };
    };

    /// Get and set currently used options.
    int getOptions() const noexcept { return _options; }
    void setOptions(int v) noexcept {_options = v;}

    /// Processing errors
    enum struct ErrorCode 
    {
        kErrOk = 0,            /// No errors 
        kErrOpenFile = 1,      /// Can't open file.
        kErrReadFile  = 2,     /// Can't read from file.
        kErrTagUnclosed = 4,   /// A tag without the closing brace.
        kErrTagUnmatch = 8     /// An end-tag is missing.
    };

    /// Types of entities
    enum struct EntityType 
    {
                     
        kEnd = 0,               /// End-of-file or error
        kPrefix = 1,            /// Start-tag of an element
        kSuffix = 2,            /// End-tag of an element
        kSelfClosing = 4    ,   /// Self-closing element
        kEscapedText = 8,       /// Text block not inside CDATA 
        kCData = 16,            /// Text block inside CDATA
        kPI = 32,               /// Processing instruction
        kComment = 64,          /// Comment
        kDTD = 128,             /// Document Type Declaration
        kBegin = 256            /// Initial state, the start of processing 
    }; 

    /// Atribute of an element
    struct  Attribute  {
        std::string_view name; 
        std::string_view value;
    };


    ///@{ Current state of the processor

    /// True: end-of-file, cannot continue
    bool eof() const noexcept {return _eof;}  

    /// True: error, processing cannot be continued.
    bool error() const noexcept {return _errorCode != ErrorCode::kErrOk;}  

    /// Gets the error code
    ErrorCode getErrorCode() const noexcept { return _errorCode;}

    /// Returns !error() && !eof() 
    bool good() const noexcept {return !error() && !eof();}

    /// Gets the number of bytes processed at this moment. 
    std::size_t getFilePos() const noexcept 
    { return _nReadTotal - size(); }
 
    ///}

    ///@{
    /** Type and text of current entity */

    const EntityType& getEntityType() const noexcept { return _entityType; }

    /// Checks if the entity is either a start-tag or a self-closing tag.
    bool isElement() const noexcept 
    {return ((int)_entityType & ((int)EntityType::kPrefix | 
        (int)EntityType::kSelfClosing));}
    /// Checks if the entity is element of specific name.

    /// Checks if the entity is a self-closing tag.
    bool isSelfClosing() const noexcept 
    {return (_entityType == EntityType::kSelfClosing);}
  
    /// Same as ! isSelfClosing().
    bool isPrefix() const noexcept 
    {return (_entityType == EntityType::kPrefix);}

    /// Checks if the entity is an end-tag.
    bool isSuffix() const noexcept 
    {return (_entityType == EntityType::kSuffix);}

    /// Checks if the entity is either an end-tag or a self-closing tag.
    bool isElementEnd() const noexcept 
    {return ((int)_entityType & ((int)EntityType::kSuffix |
        (int)EntityType::kSelfClosing));}

    /// Checks if the entity is a text block, including CDATA blocks.
    bool isText() const noexcept 
    {return ((int)_entityType & ((int)EntityType::kEscapedText | 
        (int)EntityType::kCData));}

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
    

    /// Text of the current entity.
    /// \detail For a text block, gets the text; for any kind of tag
    /// gets the tag's text including angle brackets.
    /// For EntityType::kEnd, the text is either empty or contains 
    /// an incomplete tag which produced error.
    const std::string& getText() const noexcept { return _text; }


    ///}@

    ///@{
    /** Properties of current element */

    /// Current element's start tag
    std::string_view getStartTag() const noexcept;

    /// Current element's name; valid while current entity
    ///   is this element's start-tag, text, comment, PI or end-tag
    std::string_view getName() const noexcept;


    ///  True if current element has attributes; valid while current
    ///  entity is this element's start-tag, text, comment, PI or end-tag
    bool hasAttributes() const noexcept;

    ///  Current element's attributes; valid while current entity
    ///   is this element's start-tag, text, comment, PI or end-tag
    std::vector<Attribute> getAttributes() const noexcept;

    ///}@

    ///@{
    /** Current path, level, and parent elements's properties. */

    /// Path depth; 0: document level; 1: root element etc.
    /// \detail 
    ///     <?xml....?>         // level = 0
    ///     <!DOCTYPE...>       // level = 0
    ///     <!-- bla bla -->    // level = 0
    ///     <root>              // level = 1 - new element
    ///       bla bla           // level = 1
    ///       <!-- bla bla -->  // level = 1
    ///       <?php...?>        // level = 1
    ///       <group>           // level = 2 - new element
    ///         bla bla         // level = 2
    ///         <thingy/>       // level = 3 - new, even self-closing
    ///         bla bla         // level = 2 - back
    ///         <group>         // level = 3 
    ///           bla bla       // level = 3
    ///         </group>        // level = 3
    ///         <thingy/>       // level = 3 
    ///       </group>          // level = 2 - back
    ///     </root>             // level = 1 - back
    ///                         // level = 0 - EOF

    std::size_t level() const noexcept { return _path.size();}

    /// Full start-tag string enclosing element or empty string if root
    std::string_view getParentStartTag() const noexcept;

    /// Name of enclosing element or empty string if root
    std::string_view getParentName() const noexcept;

    ///  True if enclosing element has attributes
    bool hasParentAttributes() const noexcept;

    /// Attributes of enclosing element or empty vector if root
    std::vector<Attribute> getParentAttributes() const noexcept;

    /// Start-tag string of an upper-level element of current path
    ///\param index should be <= level(); if 0, returns an empty string
    std::string_view getStartTag(std::size_t lvl) const noexcept;

    /// Name of an upper-level element of current path
    ///\param index should be <= level(); if 0, returns an empty string
    std::string_view getName(std::size_t lvl) const noexcept;

   ///  True if an upper-level element has attributes
    bool hasAttributes(std::size_t lvl) const noexcept;

    /// Attributes of an upper-level element of current path
    ///\param index should be <= level(); if 0, returns an empty string
    std::vector<Attribute> getAttributes(std::size_t lvl) const noexcept;


    ///}@


    ///@{
    /** Traversing entities */

    /// Loads next entity.
    /// \return False if EOF is reached or data error occured -
    /// in either case furher processing is not possible.
    /// \detail 
    /// The iterated items are: all element's tags (start, end, and 
    /// self-closing ones), text blocks, CDATA text blocks, processing 
    /// instuctions (PI), comments, type declarations (DTD). Text blocks may 
    /// come more than once if interleaved with nested entities or enclosed 
    /// by CDATA-s. 
    
    bool next() noexcept;

    /// Loads next entity in bounds of a specified level.
    /// \param level Level. 
    /// \return False if the end-tag of an element of this level or EOF is 
    /// reached, or data error ocurred. 
    bool next(std::size_t lvl) noexcept 
    { return !(isElementEnd() && level() == lvl) ? next() : false;}


    ///}@


    ///@{ Write


    /// Interface intended to copy data e.g. to write it to a file
    struct IWriter
    {
        /// Callback for data writing functions 
        /// \param data Poiter to the data to write
        /// \param n Size of data, bytes
        /// \param userIndex Can be used e.g.to choose a particular file or data container. 
        virtual void write(const char* data, std::size_t n, std::size_t userIndex) = 0;
    };

    /// A simple IWriter implementation to write to files
    class FileWriter
    {
        public:
        bool openFiles(std::string dir, std::initializer_list<const char*>filenames) noexcept;
        void closeFiles() noexcept;
        ~FileWriter();

        virtual void write(const char* data, std::size_t n, std::size_t iFile) noexcept;
        private:
            std::vector<FILE*> outputs; 
    };

    /// Passes current entity's text to IWriter and performs next().
    bool writeEntity(IWriter& writer, std::size_t userIndex);
  

    /// Passes the current element's content, including all tags, to IWriter and
    /// stops at the end-tag of this element.
    void writeElement(IWriter& writer, std::size_t userIndex);



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

    bool loadNextChunk() noexcept;
    bool appendRestOfPI() noexcept;
    bool appendRestOfComment() noexcept;
    bool appendRestOfCDATA() noexcept;
    EntityType loadTag() noexcept;
    EntityType loadText() noexcept;
    void pushElement() noexcept;
    void popElement() noexcept;
    static std::string_view getNameFromTag(std::string_view) noexcept;
    static bool hasTagAttributes(std::string_view s) noexcept;
    static std::vector<Attribute> getAttributesFromTag(std::string_view) 
        noexcept;
};


