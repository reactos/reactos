//========================================================================
//
// HtmlOutputDev.cc
//
// Copyright 1997-2002 Glyph & Cog, LLC
//
// Changed 1999-2000 by G.Ovtcharov
//
// Changed 2002 by Mikhail Kruk
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "UnicodeMap.h"
#include "goo/gmem.h"
#include "Error.h"
#include "GfxState.h"
#ifdef ENABLE_LIBJPEG
#include "DCTStream.h"
#endif
#include "GlobalParams.h"
#include "HtmlOutputDev.h"
#include "HtmlFonts.h"
#include "UGooString.h"

int HtmlPage::pgNum=0;
int HtmlOutputDev::imgNum=1;

extern double scale;
extern GBool complexMode;
extern GBool ignore;
extern GBool printCommands;
extern GBool printHtml;
extern GBool noframes;
extern GBool stout;
extern GBool xml;
extern GBool showHidden;
extern GBool noMerge;

static GooString* basename(GooString* str){
  
  char *p=str->getCString();
  int len=str->getLength();
  for (int i=len-1;i>=0;i--)
    if (*(p+i)==SLASH) 
      return new GooString((p+i+1),len-i-1);
  return new GooString(str);
}

static GooString* Dirname(GooString* str){
  
  char *p=str->getCString();
  int len=str->getLength();
  for (int i=len-1;i>=0;i--)
    if (*(p+i)==SLASH) 
      return new GooString(p,i+1);
  return new GooString();
} 

//------------------------------------------------------------------------
// HtmlString
//------------------------------------------------------------------------

HtmlString::HtmlString(GfxState *state, double fontSize, HtmlFontAccu* fonts) {
  GfxFont *font;
  double x, y;

  state->transform(state->getCurX(), state->getCurY(), &x, &y);
  if ((font = state->getFont())) {
    yMin = y - font->getAscent() * fontSize;
    yMax = y - font->getDescent() * fontSize;
    GfxRGB rgb;
    state->getFillRGB(&rgb);
    GooString *name = state->getFont()->getName();
    if (!name) name = HtmlFont::getDefaultFont(); //new GooString("default");
    HtmlFont hfont=HtmlFont(name, static_cast<int>(fontSize-1), rgb);
    fontpos = fonts->AddFont(hfont);
  } else {
    // this means that the PDF file draws text without a current font,
    // which should never happen
    yMin = y - 0.95 * fontSize;
    yMax = y + 0.35 * fontSize;
    fontpos=0;
  }
  if (yMin == yMax) {
    // this is a sanity check for a case that shouldn't happen -- but
    // if it does happen, we want to avoid dividing by zero later
    yMin = y;
    yMax = y + 1;
  }
  col = 0;
  text = NULL;
  xRight = NULL;
  link = NULL;
  len = size = 0;
  yxNext = NULL;
  xyNext = NULL;
  htext=new GooString();
  dir = textDirUnknown;
}


HtmlString::~HtmlString() {
  delete text;
  delete htext;
  gfree(xRight);
}

void HtmlString::addChar(GfxState *state, double x, double y,
			 double dx, double dy, Unicode u) {
  if (dir == textDirUnknown) {
    //dir = UnicodeMap::getDirection(u);
    dir = textDirLeftRight;
  } 

  if (len == size) {
    size += 16;
    text = (Unicode *)grealloc(text, size * sizeof(Unicode));
    xRight = (double *)grealloc(xRight, size * sizeof(double));
  }
  text[len] = u;
  if (len == 0) {
    xMin = x;
  }
  xMax = xRight[len] = x + dx;
//printf("added char: %f %f xright = %f\n", x, dx, x+dx);
  ++len;
}

void HtmlString::endString()
{
  if( dir == textDirRightLeft && len > 1 )
  {
    //printf("will reverse!\n");
    for (int i = 0; i < len / 2; i++)
    {
      Unicode ch = text[i];
      text[i] = text[len - i - 1];
      text[len - i - 1] = ch;
    }
  }
}

//------------------------------------------------------------------------
// HtmlPage
//------------------------------------------------------------------------

HtmlPage::HtmlPage(GBool rawOrder, char *imgExtVal) {
  this->rawOrder = rawOrder;
  curStr = NULL;
  yxStrings = NULL;
  xyStrings = NULL;
  yxCur1 = yxCur2 = NULL;
  fonts=new HtmlFontAccu();
  links=new HtmlLinks();
  pageWidth=0;
  pageHeight=0;
  fontsPageMarker = 0;
  DocName=NULL;
  firstPage = -1;
  imgExt = new GooString(imgExtVal);
}

HtmlPage::~HtmlPage() {
  clear();
  if (DocName) delete DocName;
  if (fonts) delete fonts;
  if (links) delete links;
  if (imgExt) delete imgExt;  
}

void HtmlPage::updateFont(GfxState *state) {
  GfxFont *font;
  double *fm;
  char *name;
  int code;
  double w;
  
  // adjust the font size
  fontSize = state->getTransformedFontSize();
  if ((font = state->getFont()) && font->getType() == fontType3) {
    // This is a hack which makes it possible to deal with some Type 3
    // fonts.  The problem is that it's impossible to know what the
    // base coordinate system used in the font is without actually
    // rendering the font.  This code tries to guess by looking at the
    // width of the character 'm' (which breaks if the font is a
    // subset that doesn't contain 'm').
    for (code = 0; code < 256; ++code) {
      if ((name = ((Gfx8BitFont *)font)->getCharName(code)) &&
	  name[0] == 'm' && name[1] == '\0') {
	break;
      }
    }
    if (code < 256) {
      w = ((Gfx8BitFont *)font)->getWidth(code);
      if (w != 0) {
	// 600 is a generic average 'm' width -- yes, this is a hack
	fontSize *= w / 0.6;
      }
    }
    fm = font->getFontMatrix();
    if (fm[0] != 0) {
      fontSize *= fabs(fm[3] / fm[0]);
    }
  }
}

void HtmlPage::beginString(GfxState *state, GooString *s) {
  curStr = new HtmlString(state, fontSize, fonts);
}


