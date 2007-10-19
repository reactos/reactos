#ifndef _HTML_LINKS
#define _HTML_LINKS

#include <stdlib.h>
#include <string.h>
#include "goo/GooVector.h"
#include "goo/GooString.h"

class HtmlLink{

private:
  double Xmin;
  double Ymin;
  double Xmax;
  double Ymax;
  GooString* dest;

public:
  HtmlLink(){dest=NULL;}
  HtmlLink(const HtmlLink& x);
  HtmlLink& operator=(const HtmlLink& x);
  HtmlLink(double xmin,double ymin,double xmax,double ymax,GooString *_dest);
  ~HtmlLink();
  GBool isEqualDest(const HtmlLink& x) const;
  GooString *getDest(){return new GooString(dest);}
  double getX1() const {return Xmin;}
  double getX2() const {return Xmax;}
  double getY1() const {return Ymin;}
  double getY2() const {return Ymax;}
  GBool inLink(double xmin,double ymin,double xmax,double ymax) const ;
  //GooString *Link(GooString *content);
  GooString* getLinkStart();

};

class HtmlLinks{
private:
 GooVector<HtmlLink> *accu;
public:
 HtmlLinks();
 ~HtmlLinks();
 void AddLink(const HtmlLink& x) {accu->push_back(x);}
 GBool inLink(double xmin,double ymin,double xmax,double ymax,int& p) const;
 HtmlLink* getLink(int i) const;

};

#endif

