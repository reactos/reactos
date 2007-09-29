#ifndef _HTML_FONTS_H
#define _HTML_FONTS_H
#include "goo/GooVector.h"
#include "goo/GooString.h"
#include "GfxState.h"
#include "CharTypes.h"


class HtmlFontColor{
 private:
   unsigned int r;
   unsigned int g;
   unsigned int b;
   GBool Ok(unsigned int xcol){ return ((xcol<=255)&&(xcol>=0));}
   GooString *convtoX(unsigned  int xcol) const;
 public:
   HtmlFontColor():r(0),g(0),b(0){}
   HtmlFontColor(GfxRGB rgb);
   HtmlFontColor(const HtmlFontColor& x){r=x.r;g=x.g;b=x.b;}
   HtmlFontColor& operator=(const HtmlFontColor &x){
     r=x.r;g=x.g;b=x.b;
     return *this;
   }
   ~HtmlFontColor(){};
   GooString* toString() const;
   GBool isEqual(const HtmlFontColor& col) const{
     return ((r==col.r)&&(g==col.g)&&(b==col.b));
   }
} ;  


class HtmlFont{
 private:
   unsigned int size;
   int lineSize;
   GBool italic;
   GBool bold;
   int pos; // position of the font name in the fonts array
   static GooString *DefaultFont;
   GooString *FontName;
   HtmlFontColor color;
   static GooString* HtmlFilter(Unicode* u, int uLen); //char* s);
public:  

   HtmlFont(){FontName=NULL;};
   HtmlFont(GooString* fontname,int _size, GfxRGB rgb);
   HtmlFont(const HtmlFont& x);
   HtmlFont& operator=(const HtmlFont& x);
   HtmlFontColor getColor() const {return color;}
   ~HtmlFont();
   static void clear();
   GooString* getFullName();
   GBool isItalic() const {return italic;}
   GBool isBold() const {return bold;}
   unsigned int getSize() const {return size;}
   int getLineSize() const {return lineSize;}
   void setLineSize(int _lineSize) { lineSize = _lineSize; }
   GooString* getFontName();
   static GooString* getDefaultFont();
   static void setDefaultFont(GooString* defaultFont);
   GBool isEqual(const HtmlFont& x) const;
   GBool isEqualIgnoreBold(const HtmlFont& x) const;
   static GooString* simple(HtmlFont *font, Unicode *content, int uLen);
   void print() const {printf("font: %s %d %s%spos: %d\n", FontName->getCString(), size, bold ? "bold " : "", italic ? "italic " : "", pos);};
};

class HtmlFontAccu{
private:
  GooVector<HtmlFont> *accu;
  
public:
  HtmlFontAccu();
  ~HtmlFontAccu();
  int AddFont(const HtmlFont& font);
  HtmlFont* Get(int i){
    GooVector<HtmlFont>::iterator g=accu->begin();
    g+=i;  
    return g;
  } 
  GooString* getCSStyle (int i, GooString* content);
  GooString* CSStyle(int i);
  int size() const {return accu->size();}
  
};  
#endif
