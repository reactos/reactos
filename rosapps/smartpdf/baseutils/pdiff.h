#ifndef PDIFF_H_
#define PDIFF_H_

/*
Copyright (C) 2006 Yangli Hector Yee

This program is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program;
if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

Code from http://pdiff.sourceforge.net
*/

class RGBAImage
{
public:
    virtual int Get_Width(void) const  = 0;
    virtual int Get_Height(void) const = 0;
    virtual unsigned char Get_Red(unsigned int i) = 0;
    virtual unsigned char Get_Green(unsigned int i) = 0;
    virtual unsigned char Get_Blue(unsigned int i) = 0;
    virtual unsigned char Get_Alpha(unsigned int i) = 0;
    virtual void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int i) = 0;
    virtual unsigned int Get(int i) const = 0;
};

class RGBAImageData : RGBAImage
{
public:
    RGBAImageData(int w, int h)
    {
        Width = w;
        Height = h;
        Data = new unsigned int[w * h];
    };
    ~RGBAImageData() { if (Data) delete[] Data; }
    unsigned char Get_Red(unsigned int i) { return (Data[i] & 0xFF); }
    unsigned char Get_Green(unsigned int i) { return ((Data[i]>>8) & 0xFF); }
    unsigned char Get_Blue(unsigned int i) { return ((Data[i]>>16) & 0xFF); }
    unsigned char Get_Alpha(unsigned int i) { return ((Data[i]>>24) & 0xFF); }
    void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int i)
    { Data[i] = r | (g << 8) | (b << 16) | (a << 24); }
    int Get_Width(void) const { return Width; }
    int Get_Height(void) const { return Height; }
    void Set(int x, int y, unsigned int d) { Data[x + y * Width] = d; }
    unsigned int Get(int x, int y) const { return Data[x + y * Width]; }
    unsigned int Get(int i) const { return Data[i]; }

protected:
    int             Width;
    int             Height;
    unsigned int *  Data;
};

class CompareArgs {
public:
    CompareArgs();
    ~CompareArgs();

    RGBAImage       *ImgA;
    RGBAImage       *ImgB;
    RGBAImage       *ImgDiff;
    float           FieldOfView;        // Field of view in degrees
    float           Gamma;              // The gamma to convert to linear color space
    float           Luminance;          // the display's luminance
};

#define DIFFERENT_SIZES (unsigned long)-1
#define IDENTICAL (unsigned long)0

unsigned long Yee_Compare(CompareArgs &args);

#endif

