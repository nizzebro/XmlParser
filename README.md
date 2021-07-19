# XmlParser
Simple C++ class for fast stream parsing XML files with a minimum overhead.

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
            auto i = getLevel();
            while(next(i)) // iterates until the end-tag of current nesting level
            {
                if(isText() && getName() == "item") // getName() is the element's name
                {
                    std::cout << "The text of <item> : " << '\n' << getText() << '\n';
                }
            }
            std::cout << "</category> end-tag is reached" << '\n';
        }
     }
    parser.closeFile(); 
