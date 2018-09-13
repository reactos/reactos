/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_LANG_INTEGER
#define _CORE_LANG_INTEGER


DEFINE_CLASS(Integer);

/**
 */
class Integer : public Base
{
    DECLARE_CLASS_MEMBERS(Integer, Base);
    DECLARE_CLASS_CLONING(Integer, Base);

    // cloning constructor, shouldn't do anything with data members...
    protected:  Integer(CloningEnum e) : super(e) {}

    public: enum
            {
            MAX_VALUE = 2147483647,
            MIN_VALUE = 0x80000000,
            };

    protected: Integer()
    {
    }

    protected: Integer(int i)
    {
        value = i;
    }

    public: static Integer * newInteger(int i);

    public: virtual int hashCode()
    {
        return value;
    }

    public: int intValue()
    {
        return value;
    }

    public: virtual Object * clone()
    {
        Integer * i = (Integer *)super::clone();
        i->value = value;
        return i;
    }

    public: static int parseInt(String * s);

    public: virtual String * toString();

    private: int value;
};


#endif _CORE_LANG_INTEGER
