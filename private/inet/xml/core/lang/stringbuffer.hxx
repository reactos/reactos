/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_LANG_STRINGBUFFER
#define _CORE_LANG_STRINGBUFFER

DEFINE_CLASS(StringBuffer);
class String;

#define FINAL
#define VIRTUAL virtual

class DLLEXPORT StringBuffer: public Base
{
    friend class String;
    
    DECLARE_CLASS_MEMBERS(StringBuffer, Base);
    
    //////////////   Java constructors   //////////////////////////////////////////

    //  StringBuffer() 
    // Constructs a string buffer with no characters in it and an initial capacity 
    // of 16 characters. 

    //  StringBuffer(int) 
    // Constructs a string buffer with no characters in it and an initial capacity 
    // specified by the length argument. 
    protected: StringBuffer() : _chars(REF_NOINIT) { init(); }
    protected: StringBuffer(int capacity) : _chars(REF_NOINIT) { init(capacity); }
    protected: void init(int capacity = 16);
    public: static StringBuffer * newStringBuffer(int capacity);

    //  StringBuffer(String) 
    // Constructs a string buffer so that it represents the same sequence of 
    // characters as the string argument. 

    ///////////////    Java methods   /////////////////////////////////////////////

    //  append(boolean) 
    // Appends the string representation of the boolean argument to the string buffer. 

    //  append(char) 
    // Appends the string representation of the char argument to this string buffer. 
    public: FINAL StringBuffer * append(TCHAR ch);

    //  append(char[]) 
    // Appends the string representation of the char array argument to this string 
    // buffer. 
    public: FINAL StringBuffer * append(ATCHAR * str);

    //  append(char[], int, int) 
    // Appends the string representation of a subarray of the char array argument to 
    // this string buffer. 
    public: FINAL StringBuffer * append(ATCHAR * str, int offset, int len);

    //  append(double) 
    // Appends the string representation of the double argument to this string buffer.

    //  append(float) 
    // Appends the string representation of the float argument to this string buffer. 

    //  append(int) 
    // Appends the string representation of the int argument to this string buffer. 

    //  append(long) 
    // Appends the string representation of the long argument to this string buffer. 

    //  append(Object) 
    // Appends the string representation of the Object argument to this string buffer.

    //  append(String) 
    // Appends the string to this string buffer. 
    public: FINAL StringBuffer * append(String * str);

    //  capacity() 
    // Returns the current capacity of the String buffer. 
    public: FINAL int capacity() const { return _chars->length(); }

    //  charAt(int) 
    // Returns the character at a specific index in this string buffer. 

    //  ensureCapacity(int) 
    // Ensures that the capacity of the buffer is at least equal to the specified 
    // minimum. 
    public: FINAL void ensureCapacity(int minCapacity);

    //  getChars(int, int, char[], int) 
    // Characters are copied from this string buffer into the destination character 
    // array dst. 
    public: FINAL void getChars(int srcBegin, int srcEnd, ATCHAR *dst, int dstBegin);

    //  insert(int, boolean) 
    // Inserts the string representation of the boolean argument into this string 
    // buffer. 

    //  insert(int, char) 
    // Inserts the string representation of the char argument into this string buffer.

    //  insert(int, char[]) 
    // Inserts the string representation of the char array argument into this string 
    // buffer. 

    //  insert(int, double) 
    // Inserts the string representation of the double argument into this string 
    // buffer. 

    //  insert(int, float) 
    // Inserts the string representation of the float argument into this string 
    // buffer. 

    //  insert(int, int) 
    // Inserts the string representation of the second int argument into this string 
    // buffer. 

    //  insert(int, long) 
    // Inserts the string representation of the long argument into this string buffer.

    //  insert(int, Object) 
    // Inserts the string representation of the Object argument into this string 
    // buffer. 

    //  insert(int, String) 
    // Inserts the string into this string buffer. 

    // //  length() 
    // Returns the length (character count) of this string buffer. 
    public: FINAL int length() const { return _length; }

    //  reverse() 
    // The character sequence contained in this string buffer is replaced by the 
    // reverse of the sequence. 

    //  setCharAt(int, char) 
    // The character at the specified index of this string buffer is set to ch. 

    //  setLength(int) 
    // Sets the length of this String buffer. 

    //  toString() [Object]
    // Converts to a string representing the data in this string buffer. 
    public: VIRTUAL String * toString();

    //////////////////   Non-Java methods   ///////////////////////////////////
    
    public: FINAL StringBuffer * append(abyte * str, int offset, int len);
    public: FINAL StringBuffer * append(TCHAR * str);
    public: FINAL StringBuffer * append(TCHAR * str, int offset, int len);

    private: ATCHAR * getArray() { return _chars; }

    private: void aboutToChange(int newCapacity) { if (_copyBeforeChange) copy(newCapacity); }
    private: void copy(int newCapacity);
    
    //////////////////   representation   /////////////////////////////////////
private:
    int     _length;
    RATCHAR _chars;
    bool    _copyBeforeChange;  // true when buffer is shared by a String
};


#undef FINAL
#undef VIRTUAL


#endif _CORE_LANG_STRINGBUFFER

