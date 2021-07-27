# XmlParser
A simple C++ class for parsing large XML, aimed at a maximum speed and minimum overhead. <br>
Works as a stream with its own buffer reading fixed-sized chunks. Entities are processed one-by-one.  <br>
Currently supports utf-8 only.

    XmlParser p;
    p.openFile("D:\\sample.xml");
    while(p.next())
    {
        if(p.isElement("fruits"))
        {
            for(auto & a : p.getAttributes())
            { 
                std::cout << a.name << '=' << a.value << '\n';
            }
            auto i = p.getLevel();
            while(p.next(i)) // loop until </fruits> (or <fruits/> - which returns immidiately)
            {
                if(p.isText("apples")) // a text block of an <apples> element ? 
                {
                    std::cout << "The text of <apples> : " << '\n' << p.text << '\n'; 
                    for(auto & elem : p.path)
                    {
                        std::cout << elem.getName() << '\\'; // path
                    }
                    std::cout << '\n';
                }
            }
            std::cout << "the end-tag </fruits> is reached" << '\n';
        }
        else if(p.isPI() || p.isDTD() || p.isComment())
        {
            std::cout << p.text; // the tag's text
        }
    }
    p.closeFile(); 