void HtmlPage::conv(){
  HtmlString *tmp;

  int linkIndex = 0;
  HtmlFont* h;
  for(tmp=yxStrings;tmp;tmp=tmp->yxNext){
     int pos=tmp->fontpos;
     //  printf("%d\n",pos);
     h=fonts->Get(pos);

     if (tmp->htext) delete tmp->htext; 
     tmp->htext=HtmlFont::simple(h,tmp->text,tmp->len);

     if (links->inLink(tmp->xMin,tmp->yMin,tmp->xMax,tmp->yMax, linkIndex)){
       tmp->link = links->getLink(linkIndex);
       /*GooString *t=tmp->htext;
       tmp->htext=links->getLink(k)->Link(tmp->htext);
       delete t;*/
     }
  }

}


void HtmlPage::addChar(GfxState *state, double x, double y,
		       double dx, double dy, 
			double ox, double oy, Unicode *u, int uLen) {
  double x1, y1, w1, h1, dx2, dy2;
  int n, i;
  state->transform(x, y, &x1, &y1);
  n = curStr->len;
 
  // check that new character is in the same direction as current string
  // and is not too far away from it before adding 
  //if ((UnicodeMap::getDirection(u[0]) != curStr->dir) || 
  // XXX
  if (
     (n > 0 && 
      fabs(x1 - curStr->xRight[n-1]) > 0.1 * (curStr->yMax - curStr->yMin))) {
    endString();
    beginString(state, NULL);
  }
  state->textTransformDelta(state->getCharSpace() * state->getHorizScaling(),
			    0, &dx2, &dy2);
  dx -= dx2;
  dy -= dy2;
  state->transformDelta(dx, dy, &w1, &h1);
  if (uLen != 0) {
    w1 /= uLen;
    h1 /= uLen;
  }
  for (i = 0; i < uLen; ++i) {
    curStr->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
  }
}

void HtmlPage::endString() {
  HtmlString *p1, *p2;
  double h, y1, y2;

  // throw away zero-length strings -- they don't have valid xMin/xMax
  // values, and they're useless anyway
  if (curStr->len == 0) {
    delete curStr;
    curStr = NULL;
    return;
  }

  curStr->endString();

#if 0 //~tmp
  if (curStr->yMax - curStr->yMin > 20) {
    delete curStr;
    curStr = NULL;
    return;
  }
#endif

  // insert string in y-major list
  h = curStr->yMax - curStr->yMin;
  y1 = curStr->yMin + 0.5 * h;
  y2 = curStr->yMin + 0.8 * h;
  if (rawOrder) {
    p1 = yxCur1;
    p2 = NULL;
  } else if ((!yxCur1 ||
              (y1 >= yxCur1->yMin &&
               (y2 >= yxCur1->yMax || curStr->xMax >= yxCur1->xMin))) &&
             (!yxCur2 ||
              (y1 < yxCur2->yMin ||
               (y2 < yxCur2->yMax && curStr->xMax < yxCur2->xMin)))) {
    p1 = yxCur1;
    p2 = yxCur2;
  } else {
    for (p1 = NULL, p2 = yxStrings; p2; p1 = p2, p2 = p2->yxNext) {
      if (y1 < p2->yMin || (y2 < p2->yMax && curStr->xMax < p2->xMin))
        break;
    }
    yxCur2 = p2;
  }
  yxCur1 = curStr;
  if (p1)
    p1->yxNext = curStr;
  else
    yxStrings = curStr;
  curStr->yxNext = p2;
  curStr = NULL;
}

void HtmlPage::coalesce() {
  HtmlString *str1, *str2;
  HtmlFont *hfont1, *hfont2;
  double space, horSpace, vertSpace, vertOverlap;
  GBool addSpace, addLineBreak;
  int n, i;
  double curX, curY;

#if 0 //~ for debugging
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    printf("x=%f..%f  y=%f..%f  size=%2d '",
	   str1->xMin, str1->xMax, str1->yMin, str1->yMax,
	   (int)(str1->yMax - str1->yMin));
    for (i = 0; i < str1->len; ++i) {
      fputc(str1->text[i] & 0xff, stdout);
    }
    printf("'\n");
  }
  printf("\n------------------------------------------------------------\n\n");
