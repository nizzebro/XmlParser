# XmlParser
A simple C++ class for fast parsing XML files, aimed at a maximum speed and minimum overhead. <br>
Works as a stream with buffer which reads fixed-sized chunks. Entities are processed one-by-one.  <br>
Currently supports utf-8 only.

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
            while(next(i)) // until </fruits>; as for <fruits/>, will return false immediately
            {
                if(isText("apples")) // a text block of an <apples> element ? 
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
