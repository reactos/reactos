/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
//
// MSXML C++ class corresponding to Java's String
//

#ifndef _CORE_LANG_STRING
#define _CORE_LANG_STRING

DEFINE_CLASS(String);
DEFINE_CLASS(StringBuffer);

class Locale;
class SlotAllocator;

///////////////////////////////////////////////////////////////////////////////////////////
//          String class (close to Java's String)

#define FINAL 
#define VIRTUAL virtual

class DLLEXPORT String : public Base
{
    friend class WSStringBuffer;
    friend class AsciiText;

    public: static void classInit();

    DECLARE_CLASS_MEMBERS(String, Base);
    DECLARE_CLASS_INSTANCE(String, Base);

    private: static SRString s_nullString;
    private: static SRString s_emptyString;
    private: static SRString s_newLineString;

    public:  static String * nullString();
    public:  static String * emptyString();
    public:  static String * newLineString();

    /////////////  Constructors  ////////////
    
    //  String() 
    // Allocates a newString containing no characters. 
    protected: String() : _chars(REF_NOINIT) 
               { init((ATCHAR *)null, 0, 0); }

    //  String(char[]) 
    // Allocates a newString so that it represents the sequence of characters 
    // currently contained in the character array argument. 
    public: static String * newString(const ATCHAR * value); 

    //  String(char[], int, int) 
    // Allocates a newString that contains characters from a subarray of the 
    // character array argument. 
    protected: String(const ATCHAR * value, int offset, int count) : _chars(REF_NOINIT) 
               { init(value, offset, count); }
    public: static String * newString(const ATCHAR * value, int offset, int count);

    //  String(String) 
    // Allocates a new string that contains the same sequence of characters as the 
    // string argument. 
    public: static String * newString(const String * value);

    //  String(StringBuffer) 
    // Allocates a new string that contains the sequence of characters currently 
    // contained in the string buffer argument. 
    public: static String * newString(StringBuffer * buffer); 
    
    protected: String(const TCHAR * value, int offset, int count) : _chars(REF_NOINIT) 
               { init(value, offset, count); }
    public: static String * newString(const TCHAR * c);
    public: static String * newString(const TCHAR * c, int offset, int length);
    public: static String * newString(int c);
    public: static String * newString(const char * s);
    public: static String * newString(TCHAR c);

    // create a (deep) copy of this string
    public: String * copyString();

    //////////////  Methods  ////////////

    //  charAt(int) 
    // Returns the character at the specified index. 
    public: FINAL TCHAR charAt(int index);

    //  compareTo(String) 
    // Compares two strings lexicographically. 
    public: FINAL int compareTo(String * anotherString);

    //  concat(String) 
    // Concatenates the specified string to the end of this string. 
    public: FINAL String * concat(String * str);

    //  copyValueOf(char[]) 
    // Returns a String that is equivalent to the specified character array. 
    public: static String * copyValueOf(ATCHAR * data)
        { return copyValueOf(data, 0, data->length()); }

    //  copyValueOf(char[], int, int) 
    // Returns a String that is equivalent to the specified character array. 
    public: static String * copyValueOf(ATCHAR * data, int offset, int count)
        { return String::newString(data, offset, count); }

    //  endsWith(String) 
    // Tests if this string ends with the specified suffix. 
    public: FINAL bool endsWith(String * suffix)
        { return startsWith(suffix, length() - suffix->length()); }

    //  equals(Object) [Object]
    // Compares this string to the specified object. 
    public: FINAL bool equals(Object * anObject);

    //  compare(String)
    // Compares this string to the specified string.
    public: FINAL int compare(String * strOther);

    //  compare(String)
    // Compares this string to the specified string.
    public: FINAL int compare(const TCHAR * strOther, int length = -1);

    //  equalsIgnoreCase(String) 
    // Compares this String to another String. 
    public: FINAL bool equalsIgnoreCase(String * anotherString);

    //  getBytes() 
    // Convert this String into bytes according to the platform's default character 
    // encoding, storing the result into a new byte array. 