#endif
  str1 = yxStrings;

  if( !str1 ) return;

  //----- discard duplicated text (fake boldface, drop shadows)
  if( !complexMode )
  {	/* if not in complex mode get rid of duplicate strings */
	HtmlString *str3;
	GBool found;
  	while (str1)
	{
		double size = str1->yMax - str1->yMin;
		double xLimit = str1->xMin + size * 0.2;
		found = gFalse;
		for (str2 = str1, str3 = str1->yxNext;
			str3 && str3->xMin < xLimit;
			str2 = str3, str3 = str2->yxNext)
		{
			if (str3->len == str1->len &&
				!memcmp(str3->text, str1->text, str1->len * sizeof(Unicode)) &&
				fabs(str3->yMin - str1->yMin) < size * 0.2 &&
				fabs(str3->yMax - str1->yMax) < size * 0.2 &&
				fabs(str3->xMax - str1->xMax) < size * 0.2)
			{
				found = gTrue;
				//printf("found duplicate!\n");
				break;
			}
		}
		if (found)
		{
			str2->xyNext = str3->xyNext;
			str2->yxNext = str3->yxNext;
			delete str3;
		}
		else
		{
			str1 = str1->yxNext;
		}
	}		
  }	/*- !complexMode */
  
  str1 = yxStrings;
  
  hfont1 = getFont(str1);
  if( hfont1->isBold() )
    str1->htext->insert(0,"<b>",3);
  if( hfont1->isItalic() )
    str1->htext->insert(0,"<i>",3);
  if( str1->getLink() != NULL ) {
    GooString *ls = str1->getLink()->getLinkStart();
    str1->htext->insert(0, ls);
    delete ls;
  }
  curX = str1->xMin; curY = str1->yMin;

  while (str1 && (str2 = str1->yxNext)) {
    hfont2 = getFont(str2);
    space = str1->yMax - str1->yMin;
    horSpace = str2->xMin - str1->xMax;
    addLineBreak = !noMerge && (fabs(str1->xMin - str2->xMin) < 0.4);
    vertSpace = str2->yMin - str1->yMax;

//printf("coalesce %d %d %f? ", str1->dir, str2->dir, d);

    if (str2->yMin >= str1->yMin && str2->yMin <= str1->yMax)
    {
	vertOverlap = str1->yMax - str2->yMin;
    } else
    if (str2->yMax >= str1->yMin && str2->yMax <= str1->yMax)
    {
	vertOverlap = str2->yMax - str1->yMin;
    } else
    {
    	vertOverlap = 0;
    } 
    
    if (
	(
	 (
	  (
	   (rawOrder && vertOverlap > 0.5 * space) 
	   ||
	   (!rawOrder && str2->yMin < str1->yMax)
	  ) &&
	  (horSpace > -0.5 * space && horSpace < space)
	 ) ||
       	 (vertSpace >= 0 && vertSpace < 0.5 * space && addLineBreak)
	) &&
	(!complexMode || (hfont1->isEqualIgnoreBold(*hfont2))) && // in complex mode fonts must be the same, in other modes fonts do not metter
	str1->dir == str2->dir // text direction the same
       ) 
    {
//      printf("yes\n");
      n = str1->len + str2->len;
      if ((addSpace = horSpace > 0.1 * space)) {
        ++n;
      }
      if (addLineBreak) {
        ++n;
      }
  
      str1->size = (n + 15) & ~15;
      str1->text = (Unicode *)grealloc(str1->text,
				       str1->size * sizeof(Unicode));
      str1->xRight = (double *)grealloc(str1->xRight,
					str1->size * sizeof(double));
      if (addSpace) {
		  str1->text[str1->len] = 0x20;
		  str1->htext->append(" ");
		  str1->xRight[str1->len] = str2->xMin;
		  ++str1->len;
      }
      if (addLineBreak) {
	  str1->text[str1->len] = '\n';
	  str1->htext->append("<br>");
	  str1->xRight[str1->len] = str2->xMin;
	  ++str1->len;
	  str1->yMin = str2->yMin;
	  str1->yMax = str2->yMax;
	  str1->xMax = str2->xMax;
	  int fontLineSize = hfont1->getLineSize();
	  int curLineSize = (int)(vertSpace + space); 
	  if( curLineSize != fontLineSize )
	  {
	      HtmlFont *newfnt = new HtmlFont(*hfont1);
	      newfnt->setLineSize(curLineSize);
	      str1->fontpos = fonts->AddFont(*newfnt);
	      delete newfnt;
	      hfont1 = getFont(str1);
	      // we have to reget hfont2 because it's location could have
	      // changed on resize
	      hfont2 = getFont(str2); 
	  }
      }
      for (i = 0; i < str2->len; ++i) {
	str1->text[str1->len] = str2->text[i];
	str1->xRight[str1->len] = str2->xRight[i];
	++str1->len;
      }

      /* fix <i> and <b> if str1 and str2 differ */
      if( hfont1->isBold() && !hfont2->isBold() )
	str1->htext->append("</b>", 4);
      if( hfont1->isItalic() && !hfont2->isItalic() )
	str1->htext->append("</i>", 4);
      if( !hfont1->isBold() && hfont2->isBold() )
	str1->htext->append("<b>", 3);
      if( !hfont1->isItalic() && hfont2->isItalic() )
	str1->htext->append("<i>", 3);

      /* now handle switch of links */
      HtmlLink *hlink1 = str1->getLink();
      HtmlLink *hlink2 = str2->getLink();
      if( !hlink1 || !hlink2 || !hlink1->isEqualDest(*hlink2) ) {
	if(hlink1 != NULL )
	  str1->htext->append("</a>");
	if(hlink2 != NULL ) {
	  GooString *ls = hlink2->getLinkStart();
	  str1->htext->append(ls);
	  delete ls;
	}
      }

      str1->htext->append(str2->htext);
      // str1 now contains href for link of str2 (if it is defined)
      str1->link = str2->link; 
      hfont1 = hfont2;
      if (str2->xMax > str1->xMax) {
	str1->xMax = str2->xMax;
      }
      if (str2->yMax > str1->yMax) {
	str1->yMax = str2->yMax;
      }
      str1->yxNext = str2->yxNext;
      delete str2;
    } else { // keep strings separate
//      printf("no\n"); 
      if( hfont1->isBold() )
	str1->htext->append("</b>",4);
      if( hfont1->isItalic() )
	str1->htext->append("</i>",4);
      if(str1->getLink() != NULL )
	str1->htext->append("</a>");
     
      str1->xMin = curX; str1->yMin = curY; 
      str1 = str2;
      curX = str1->xMin; curY = str1->yMin;
      hfont1 = hfont2;
      if( hfont1->isBold() )
	str1->htext->insert(0,"<b>",3);
      if( hfont1->isItalic() )
	str1->htext->insert(0,"<i>",3);
      if( str1->getLink() != NULL ) {
	GooString *ls = str1->getLink()->getLinkStart();
	str1->htext->insert(0, ls);
	delete ls;
      }
    }
  }
  str1->xMin = curX; str1->yMin = curY;
  if( hfont1->isBold() )
    str1->htext->append("</b>",4);
  if( hfont1->isItalic() )
    str1->htext->append("</i>",4);
  if(str1->getLink() != NULL )
    str1->htext->append("</a>");

#if 0 //~ for debugging
  for (str1 = yxStrings; str1; str1 = str1->yxNext) {
    printf("x=%3d..%3d  y=%3d..%3d  size=%2d ",
	   (int)str1->xMin, (int)str1->xMax, (int)str1->yMin, (int)str1->yMax,
	   (int)(str1->yMax - str1->yMin));
    printf("'%s'\n", str1->htext->getCString());  
  }
  printf("\n------------------------------------------------------------\n\n");
#endif

}

