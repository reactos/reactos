/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _NODEPROPERITY_HXX
#define _NODEPROPERITY_HXX

typedef struct NDProperties
{
    #define HASNAME         0x20
    #define HASNAMESPACE    0x40    
    #define ISCONTAINER     0x80
    #define ISDATA          0x100


    union
    {
        struct
        {
            unsigned    iNodeType   : 5;          // the corresponding Node Type
            unsigned    fName       : 1;          // true if the node has a name
            unsigned    fNameSpace  : 1;          // ture if the node name can have a namespace
            unsigned    fContainer  : 1;          // true if it is ELEMENT, XMLDECL, or DOCTYPE
            unsigned    fData       : 1;          // true if XML_PCDATA, XML_CDATA, or XML_WHITESPACE
            unsigned    uBites      : 7;          // unused bites
        };
        unsigned short properties;
    };
    
    // The following changes are for performance
    // they may cause problems on other platforms 
    // where there is an endian problem!
    inline unsigned short HasName()
    {
#ifdef UNIX
        return fName;
#else
        return (properties & HASNAME);  // fName
#endif
    };

    inline unsigned short HasNameSpace()
    {
#ifdef UNIX
        return fNameSpace;
#else
        return (properties & HASNAMESPACE);  // fNameSpace
#endif
    };

    inline unsigned short IsContainer()
    {
#ifdef UNIX
        return fContainer;
#else
        return (properties & ISCONTAINER);  // fContainer
#endif
    };

    inline unsigned short IsData()
    {
#ifdef UNIX
        return fData;
#else
        return (properties & ISDATA);  // fData
#endif
    };


} NodeProperties;

extern NodeProperties nodeProperties[];


#endif _NODEPROPERITY_HXX