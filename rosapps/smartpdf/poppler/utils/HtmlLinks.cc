#include "HtmlLinks.h"

HtmlLink::HtmlLink(const HtmlLink& x){
  Xmin=x.Xmin;
  Ymin=x.Ymin;
  Xmax=x.Xmax;
  Ymax=x.Ymax;
  dest=new GooString(x.dest);
}

HtmlLink::HtmlLink(double xmin,double ymin,double xmax,double ymax,GooString * _dest)
{
   if (xmin < xmax) {
    Xmin=xmin;
    Xmax=xmax;
  } else {
    Xmin=xmax;
    Xmax=xmin;
  }
  if (ymin < ymax) {
    Ymin=ymin;
    Ymax=ymax;
  } else {
    Ymin=ymax;
    Ymax=ymin;
  }                    
  dest=new GooString(_dest);
}

HtmlLink::~HtmlLink(){
 if (dest) delete dest;
}

GBool HtmlLink::isEqualDest(const HtmlLink& x) const{
  return (!strcmp(dest->getCString(), x.dest->getCString()));
}

GBool HtmlLink::inLink(double xmin,double ymin,double xmax,double ymax) const {
  double y=(ymin+ymax)/2;
  if (y>Ymax) return gFalse;
  return (y>Ymin)&&(xmin<Xmax)&&(xmax>Xmin);
 }
  

HtmlLink& HtmlLink::operator=(const HtmlLink& x){
  if (this==&x) return *this;
  if (dest) {delete dest;dest=NULL;} 
  Xmin=x.Xmin;
  Ymin=x.Ymin;
  Xmax=x.Xmax;
  Ymax=x.Ymax;
  dest=new GooString(x.dest);
  return *this;
} 

GooString* HtmlLink::getLinkStart() {
  GooString *res = new GooString("<A href=\"");
  res->append(dest);
  res->append("\">");
  return res;
}

/*GooString* HtmlLink::Link(GooString* content){
  //GooString* _dest=new GooString(dest);
  GooString *tmp=new GooString("<a href=\"");
  tmp->append(dest);
  tmp->append("\">");
  tmp->append(content);
  tmp->append("</a>");
  //delete _dest;
  return tmp;
  }*/

   

HtmlLinks::HtmlLinks(){
  accu=new GooVector<HtmlLink>();
}

HtmlLinks::~HtmlLinks(){
  delete accu;
  accu=NULL; 
}

GBool HtmlLinks::inLink(double xmin,double ymin,double xmax,double ymax,int& p)const {
  
  for(GooVector<HtmlLink>::iterator i=accu->begin();i!=accu->end();i++){
    if (i->inLink(xmin,ymin,xmax,ymax)) {
        p=(i - accu->begin());
        return 1;
    }
   }
  return 0;
}

HtmlLink* HtmlLinks::getLink(int i) const{
  GooVector<HtmlLink>::iterator g=accu->begin();
  g+=i; 
  return g;
}