void HtmlPage::dumpAsXML(FILE* f,int page){  
  fprintf(f, "<page number=\"%d\" position=\"absolute\"", page);
  fprintf(f," top=\"0\" left=\"0\" height=\"%d\" width=\"%d\">\n", pageHeight,pageWidth);
    
  for(int i=fontsPageMarker;i < fonts->size();i++) {
    GooString *fontCSStyle = fonts->CSStyle(i);
    fprintf(f,"\t%s\n",fontCSStyle->getCString());
    delete fontCSStyle;
  }
  
  GooString *str, *str1;
  for(HtmlString *tmp=yxStrings;tmp;tmp=tmp->yxNext){
    if (tmp->htext){
      str=new GooString(tmp->htext);
      fprintf(f,"<text top=\"%d\" left=\"%d\" ",xoutRound(tmp->yMin),xoutRound(tmp->xMin));
      fprintf(f,"width=\"%d\" height=\"%d\" ",xoutRound(tmp->xMax-tmp->xMin),xoutRound(tmp->yMax-tmp->yMin));
      fprintf(f,"font=\"%d\">", tmp->fontpos);
      if (tmp->fontpos!=-1){
	str1=fonts->getCSStyle(tmp->fontpos, str);
      }
      fputs(str1->getCString(),f);
      delete str;
      delete str1;
      fputs("</text>\n",f);
    }
  }
  fputs("</page>\n",f);
}


