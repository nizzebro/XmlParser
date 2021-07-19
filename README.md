# XmlParser
A simple C++ class for fast parsing XML files with a minimum overhead.
A stream with buffer, fixed-size chunks. Currently supports utf-8 only.

    XmlParser parser;
    parser.openFile("D:\sample.xml");
    while(next())
    {
        if(isElement("fruits"))
        {
            for(auto & a : getAttributes())
            { 
                std::cout << a.name << '=' << a.value << '\n';
            }
            auto i = getLevel();
            while(next(i)) // until </fruits> (self-closing <fruits/> returns false immediately)
            {
                if(isText("apples")) // text of <apple>? 
                {
                    std::cout << "The text of <apples> : " << '\n' << getText() << '\n';
                    for(auto & elem : getPath())
                    {
                        std::cout << elem.getName(i) << '\\'; // path
                    }
                    std::cout << '\n';
                }
            }
            std::cout << "the end-tag </fruits> reached" << '\n';
        }
        else if(isPI() || isDTD() || isComment())
        {
            std::cout << getText(); // the tag's text
        }
     }
    parser.closeFile(); 
