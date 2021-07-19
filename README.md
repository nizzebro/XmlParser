# XmlParser
A simple C++ class for fast stream parsing XML files with a minimum overhead.

    XmlParser parser;
    parser.openFile("D:\sample.xml");
    while(next())
    {
        if(isElement("fruits"))
        {
            for(auto & a : attributes())
            { 
                std::cout << a.name << '=' << a.value << '\n';
            }
            auto i = getLevel();
            while(next(i)) // until </fruits> (self-closing <fruits/> returns false immediately)
            {
                if(isText("apple")) // text of <apple>?
                {
                    std::cout << "The text of <apple> : " << '\n' << getText() << '\n';
                    for(int i = 0; i <= getLevel(); ++i)
                    {
                        std::cout << getName(i) << '\\'; // path
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