void HtmlPage::dumpComplex(FILE *file, int page){
  FILE* pageFile;
  GooString* tmp;
  char* htmlEncoding;

  if( firstPage == -1 ) firstPage = page; 
  
  if( !noframes )
  {
      GooString* pgNum=GooString::fromInt(page);
      tmp = new GooString(DocName);
      tmp->append('-')->append(pgNum)->append(".html");
      delete pgNum;
  
      if (!(pageFile = fopen(getFileNameFromPath(tmp->getCString(),tmp->getLength()), "w"))) {
	  error(-1, "Couldn't open html file '%s'", tmp->getCString());
	  delete tmp;
	  return;
      } 
      delete tmp;

      fprintf(pageFile,"%s\n<HTML>\n<HEAD>\n<TITLE>Page %d</TITLE>\n\n",
	      DOCTYPE, page);

      htmlEncoding = HtmlOutputDev::mapEncodingToHtml
	  (globalParams->getTextEncodingName());
      fprintf(pageFile, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", htmlEncoding);
  }
  else 
  {
      pageFile = file;
      fprintf(pageFile,"<!-- Page %d -->\n", page);
      fprintf(pageFile,"<a name=\"%d\"></a>\n", page);
  } 
  
  fprintf(pageFile,"<DIV style=\"position:relative;width:%d;height:%d;\">\n",
	pageWidth, pageHeight);

  tmp=basename(DocName);
   
  fputs("<STYLE type=\"text/css\">\n<!--\n",pageFile);
  for(int i=fontsPageMarker;i!=fonts->size();i++) {
    GooString *fontCSStyle = fonts->CSStyle(i);
    fprintf(pageFile,"\t%s\n",fontCSStyle->getCString());
    delete fontCSStyle;
  }
 
  fputs("-->\n</STYLE>\n",pageFile);
  
  if( !noframes )
  {  
      fputs("</HEAD>\n<BODY bgcolor=\"#A0A0A0\" vlink=\"blue\" link=\"blue\">\n",pageFile); 
  }
  
  if( !ignore ) 
  {
    fprintf(pageFile,
	    "<IMG width=\"%d\" height=\"%d\" src=\"%s%03d.%s\" alt=\"background image\">\n",
	    pageWidth, pageHeight, tmp->getCString(), 
		(page-firstPage+1), imgExt->getCString());
  }
  
  delete tmp;
  
  GooString *str, *str1;
  for(HtmlString *tmp1=yxStrings;tmp1;tmp1=tmp1->yxNext){
    if (tmp1->htext){
      str=new GooString(tmp1->htext);
      fprintf(pageFile,
	      "<DIV style=\"position:absolute;top:%d;left:%d\">",
	      xoutRound(tmp1->yMin),
	      xoutRound(tmp1->xMin));
      fputs("<nobr>",pageFile); 
      if (tmp1->fontpos!=-1){
	str1=fonts->getCSStyle(tmp1->fontpos, str);  
      }
      //printf("%s\n", str1->getCString());
      fputs(str1->getCString(),pageFile);
      
      delete str;      
      delete str1;
      fputs("</nobr></DIV>\n",pageFile);
    }
  }

  fputs("</DIV>\n", pageFile);
  
  if( !noframes )
  {
      fputs("</BODY>\n</HTML>\n",pageFile);
      fclose(pageFile);
  }
}


void HtmlPage::dump(FILE *f, int pageNum) 
{
  if (complexMode)
  {
    if (xml) dumpAsXML(f, pageNum);
    if (!xml) dumpComplex(f, pageNum);  
  }
  else
  {
    fprintf(f,"<A name=%d></a>",pageNum);
    GooString* fName=basename(DocName); 
    for (int i=1;i<HtmlOutputDev::imgNum;i++)
      fprintf(f,"<IMG src=\"%s-%d_%d.jpg\"><br>\n",fName->getCString(),pageNum,i);
    HtmlOutputDev::imgNum=1;
    delete fName;

    GooString* str;
    for(HtmlString *tmp=yxStrings;tmp;tmp=tmp->yxNext){
      if (tmp->htext){
		str=new GooString(tmp->htext); 
		fputs(str->getCString(),f);
		delete str;      
		fputs("<br>\n",f);  
      }
    }
	fputs("<hr>\n",f);  
  }
}



void HtmlPage::clear() {
  HtmlString *p1, *p2;

  if (curStr) {
    delete curStr;
    curStr = NULL;
  }
  for (p1 = yxStrings; p1; p1 = p2) {
    p2 = p1->yxNext;
    delete p1;
  }
  yxStrings = NULL;
  xyStrings = NULL;
  yxCur1 = yxCur2 = NULL;

  if( !noframes )
  {
      delete fonts;
      fonts=new HtmlFontAccu();
      fontsPageMarker = 0;
  }
  else
  {
      fontsPageMarker = fonts->size();
  }

  delete links;
  links=new HtmlLinks();
 

}

void HtmlPage::setDocName(char *fname){
  DocName=new GooString(fname);
}

//------------------------------------------------------------------------
// HtmlMetaVar
//------------------------------------------------------------------------

HtmlMetaVar::HtmlMetaVar(char *_name, char *_content)
{
    name = new GooString(_name);
    content = new GooString(_content);
}

HtmlMetaVar::~HtmlMetaVar()
{
   delete name;
   delete content;
} 
    
GooString* HtmlMetaVar::toString()	
{
    GooString *result = new GooString("<META name=\"");
    result->append(name);
    result->append("\" content=\"");
    result->append(content);
    result->append("\">"); 
    return result;
}

//------------------------------------------------------------------------
// HtmlOutputDev
//------------------------------------------------------------------------

static char* HtmlEncodings[][2] = {
    {"Latin1", "ISO-8859-1"},
    {NULL, NULL}
};


char* HtmlOutputDev::mapEncodingToHtml(GooString* encoding)
{
    char* enc = encoding->getCString();
    for(int i = 0; HtmlEncodings[i][0] != NULL; i++)
    {
	if( strcmp(enc, HtmlEncodings[i][0]) == 0 )
	{
	    return HtmlEncodings[i][1];
	}
    }
    return enc; 
}

void HtmlOutputDev::doFrame(int firstPage){
  GooString* fName=new GooString(Docname);
  char* htmlEncoding;
  fName->append(".html");

  if (!(fContentsFrame = fopen(getFileNameFromPath(fName->getCString(),fName->getLength()), "w"))){
    delete fName;
    error(-1, "Couldn't open html file '%s'", fName->getCString());
    return;
  }
  
  delete fName;
    
  fName=basename(Docname);
  fputs(DOCTYPE_FRAMES, fContentsFrame);
  fputs("\n<HTML>",fContentsFrame);
  fputs("\n<HEAD>",fContentsFrame);
  fprintf(fContentsFrame,"\n<TITLE>%s</TITLE>",docTitle->getCString());
  htmlEncoding = mapEncodingToHtml(globalParams->getTextEncodingName());
  fprintf(fContentsFrame, "\n<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", htmlEncoding);
  dumpMetaVars(fContentsFrame);
  fprintf(fContentsFrame, "</HEAD>\n");
  fputs("<FRAMESET cols=\"100,*\">\n",fContentsFrame);
  fprintf(fContentsFrame,"<FRAME name=\"links\" src=\"%s_ind.html\">\n",fName->getCString());
  fputs("<FRAME name=\"contents\" src=",fContentsFrame); 
  if (complexMode) 
      fprintf(fContentsFrame,"\"%s-%d.html\"",fName->getCString(), firstPage);
  else
      fprintf(fContentsFrame,"\"%ss.html\"",fName->getCString());
  
  fputs(">\n</FRAMESET>\n</HTML>\n",fContentsFrame);
 
  delete fName;
  fclose(fContentsFrame);  
}

HtmlOutputDev::HtmlOutputDev(char *fileName, char *title, 
	char *author, char *keywords, char *subject, char *date,
	char *extension,
	GBool rawOrder, int firstPage, GBool outline) 
{
  char *htmlEncoding;
  
  fContentsFrame = NULL;
  docTitle = new GooString(title);
  pages = NULL;
  dumpJPEG=gTrue;
  //write = gTrue;
  this->rawOrder = rawOrder;
  this->doOutline = outline;
  ok = gFalse;
  imgNum=1;
  //this->firstPage = firstPage;
  //pageNum=firstPage;
  // open file
  needClose = gFalse;
  pages = new HtmlPage(rawOrder, extension);
  
  glMetaVars = new GooList();
  glMetaVars->append(new HtmlMetaVar("generator", "pdftohtml 0.36"));  
  if( author ) glMetaVars->append(new HtmlMetaVar("author", author));  
  if( keywords ) glMetaVars->append(new HtmlMetaVar("keywords", keywords));  
  if( date ) glMetaVars->append(new HtmlMetaVar("date", date));  
  if( subject ) glMetaVars->append(new HtmlMetaVar("subject", subject));
 
  maxPageWidth = 0;
  maxPageHeight = 0;

  pages->setDocName(fileName);
  Docname=new GooString (fileName);

  // for non-xml output (complex or simple) with frames generate the left frame
  if(!xml && !noframes)
  {
     GooString* left=new GooString(fileName);
     left->append("_ind.html");

     doFrame(firstPage);
   
     if (!(fContentsFrame = fopen(getFileNameFromPath(left->getCString(),left->getLength()), "w")))
	 {
        error(-1, "Couldn't open html file '%s'", left->getCString());
		delete left;
        return;
     }
     delete left;
     fputs(DOCTYPE, fContentsFrame);
     fputs("<HTML>\n<HEAD>\n<TITLE></TITLE>\n</HEAD>\n<BODY>\n",fContentsFrame);
     
  	if (doOutline)
	{
		GooString *str = basename(Docname);
		fprintf(fContentsFrame, "<A href=\"%s%s\" target=\"contents\">Outline</a><br>", str->getCString(), complexMode ? "-outline.html" : "s.html#outline");
		delete str;
	}
  	
	if (!complexMode)
	{	/* not in complex mode */
		
       GooString* right=new GooString(fileName);
       right->append("s.html");

       if (!(page=fopen(getFileNameFromPath(right->getCString(),right->getLength()),"w"))){
        error(-1, "Couldn't open html file '%s'", right->getCString());
        delete right;
		return;
       }
       delete right;
       fputs(DOCTYPE, page);
       fputs("<HTML>\n<HEAD>\n<TITLE></TITLE>\n</HEAD>\n<BODY>\n",page);
     }
  }

  if (noframes) {
    if (stout) page=stdout;
    else {
      GooString* right=new GooString(fileName);
      if (!xml) right->append(".html");
      if (xml) right->append(".xml");
      if (!(page=fopen(getFileNameFromPath(right->getCString(),right->getLength()),"w"))){
	delete right;
	error(-1, "Couldn't open html file '%s'", right->getCString());
	return;
      }  
      delete right;
    }

    htmlEncoding = mapEncodingToHtml(globalParams->getTextEncodingName()); 
    if (xml) 
    {
      fprintf(page, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", htmlEncoding);
      fputs("<!DOCTYPE pdf2xml SYSTEM \"pdf2xml.dtd\">\n\n", page);
      fputs("<pdf2xml>\n",page);
    } 
    else 
    {
      fprintf(page,"%s\n<HTML>\n<HEAD>\n<TITLE>%s</TITLE>\n",
	      DOCTYPE, docTitle->getCString());
      
      fprintf(page, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n", htmlEncoding);
      
      dumpMetaVars(page);
      fprintf(page,"</HEAD>\n");
      fprintf(page,"<BODY bgcolor=\"#A0A0A0\" vlink=\"blue\" link=\"blue\">\n");
    }
  }
  ok = gTrue; 
}

HtmlOutputDev::~HtmlOutputDev() {
  /*if (mode&&!xml){
    int h=xoutRound(pages->pageHeight/scale);
    int w=xoutRound(pages->pageWidth/scale);
    fprintf(tin,"%s=%03d\n","PAPER_WIDTH",w);
    fprintf(tin,"%s=%03d\n","PAPER_HEIGHT",h);
    fclose(tin);
    }*/

    HtmlFont::clear(); 
    
    delete Docname;
    delete docTitle;

    deleteGooList(glMetaVars, HtmlMetaVar);

    if (fContentsFrame){
      fputs("</BODY>\n</HTML>\n",fContentsFrame);  
      fclose(fContentsFrame);
    }
    if (xml) {
      fputs("</pdf2xml>\n",page);  
      fclose(page);
    } else
    if ( !complexMode || xml || noframes )
    { 
      fputs("</BODY>\n</HTML>\n",page);  
      fclose(page);
    }
    if (pages)
      delete pages;
}



void HtmlOutputDev::startPage(int pageNum, GfxState *state) {
  /*if (mode&&!xml){
    if (write){
      write=gFalse;
      GooString* fname=Dirname(Docname);
      fname->append("image.log");
      if((tin=fopen(getFileNameFromPath(fname->getCString(),fname->getLength()),"w"))==NULL){
	printf("Error : can not open %s",fname);
	exit(1);
      }
      delete fname;
    // if(state->getRotation()!=0) 
    //  fprintf(tin,"ROTATE=%d rotate %d neg %d neg translate\n",state->getRotation(),state->getX1(),-state->getY1());
    // else 
      fprintf(tin,"ROTATE=%d neg %d neg translate\n",state->getX1(),state->getY1());  
    }
  }*/

  this->pageNum = pageNum;
  GooString *str=basename(Docname);
  pages->clear(); 
  if(!noframes)
  {
    if (fContentsFrame)
	{
      if (complexMode)
		fprintf(fContentsFrame,"<A href=\"%s-%d.html\"",str->getCString(),pageNum);
      else 
		fprintf(fContentsFrame,"<A href=\"%ss.html#%d\"",str->getCString(),pageNum);
      fprintf(fContentsFrame," target=\"contents\" >Page %d</a><br>\n",pageNum);
    }
  }

  pages->pageWidth=static_cast<int>(state->getPageWidth());
  pages->pageHeight=static_cast<int>(state->getPageHeight());

  delete str;
} 


void HtmlOutputDev::endPage() {
  pages->conv();
  pages->coalesce();
  pages->dump(page, pageNum);
  
  // I don't yet know what to do in the case when there are pages of different
  // sizes and we want complex output: running ghostscript many times 
  // seems very inefficient. So for now I'll just use last page's size
  maxPageWidth = pages->pageWidth;
  maxPageHeight = pages->pageHeight;
  
  //if(!noframes&&!xml) fputs("<br>\n", fContentsFrame);
  if(!stout && !globalParams->getErrQuiet()) printf("Page-%d\n",(pageNum));
}

void HtmlOutputDev::updateFont(GfxState *state) {
  pages->updateFont(state);
}

void HtmlOutputDev::beginString(GfxState *state, GooString *s) {
  pages->beginString(state, s);
}

void HtmlOutputDev::endString(GfxState *state) {
  pages->endString();
}

void HtmlOutputDev::drawChar(GfxState *state, double x, double y,
	      double dx, double dy,
	      double originX, double originY,
	      CharCode code, int /*nBytes*/, Unicode *u, int uLen) 
{
  if ( !showHidden && (state->getRender() & 3) == 3) {
    return;
  }
  pages->addChar(state, x, y, dx, dy, originX, originY, u, uLen);
}

void HtmlOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
			      int width, int height, GBool invert,
			      GBool inlineImg) {

  int i, j;
  
  if (ignore||complexMode) {
    OutputDev::drawImageMask(state, ref, str, width, height, invert, inlineImg);
    return;
  }
  
  FILE *f1;
  int c;
  
  int x0, y0;			// top left corner of image
  int w0, h0, w1, h1;		// size of image
  double xt, yt, wt, ht;
  GBool rotate, xFlip, yFlip;
  GBool dither;
  int x, y;
  int ix, iy;
  int px1, px2, qx, dx;
  int py1, py2, qy, dy;
  Gulong pixel;
  int nComps, nVals, nBits;
  double r1, g1, b1;
 
  // get image position and size
  state->transform(0, 0, &xt, &yt);
  state->transformDelta(1, 1, &wt, &ht);
  if (wt > 0) {
    x0 = xoutRound(xt);
    w0 = xoutRound(wt);
  } else {
    x0 = xoutRound(xt + wt);
    w0 = xoutRound(-wt);
  }
  if (ht > 0) {
    y0 = xoutRound(yt);
    h0 = xoutRound(ht);
  } else {
    y0 = xoutRound(yt + ht);
    h0 = xoutRound(-ht);
  }
  state->transformDelta(1, 0, &xt, &yt);
  rotate = fabs(xt) < fabs(yt);
  if (rotate) {
    w1 = h0;
    h1 = w0;
    xFlip = ht < 0;
    yFlip = wt > 0;
  } else {
    w1 = w0;
    h1 = h0;
    xFlip = wt < 0;
    yFlip = ht > 0;
  }

  // dump JPEG file
  if (dumpJPEG  && str->getKind() == strDCT) {
    GooString *fName=new GooString(Docname);
    fName->append("-");
    GooString *pgNum=GooString::fromInt(pageNum);
    GooString *imgnum=GooString::fromInt(imgNum);
    // open the image file
    fName->append(pgNum)->append("_")->append(imgnum)->append(".jpg");
    ++imgNum;
    if (!(f1 = fopen(getFileNameFromPath(fName->getCString(),fName->getLength()), "wb"))) {
      error(-1, "Couldn't open image file '%s'", fName->getCString());
      return;
    }

    // initialize stream
    str = ((DCTStream *)str)->getRawStream();
    str->reset();

    // copy the stream
    while ((c = str->getChar()) != EOF)
      fputc(c, f1);

    fclose(f1);
   
  if (pgNum) delete pgNum;
  if (imgnum) delete imgnum;
  if (fName) delete fName;
  }
  else {
    OutputDev::drawImageMask(state, ref, str, width, height, invert, inlineImg);
  }
}

void HtmlOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
			  int width, int height, GfxImageColorMap *colorMap,
			  int *maskColors, GBool inlineImg) {

  int i, j;
   
  if (ignore||complexMode) {
    OutputDev::drawImage(state, ref, str, width, height, colorMap, 
			 maskColors, inlineImg);
    return;
  }

  FILE *f1;
  ImageStream *imgStr;
  Guchar pixBuf[4];
  GfxColor color;
  int c;
  
  int x0, y0;			// top left corner of image
  int w0, h0, w1, h1;		// size of image
  double xt, yt, wt, ht;
  GBool rotate, xFlip, yFlip;
  GBool dither;
  int x, y;
  int ix, iy;
  int px1, px2, qx, dx;
  int py1, py2, qy, dy;
  Gulong pixel;
  int nComps, nVals, nBits;
  double r1, g1, b1;
 
  // get image position and size
  state->transform(0, 0, &xt, &yt);
  state->transformDelta(1, 1, &wt, &ht);
  if (wt > 0) {
    x0 = xoutRound(xt);
    w0 = xoutRound(wt);
  } else {
    x0 = xoutRound(xt + wt);
    w0 = xoutRound(-wt);
  }
  if (ht > 0) {
    y0 = xoutRound(yt);
    h0 = xoutRound(ht);
  } else {
    y0 = xoutRound(yt + ht);
    h0 = xoutRound(-ht);
  }
  state->transformDelta(1, 0, &xt, &yt);
  rotate = fabs(xt) < fabs(yt);
  if (rotate) {
    w1 = h0;
    h1 = w0;
    xFlip = ht < 0;
    yFlip = wt > 0;
  } else {
    w1 = w0;
    h1 = h0;
    xFlip = wt < 0;
    yFlip = ht > 0;
  }

   
  /*if( !globalParams->getErrQuiet() )
    printf("image stream of kind %d\n", str->getKind());*/
  // dump JPEG file
  if (dumpJPEG && str->getKind() == strDCT) {
    GooString *fName=new GooString(Docname);
    fName->append("-");
    GooString *pgNum= GooString::fromInt(pageNum);
    GooString *imgnum= GooString::fromInt(imgNum);  
    
    // open the image file
    fName->append(pgNum)->append("_")->append(imgnum)->append(".jpg");
    ++imgNum;
    
    if (!(f1 = fopen(getFileNameFromPath(fName->getCString(),fName->getLength()), "wb"))) {
      error(-1, "Couldn't open image file '%s'", fName->getCString());
      return;
    }

    // initialize stream
    str = ((DCTStream *)str)->getRawStream();
    str->reset();

    // copy the stream
    while ((c = str->getChar()) != EOF)
      fputc(c, f1);
    
    fclose(f1);
  
    delete fName;
    delete pgNum;
    delete imgnum;
  }
  else {
    OutputDev::drawImage(state, ref, str, width, height, colorMap,
			 maskColors, inlineImg);
  }
}



void HtmlOutputDev::drawLink(Link* link,Catalog *cat){
  double _x1,_y1,_x2,_y2,w;
  int x1,y1,x2,y2;
  
  link->getRect(&_x1,&_y1,&_x2,&_y2);
  w = link->getBorderStyle()->getWidth();
  cvtUserToDev(_x1,_y1,&x1,&y1);
  
  cvtUserToDev(_x2,_y2,&x2,&y2); 


  GooString* _dest=getLinkDest(link,cat);
  HtmlLink t((double) x1,(double) y2,(double) x2,(double) y1,_dest);
  pages->AddLink(t);
  delete _dest;
}

GooString* HtmlOutputDev::getLinkDest(Link *link,Catalog* catalog){
  char *p;
  switch(link->getAction()->getKind()) 
  {
      case actionGoTo:
	  { 
	  GooString* file=basename(Docname);
	  int page=1;
	  LinkGoTo *ha=(LinkGoTo *)link->getAction();
	  LinkDest *dest=NULL;
	  if (ha->getDest()==NULL) 
	      dest=catalog->findDest(ha->getNamedDest());
	  else 
	      dest=ha->getDest()->copy();
	  if (dest){ 
	      if (dest->isPageRef()){
		  Ref pageref=dest->getPageRef();
		  page=catalog->findPage(pageref.num,pageref.gen);
	      }
	      else {
		  page=dest->getPageNum();
	      }

	      delete dest;

	      GooString *str=GooString::fromInt(page);
	      /* 		complex 	simple
	       	frames		file-4.html	files.html#4
		noframes	file.html#4	file.html#4
	       */
	      if (noframes)
	      {
		  file->append(".html#");
		  file->append(str);
	      }
	      else
	      {
	      	if( complexMode ) 
		{
		    file->append("-");
		    file->append(str);
		    file->append(".html");
		}
		else
		{
		    file->append("s.html#");
		    file->append(str);
		}
	      }

	      if (printCommands) printf(" link to page %d ",page);
	      delete str;
	      return file;
	  }
	  else 
	  {
	      return new GooString();
	  }
	  }
      case actionGoToR:
	  {
	  LinkGoToR *ha=(LinkGoToR *) link->getAction();
	  LinkDest *dest=NULL;
	  int page=1;
	  GooString *file=new GooString();
	  if (ha->getFileName()){
	      delete file;
	      file=new GooString(ha->getFileName()->getCString());
	  }
	  if (ha->getDest()!=NULL)  dest=ha->getDest()->copy();
	  if (dest&&file){
	      if (!(dest->isPageRef()))  page=dest->getPageNum();
	      delete dest;

	      if (printCommands) printf(" link to page %d ",page);
	      if (printHtml){
		  p=file->getCString()+file->getLength()-4;
		  if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")){
		      file->del(file->getLength()-4,4);
		      file->append(".html");
		  }
		  file->append('#');
		  file->append(GooString::fromInt(page));
	      }
	  }
	  if (printCommands) printf("filename %s\n",file->getCString());
	  return file;
	  }
      case actionURI:
	  { 
	  LinkURI *ha=(LinkURI *) link->getAction();
	  GooString* file=new GooString(ha->getURI()->getCString());
	  // printf("uri : %s\n",file->getCString());
	  return file;
	  }
      case actionLaunch:
	  {
	  LinkLaunch *ha=(LinkLaunch *) link->getAction();
	  GooString* file=new GooString(ha->getFileName()->getCString());
	  if (printHtml) { 
	      p=file->getCString()+file->getLength()-4;
	      if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")){
		  file->del(file->getLength()-4,4);
		  file->append(".html");
	      }
	      if (printCommands) printf("filename %s",file->getCString());
    
	      return file;      
  
	  }
	  }
      default:
	  return new GooString();
  }
}

