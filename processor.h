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
    enum struct ItemType 
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

    struct Path 
    {
        struct reference: std::string_view
        {
            using std::string_view::string_view;
            /// gets start-tag's text;
            std::string_view getStartTag() const noexcept;
            /// gets start-tag's name
            std::string_view getName() const noexcept;
            ///  Returns true if start-tag has attributes
            bool hasAttributes() const noexcept;
            /// gets start-tag's attributes
            std::vector<Attribute> getAttributes() const noexcept;
        };
        struct iterator
        {
            bool operator!=(const iterator& rhs) const noexcept { return _idx != rhs._idx; }
            void operator++() noexcept { ++_idx; }
            const Path::reference operator*() const noexcept { return _path[_idx]; }
            private:
            friend typename XmlParser::Path;
            iterator(const Path& path, std::size_t i):_path(path), _idx(i){}
            const Path& _path;
            std::size_t _idx;
        };
        // Checks if path is empty
        bool empty() const noexcept{ return _offsets.empty(); }
        // Gets the number of elements in path
        std::size_t size() const noexcept{ return _offsets.size(); }

        iterator begin() const noexcept {return iterator(*this, 0);}
        iterator end() const noexcept {return iterator(*this, _offsets.size());}
        // Gets an element; safe: returns empty string when out-of-bounds.
        reference operator[](std::size_t) const noexcept;
        private:
        Path(): _offsets(), _tags() {}
        std::vector<std::size_t> _offsets;
        std::string _tags;
        void pushItem(const std::string& s) noexcept;
        void popItem()  noexcept;
        void clear()  noexcept { _offsets.clear(); _tags.clear(); }
        friend typename XmlParser;
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
    /** Type and text of current item */

    const ItemType& getItemType() const noexcept { return _itemType; }

    /// Checks if the item is either start-tag or self-closing tag.
    bool isElement() const noexcept 
    {return ((int)_itemType & ((int)ItemType::kPrefix | 
        (int)ItemType::kSelfClosing));}

    /// Checks if the item is element of specific name.
    bool isElement(const char* name) const noexcept 
    { return isElement() && getName() == name;}

    /// Checks if the item is self-closing tag.
    bool isSelfClosing() const noexcept 
    {return (_itemType == ItemType::kSelfClosing);}
  
    /// Same as ! isSelfClosing().
    bool isPrefix() const noexcept 
    {return (_itemType == ItemType::kPrefix);}

    /// Checks if the item is end-tag.
    bool isSuffix() const noexcept 
    {return (_itemType == ItemType::kSuffix);}

    /// Checks if the item is either an end-tag or a self-closing tag.
    bool isElementEnd() const noexcept 
    {return ((int)_itemType & ((int)ItemType::kSuffix |
        (int)ItemType::kSelfClosing));}

    /// Checks if the item is text block, including CDATA blocks.
    bool isText() const noexcept 
    {return ((int)_itemType & ((int)ItemType::kEscapedText | 
        (int)ItemType::kCData));}

    /// Checks if the item is text block of specific name.
    bool isText(const char* name) const noexcept 
    { return isText() && getName() == name;}

    /// Checks if the item is CDATA block.
    bool isCDATA() const noexcept 
    {return (_itemType == ItemType::kCData);}
    
    /// Same as ! isCDATA().
    bool isEscapedText() const noexcept 
    {return (_itemType == ItemType::kEscapedText);}
    
    /// True if the item  is a Processor Instruction.
    bool isPI() const noexcept 
    {return (_itemType == ItemType::kPI);}

    /// True if the item  is a comment.
    bool isComment() const noexcept 
    {return (_itemType == ItemType::kComment);}

    /// True if the item is a Document Type Declaration.
    bool isDTD() const noexcept 
    {return (_itemType == ItemType::kDTD);}

    /// True if eof or error, same effect as ! good().
    /// further processing is not available.
    bool isEnd() const noexcept 
    {return (_itemType == ItemType::kEnd); }
    
    /// Text of the current item.
    /// \detail For a text block, gets the text; for any kind of tag
    /// gets the tag's text including angle brackets.
    /// For ItemType::kEnd, the text is either empty or contains 
    /// an incomplete tag which produced error.
    const std::string& getText() const noexcept { return _text; }

    ///}@

    ///@{
    /** Properties of current element */

    /// Current element's start tag
    std::string_view getStartTag() const noexcept;

    /// Current element's name; valid while current item
    /// is this element's start-tag, text, comment, PI or end-tag
    std::string_view getName() const noexcept;

    ///  True if current element has attributes; valid while current
    ///  item is this element's start-tag, text, comment, PI or end-tag
    bool hasAttributes() const noexcept;

    ///  Current element's attributes; valid while current item
    ///   is this element's start-tag, text, comment, PI or end-tag
    std::vector<Attribute> getAttributes() const noexcept;

    ///}@

    ///@{
    /** Current path, level, and parent elements's properties. */

    /// \brief Path depth; 0: document level; 1: root element etc.
    /// \detail Entering an element increments the level:
    ///
    ///     <?xml....?>         // level = 0
    ///     <!DOCTYPE...>       // level = 0
    ///     <root>              // level = 1 - new element
    ///       bla bla           // level = 1
    ///       <!-- bla bla -->  // level = 1
    ///       <?php...?>        // level = 1
    ///       <nested>          // level = 2 - new element
    ///         bla bla         // level = 2
    ///         <empty/>        // level = 3 - new, even self-closing
    ///         bla bla         // level = 2 - back
    ///         <empty/>        // level = 3 - and forth...
    ///         <group>         // level = 3 - same nesting level
    ///           bla bla       // level = 3
    ///         </group>        // level = 3
    ///       </nested>         // level = 2 - back
    ///     </root>             // level = 1 - back
    ///                         // level = 0 - EOF

    /// Returns  current path as the stack of start-tags 
    const Path& getPath() const noexcept { return _path; }

    /// Returns current path depth; same as getPath().size()
    std::size_t getLevel() const noexcept { return getPath().size();}

    ///}@

    ///@{
    /** Traversing entities */

    /// Loads next item.
    /// \return False if EOF is reached or data error occured -
    /// in either case furher processing is not possible.
    /// \detail 
    /// The iterated items are: all element's tags (start, end, and 
    /// self-closing ones), text blocks, CDATA text blocks, processing 
    /// instuctions (PI), comments, type declarations (DTD). Text blocks may 
    /// come more than once if interleaved with nested entities or enclosed 
    /// by CDATA-s. 
    
    bool next() noexcept;

    /// Loads next item in bounds of a specified level.
    /// \param level Level. 
    /// \return False if the end-tag of an element of this level or EOF is 
    /// reached, or data error ocurred. 
    bool next(std::size_t lvl) noexcept 
    { return !(isElementEnd() && getLevel() == lvl) ? next() : false;}

    ///}@

    ///@{ Write

    /// Interface intended to copy data e.g. to write it to a file
    class IWriter
    {
        public:
        /// Callback for data writing functions 
        /// \param data Poiter to the data to write
        /// \param n Size of data, bytes
        /// \param userIndex Can be used e.g.to choose a particular file or data container. 
        virtual void write(const char* data, std::size_t n, std::size_t userIndex) = 0;
        void write(const std::string_view s, std::size_t userIndex) { write(s.data(), s.size(), userIndex); }
    };

    /// A simple IWriter implementation to write to files
    class FileWriter: public IWriter
    {
        public:
        bool openFiles(std::string dir, std::initializer_list<const char*>filenames) noexcept;
        void closeFiles() noexcept;
        ~FileWriter();
        virtual void write(const char* data, std::size_t n, std::size_t iFile) noexcept;
        using IWriter::write;
        private:
            std::vector<FILE*> outputs; 
    };

    /// Passes current item's text to IWriter and performs next().
    bool writeItem(IWriter& writer, std::size_t userIndex);
  

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
  
    Path _path;  // Stack of start-tags 
    ItemType _itemType;
    std::string _text;   

    bool loadNextChunk() noexcept;
    bool appendRestOfPI() noexcept;
    bool appendRestOfComment() noexcept;
    bool appendRestOfCDATA() noexcept;
    ItemType loadTag() noexcept;
    ItemType loadText() noexcept;
    static std::string_view getNameFromTag(std::string_view) noexcept;
    static bool hasTagAttributes(std::string_view s) noexcept;
    static std::vector<Attribute> getAttributesFromTag(std::string_view) 
        noexcept;
};