    //  getBytes(int, int, byte[], int) 
    // Copies characters from this string into the destination byte array. Deprecated. 

    //  getBytes(String) 
    // Convert this String into bytes according to the specified character encoding, 
    // storing the result into a new byte array. 

    //  getChars(int, int, char[], int) 
    // Copies characters from this string into the destination character array. 
    public: FINAL void getChars(int srcBegin, int srcEnd, ATCHAR * dst, int dstBegin) const;

    //  hashCode() [Object]
    // Returns a hashcode for this string. 
    public: VIRTUAL int hashCode();

    //  indexOf(int) 
    // Returns the index within this string of the first occurrence of the specified 
    // character. 
    public: FINAL int indexOf(int ch) { return indexOf(ch, 0); }

    //  indexOf(int, int) 
    // Returns the index within this string of the first occurrence of the specified 
    // character, starting the search at the specified index. 
    public: FINAL int indexOf(int ch, int fromIndex);

    //  intern() 
    // Returns a canonical representation for the string object. 
    public: FINAL String * intern();

    //  lastIndexOf(int) 
    // Returns the index within this string of the last occurrence of the specified 
    // character. 
    public: FINAL int lastIndexOf(int ch)
        { return lastIndexOf(ch, length() - 1); }

    //  lastIndexOf(int, int) 
    // Returns the index within this string of the last occurrence of the specified 
    // character, searching backward starting at the specified index. 
    public: FINAL int lastIndexOf(int ch, int fromIndex);

    //  length() 
    // Returns the length of this string. 
    public: FINAL int length() const { return _length; }

    //  regionMatches(boolean, int, String, int, int) 
    // Tests if two string regions are equal. 
    public: FINAL bool regionMatches(bool ignoreCase, int toffset,
                           String * other, int ooffset, int len);

    //  regionMatches(int, String, int, int) 
    // Tests if two string regions are equal. 
    public: FINAL bool regionMatches(int toffset, String * other, int ooffset, int len);

    //  replace(char, char) 
    // Returns a new string resulting from replacing all occurrences of oldChar in 
    // this string with newChar. 
    public: FINAL String * replace(TCHAR oldChar, TCHAR newChar);

    //  startsWith(String) 
    // Tests if this string starts with the specified prefix. 
    public: FINAL bool startsWith(String * prefix)
        { return startsWith(prefix, 0); }

    //  startsWith(String, int) 
    // Tests if this string starts with the specified prefix. 
    public: FINAL bool startsWith(String * prefix, int toffset);

    //  substring(int) 
    // Returns a new string that is a substring of this string. 
    public: FINAL String * substring(int beginIndex)
        { return substring(beginIndex, length()); }

    //  substring(int, int) 
    // Returns a new string that is a substring of this string. 
    public: FINAL String * substring(int beginIndex, int endIndex);

    //  toCharArray() 
    // Converts this string to a new character array. 
    public: FINAL ATCHAR * toCharArray() const;

    //  toCharArray() 
    // Converts this string to a new character array. 
    public: FINAL const ATCHAR * getCharArray() const;

    //  toLowerCase() 
    // Converts this String to lowercase. 
    public: FINAL String * toLowerCase();

    //  toLowerCase(Locale) 
    // Converts all of the characters in this String to lower case using the rules 
    // of the given locale. 
    public: FINAL String * toLowerCase( Locale * locale );

    //  toString() [Object]
    // This object (which is already a string!) is itself returned. 
    public: VIRTUAL String * toString() { return this; }

    //  toUpperCase() 
    // Converts this string to uppercase. 
    public: FINAL String * toUpperCase();

    //  toUpperCase(Locale) 
    // Converts all of the characters in this String to upper case using the rules 
    // of the given locale. 
    public: FINAL String * toUpperCase( Locale * locale );

    //  trim() 
    // Removes white space from both ends of this string. 
    public: FINAL String * trim();