void HtmlOutputDev::dumpMetaVars(FILE *file)
{
  GooString *var;

  for(int i = 0; i < glMetaVars->getLength(); i++)
  {
     HtmlMetaVar *t = (HtmlMetaVar*)glMetaVars->get(i); 
     var = t->toString(); 
     fprintf(file, "%s\n", var->getCString());
     delete var;
  }
}

GBool HtmlOutputDev::dumpDocOutline(Catalog* catalog)
{ 
	FILE * output;
	GBool bClose = gFalse;

	if (!ok || xml)
    	return gFalse;
  
	Object *outlines = catalog->getOutline();
  	if (!outlines->isDict())
    	return gFalse;
  
	if (!complexMode && !xml)
  	{
		output = page;
  	}
  	else if (complexMode && !xml)
	{
		if (noframes)
		{
			output = page; 
			fputs("<hr>\n", output);
		}
		else
		{
			GooString *str = basename(Docname);
			str->append("-outline.html");
			output = fopen(getFileNameFromPath(str->getCString(),str->getLength()), "w");
			if (output == NULL)
				return gFalse;
			delete str;
			bClose = gTrue;
     		fputs("<HTML>\n<HEAD>\n<TITLE>Document Outline</TITLE>\n</HEAD>\n<BODY>\n", output);
		}
	}
 
  	GBool done = newOutlineLevel(output, outlines, catalog);
  	if (done && !complexMode)
    	fputs("<hr>\n", output);
	
	if (bClose)
	{
		fputs("</BODY>\n</HTML>\n", output);
		fclose(output);
	}
  	return done;
}

