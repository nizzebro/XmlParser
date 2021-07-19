# XmlParser
Simple XML parsing C++ class; stream-like; minimum overhead.

    XmlParser parser;
    parser.openFile("D:\sample.xml");
    while(next())
    {
        if(isElement() && getName() == "category")
        {
            for(auto & a : getAttributes())
            { 
                std::cout << a.name << '=' << a.value << '\n';
            }
            auto i = level();
            while(next(i)) // iterates until the end-tag of current nesting level
            {
                if(isText() && getName() == "item") // getName() is the element's name
                {
                    std::cout << "The text of <item> is: " << '\n' << getText() << '\n';
                }
            }
            std::cout << "</category> is reached" << '\n';
        }
     }
    parser.closeFile(); 