    //  isWhitespace() 
    // True if the string is comprised only of whitespace characters (or has length==0). 
    public: FINAL bool isWhitespace();

    //  valueOf(boolean) 
    // Returns the string representation of the boolean argument. 
    public: static String * valueOf(bool b)
        { return b ? String::newString(_T("true")) : String::newString(_T("false")); }

    //  valueOf(char) 
    // Returns the string representation of the char * argument. 
    public: static String * valueOf(TCHAR c);

    //  valueOf(char[]) 
    // Returns the string representation of the char array argument. 
    public: static String * valueOf(ATCHAR * data)
        { return String::newString(data); }

    //  valueOf(char[], int, int) 
    // Returns the string representation of a specific subarray of the char array 
    // argument. 
    public: static String * valueOf(ATCHAR * data, int offset, int count)
        { return String::newString(data, offset, count); }

    //  valueOf(double) 
    // Returns the string representation of the double argument. 
    public: static String * valueOf(double d);

    //  valueOf(float) 
    // Returns the string representation of the float argument. 
    public: static String * valueOf(float f);

    //  valueOf(int) 
    // Returns the string representation of the int argument. 
    public: static String * valueOf(int i);

    //  valueOf(long) 
    // Returns the string representation of the long argument. 
    public: static String * valueOf(int64 l);

    //  valueOf(Object) 
    // Returns the string representation of the Object argument. 
    public: static String * valueOf(Object * obj)
        { return (obj == null) ? nullString() : obj->toString(); }


    /////////////////////////////////////////////////////////////////////////////
    // The remaining methods aren't part of the Java interface.
    
    public: bool equalsIgnoreCase(const TCHAR *);

    // Concatenate the passed int strings and returns a new string object.
    public: static String * __cdecl add(String *, ...);

    // Return zero terminated character string.
    public: ATCHAR * toCharArrayZ() const;

    public: BSTR getBSTR() const;
    public: static int hashCode(const TCHAR * c, int length);
    public: bool equals(const TCHAR *);
    public: bool equals(const TCHAR *, int length);

    public: AString* split(TCHAR splitchar);

    public: static String* join(AString* array, const TCHAR joinchar, int start = -1, int end = -1);

    public: const WCHAR * getWCHARPtr() const;
    public: WCHAR * getWCHARBuffer() const;

    /////////////////////////////////////////////////////////////////////////////////
    // Representation

    public: void copyData(TCHAR * pchBuf, int iBuf);
    const TCHAR * getData() const { return _chars->getData(); }

private:
    // My string is the subarray from _chars[_offset] to _chars[_offset + length - 1].
    // This allows several Strings to share the same _chars array, and makes copying and
    // initialization fast.
    int         _offset;
    int         _length;
    RATCHAR     _chars;

    // Helper functions
    TCHAR * getDataNC() { return const_cast<TCHAR *>(getData()); }

    void init(const ATCHAR * a, int offset, int length);
    void init(const TCHAR * c, int offset, int length);
};

/**
 * This is a special helper class designed to work on the stack.
 * It should not be allocated on the heap because the constructor
 * throws exceptions and this is NOT garbage collected.
 *
 * The usage is to pass an object to the constructor. The object 
 * will be turned into string by calling toString() on it and the
 * result will be copied into this buffer object as ASCII text. If
 * toString() or buffer allocation fails the object will not contain
 * any data and so will be silently go from the stack.
 */
class AsciiText
{
private: inline void * __cdecl operator new(size_t cb)
        { Assert(FALSE && "Shouldn't be called"); return null; }

private: inline void __cdecl operator delete(void *pv)
        { Assert(FALSE && "Shouldn't be called"); }

public: AsciiText(Base * pBase);
public: ~AsciiText();

public: operator char * () { return _pac; }

private:    char * _pac;
private:    char _acbuff[80];
private:    const static char * _acnull;
public:     unsigned int _uLen;

};

#undef FINAL
#undef VIRTUAL

#endif _CORE_LANG_STRING