GBool HtmlOutputDev::newOutlineLevel(FILE *output, Object *node, Catalog* catalog, int level)
{
  Object curr, next;
  GBool atLeastOne = gFalse;
  
  if (node->dictLookup("First", &curr)->isDict()) {
    if (level == 1)
	{
		fputs("<A name=\"outline\"></a>", output);
		fputs("<h1>Document Outline</h1>\n", output);
	}
    fputs("<ul>",output);
    do {
      // get title, give up if not found
      Object title;
      if (curr.dictLookup("Title", &title)->isNull()) {
		title.free();
		break;
      }
      GooString *titleStr = new GooString(title.getString());
      title.free();

      // get corresponding link
      // Note: some code duplicated from HtmlOutputDev::getLinkDest().
      GooString *linkName = NULL;;
      Object dest;
      if (!curr.dictLookup("Dest", &dest)->isNull()) {
		LinkGoTo *link = new LinkGoTo(&dest);
		LinkDest *linkdest=NULL;
		if (link->getDest()==NULL) 
	  		linkdest=catalog->findDest(link->getNamedDest());
		else 
	  		linkdest=link->getDest()->copy();
		delete link;
		if (linkdest) { 
	  		int page;
	  		if (linkdest->isPageRef()) {
	    		Ref pageref=linkdest->getPageRef();
	    		page=catalog->findPage(pageref.num,pageref.gen);
	  		} else {
	    		page=linkdest->getPageNum();
	  		}
	  		delete linkdest;

			/* 			complex 	simple
			frames		file-4.html	files.html#4
			noframes	file.html#4	file.html#4
	   		*/
	  		linkName=basename(Docname);
	  		GooString *str=GooString::fromInt(page);
	  		if (noframes) {
	    		linkName->append(".html#");
				linkName->append(str);
	  		} else {
    			if( complexMode ) {
	   		   		linkName->append("-");
	      			linkName->append(str);
	      			linkName->append(".html");
	    		} else {
	      			linkName->append("s.html#");
	      			linkName->append(str);
	    		}
	  		}
			delete str;
		}
      }
      dest.free();

      fputs("<li>",output);
      if (linkName)
		fprintf(output,"<A href=\"%s\">", linkName->getCString());
      fputs(titleStr->getCString(),output);
      if (linkName) {
		fputs("</A>",output);
		delete linkName;
      }
      fputs("\n",output);
      delete titleStr;
      atLeastOne = gTrue;

      newOutlineLevel(output, &curr, catalog, level+1);
      curr.dictLookup("Next", &next);
      curr.free();
      curr = next;
    } while(curr.isDict());
    fputs("</ul>",output);
  }
  curr.free();

  return atLeastOne;
}

char* getFileNameFromPath(char* c, int strlen) {
  int last_slash_index = 0;
  int i = 0;
  char* res;
  
  for (i=0;i<strlen;i++) {
    if (*(c+i)=='/') {
      /* printf("/ detected\n"); */
      last_slash_index = i;      
    }
  }
  res = (char *)malloc(sizeof(char)*strlen-last_slash_index+1);
  strcpy(res,c+last_slash_index+(last_slash_index?1:0));
  /* printf("Fil: %s\n",res); */
  return res;
}
