/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_WSSTRINGBUFFER
#define _CORE_UTIL_WSSTRINGBUFFER

DEFINE_CLASS(WSStringBuffer);
class String;

class WSStringBuffer: public Base
{
    DECLARE_CLASS_MEMBERS(WSStringBuffer, Base);
    
    //////////////   constructors   //////////////////////////////////////////

    //  WSStringBuffer(int) 
    // Constructs a string buffer with no characters in it and an initial capacity 
    // specified by the length argument. 
    public: static WSStringBuffer * newWSStringBuffer(int capacity = 16);

    protected: WSStringBuffer(int capacity = 16) : _chars(REF_NOINIT) { init(capacity); }


    ///////////////    methods   /////////////////////////////////////////////

public:

    // WS_PRESERVE: Dump text into buffer verbatim.  In terms of WS interaction, it is as if the entire text was non-WS.
    // WS_COLLAPSE: Collapse adjactent WS nodes into a single space.  Trim WS from leading and tailing edge.
    // WS_TRIM: Trim, WS if it is at the leading edge of ENTIRE buffer, or tailing edge of ENTIRE buffer
    enum WSMODE {WS_PRESERVE, WS_COLLAPSE, WS_TRIM};

    WSStringBuffer * append(TCHAR ch, WSMODE eMode);
    WSStringBuffer * append(const TCHAR * ach, int length, WSMODE eMode);

    // convenience helpers
    WSStringBuffer * append(const String * str, WSMODE eMode);
    WSStringBuffer * append(const ATCHAR * chars, WSMODE eMode);

    // Returns the current capacity of the String buffer. 
    int capacity() const { return _chars->length(); }

    // Ensures that the capacity of the buffer is at least equal to the specified 
    // minimum. 
    void ensureCapacity(int minCapacity);

    // Returns the length (character count) of this string buffer. 
    int length() const { return _length; }

    // Converts to a string representing the data in this string buffer. 
    // Note: This empties the buffer!!!
    String * toString();

    //  Convert newlines into spaces (fIE4 is true) or 0xA.
    //  if fIE4 is true, also convert \t into space 
    WSStringBuffer * normalize(bool fIE4);


    //////////////////   internal methods   ///////////////////////////////////

    protected: void init(int capacity);

    private: void _append(TCHAR ch)
        {
            Assert(null != (ATCHAR *)_chars);
            (*_chars)[_length] = ch;
            ++_length;
        }

    private: bool _collapsingAppend(TCHAR ch);

    private: void copy(int newCapacity);

   //////////////////   representation   /////////////////////////////////////
private:
    int     _length;
    int     _trimPoint;
    RATCHAR _chars;

    bool    _fCachedWS;
    bool    _fLeading;
};


#endif _CORE_UTIL_STRINGBUFFER

