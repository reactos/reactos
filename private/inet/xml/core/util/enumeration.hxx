/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_UTIL_ENUMERATION
#define _CORE_UTIL_ENUMERATION


DEFINE_CLASS(Enumeration);

/**
 */

class NOVTABLE Enumeration : public Object
{
public:

    /**
     */
    virtual bool hasMoreElements() = 0;

    /**
     */
    virtual Object * peekElement() = 0;

    /**
     */
    virtual Object * nextElement() = 0;

    /**
     */
    virtual void reset() = 0;
};


#endif _CORE_UTIL_ENUMERATION
