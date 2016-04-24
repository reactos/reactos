/*
 * Usage: utf16le inputfile outputfile
 *
 * This is a tool and is compiled using the host compiler,
 * i.e. on Linux gcc and not mingw-gcc (cross-compiler).
 * It's a converter from utf-8, utf-16 (LE/BE) and utf-32 (LE/BE)
 * to utf-16 LE and especially made for automatic conversions of
 * INF-files from utf-8 to utf-16LE (so we can furthermore
 * store the INF files in utf-8 for subversion.
 *
 * Author: Matthias Kupfer (mkupfer@reactos.org)
 */

#include <fstream>
#include <iostream>

//#define DISPLAY_DETECTED_UNICODE

using namespace std;

class utf_converter
{
public:
    // detect can detect utf-8 and both utf-16 variants, but assume utf-32 only
    // due to ambiguous BOM
    enum enc_types { detect, utf8, utf16le, utf16be, utf32le, utf32be };
    enum err_types { none, iopen, oopen, eof, read, write, decode };
protected:
    err_types error;
    enc_types encoding;
    unsigned char buffer[4], fill, index; // need 4 char buffer for optional BOM handling
    fstream inputfile,outputfile;
    static const unsigned char utf8table[64];
public:
    utf_converter(string ifname, string ofname, enc_types enc = detect) : error(none), encoding(enc), fill(0), index(0)
    {
        enc_types tmp_enc;
        inputfile.open(ifname.c_str(), ios::in | ios::binary);
        if (!inputfile)
        {
            error = iopen;
            return;
        }
        outputfile.open(ofname.c_str(), ios::out | ios::binary);
        if (!outputfile)
        {
            error = oopen;
            return;
        }
        tmp_enc = getBOM();
        if (enc != detect)
        {
            if (enc != tmp_enc)
                cerr << "Warning: UTF-BOM doesn't match encoding setting, but given encoding forced" << endl;
        }
        else
            encoding = tmp_enc;
    }
    err_types getError()
    {
        return error;
    }
    enc_types getBOM()
    {
        index = 0;
        /* first byte can also detect with:
        if ((buffer[0] & 0x11) || !buffer[0]))
        valid values are 0xef, 0xff, 0xfe, 0x00
        */
        inputfile.read(reinterpret_cast<char*>(&buffer),4);
        fill =inputfile.gcount();
        // stupid utf8 bom
        if ((fill > 2) &&
            (buffer[0] == 0xef) &&
            (buffer[1] == 0xbb) &&
            (buffer[2] == 0xbf))
        {
            index += 3;
            fill -=3;
#ifdef DISPLAY_DETECTED_UNICODE
            cerr << "UTF-8 BOM found" << endl;
#endif
            return utf8;
        }
        if ((fill > 1) &&
            (buffer[0] == 0xfe) &&
            (buffer[1] == 0xff))
        {
            index += 2;
            fill -= 2;
#ifdef DISPLAY_DETECTED_UNICODE
            cerr << "UTF-16BE BOM found" << endl;
#endif
            return utf16be;
        }
        if ((fill > 1) &&
            (buffer[0] == 0xff) &&
            (buffer[1] == 0xfe))
        {
            if ((fill == 4) &&
                (buffer[2] == 0x00) &&
                (buffer[3] == 0x00))
            {
                cerr << "UTF Error: ambiguous BOM UTF-16 or UTF-32; assume UTF-32" << endl;
                fill = 0;
                index = 0;
                return utf32le;
            }
            fill -= 2;
            index += 2;
#ifdef DISPLAY_DETECTED_UNICODE
            cerr << "UTF-16LE BOM found" << endl;
#endif
            return utf16le;
        }
        if ((fill == 4) &&
            (buffer[0] == 0x00) &&
            (buffer[1] == 0x00) &&
            (buffer[2] == 0xfe) &&
            (buffer[3] == 0xff))
        {
            fill = 0;
            index = 0;
#ifdef DISPLAY_DETECTED_UNICODE
            cerr << "UTF-32BE BOM found" << endl;
#endif
            return utf32be;
        }
        return utf8; // no valid bom so use utf8 as default
    }
    int getByte(unsigned char &c)
    {
        if (fill)
        {
            index %= 4;
            --fill;
            c = buffer[index++];
            return 1;
        } else
        {
            inputfile.read(reinterpret_cast<char*>(&c),1);
            return inputfile.gcount();
        }
    }
    int getWord(unsigned short &w)
    {
        unsigned char c[2];
        if (!getByte(c[0]))
                return 0;
        if (!getByte(c[1]))
                return 1;
        if (encoding == utf16le)
            w = c[0] | (c[1] << 8);
        else
            w = c[1] | (c[0] << 8);
        return 2;
    }
    int getDWord(wchar_t &d)
    {
        unsigned char c[4];
        for (int i=0;i<4;i++)
            if (!getByte(c[i]))
                    return i;
        if (encoding == utf32le)
            d = c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
        else
            d = c[3] | (c[2] << 8) | (c[1] << 16) | (c[0] << 24);
        return 4;
    }
    wchar_t get_wchar_t()
    {
        wchar_t ret = (wchar_t)-1;
        switch (encoding)
        {
            case detect: // if still unknwon
                encoding = utf8; // assume utf8 as default
            case utf8:
                unsigned char c, tmp;
                if (!getByte(tmp))
                    return ret;
                // table for 64 bytes (all 11xxxxxx resp. >=192)
                // resulting byte is determined:
                // lower 3 bits: number of following bytes (max.8) 0=error
                // upper 5 bits: data filled with 0
                if (tmp & 0x80)
                {
                    if ((tmp & 0xc0) != 0xc0)
                    {
                        cerr << "UTF-8 Error: invalid data byte" << endl;
                        return ret;
                    }
                    unsigned char i = utf8table[tmp & 0x3f];
                    ret = i >> 3;
                    i &= 7;
                    while (i--)
                    {
                        ret <<= 6;
                        if (!getByte(c))
                            return wchar_t(-1);
                        ret |= c & 0x3f;
                    }
                    return ret;
                }
                else
                    return wchar_t(tmp);
            case utf16le:
            case utf16be:
                unsigned short w,w2;
                if (getWord(w) != 2)
                    return ret;
                if ((w & 0xfc00) == 0xd800) // high surrogate first
                {
                    if (getWord(w2) != 2)
                        return ret;
                    if ((w2 & 0xfc00) != 0xdc00)
                    {
                        cerr << "UTF-16 Error: invalid low surrogate" << endl;
                        return ret;
                    }
                    return (((w & 0x3ff) + 0x40) << 10) | (w2 & 0x3ff);
                }
                return w;
            case utf32le:
            case utf32be:
                if (getDWord(ret) != 4)
                    return wchar_t (-1);
                return ret;
        }
        return ret;
    }
    void convert2utf16le()
    {
        wchar_t c;
        unsigned char buffer[2] = {0xff, 0xfe};
        outputfile.write(reinterpret_cast<char*>(&buffer),2); // write BOM
        c = get_wchar_t();
        while (!inputfile.eof())
        {
            buffer[0] = c & 0xff;
            buffer[1] = (c >> 8) & 0xff; // create utf16-le char
            outputfile.write(reinterpret_cast<char*>(&buffer),2); // write char
            c = get_wchar_t();
        }
    }
    ~utf_converter()
    {
        if (inputfile)
            inputfile.close();
        if (outputfile)
            outputfile.close();
    }
};

const unsigned char utf_converter::utf8table[64] = {
1, 9, 17, 25, 33, 41, 49, 57, 65, 73, 81, 89, 97, 105, 113, 121,
129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 209, 217, 225, 233, 241, 249,
2, 10, 18, 26, 34, 42, 50, 58, 66, 74, 82, 90, 98, 106, 114, 122,
3, 11, 19, 27, 35, 43, 51, 59, 4, 12, 20, 28, 5, 13, 6, 7
};


int main(int argc, char* argv[])
{
    utf_converter::err_types err;
    if (argc < 3)
    {
        cout << "usage: " << argv[0] << " inputfile outputfile" << endl;
        return -1;
    }
    utf_converter conv(argv[1],argv[2]);
    if ((err = conv.getError())!=utf_converter::none)
    {
        switch (err)
        {
            case utf_converter::iopen:
                cerr << "Couldn't open input file." << endl;
                break;
            case utf_converter::oopen:
                cerr << "Couldn't open output file." << endl;
                break;
            default:
                cerr << "Unknown error." << endl;
        }
        return -1;
    } else
    conv.convert2utf16le();
    return 0;
}
