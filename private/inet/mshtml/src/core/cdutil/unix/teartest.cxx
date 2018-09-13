#include <stdio.h>

#define EMPTY_CVOID
#ifdef EMPTY_CVOID

class CVoid
{
    virtual void _dummy()=0;
};

/*
(/ms/SUNWspro/bin/dbx) examine pd /20
0xefffe040:	 0x000213e0 0x00000001 0x00000002 0x000213b8
0xefffe050:	 0x00000003 0x00010a50 0x00000001 0xefffe0bc
0xefffe060:	 0xefffe0c4 0x00021000 0x00000000 0x00000000
0xefffe070:	 0x00000000 0x00000000 0x00000000 0x00000000
0xefffe080:	 0x00000000 0x00000000 0x00000000 0x00000000
(/ms/SUNWspro/bin/dbx) examine 0x213b8 /40
0x000213b8: __0dECDerG__vtbl       :	 0x00000000 0x00000000 0x00000000 0x00010b78
0x000213c8: __0dECDerG__vtbl+0x0010:	 0x00000000 0x00010cf0 0x00000000 0x00010ca8
0x000213d8: __0dECDerG__vtbl+0x0020:	 0x00000000 0x00010d38 0x00000000 0x00000000 <-- *this
0x000213e8: __0dECDerG__vtbl+0x0030:	 0x00000000 0x00010cf0 0x00000000 0x00010ca8
0x000213f8: __0dFCBaseG__vtbl       :	 0x00000000 0x00000000 0x00000000 0x00010e00
0x00021408: __0dFCBaseG__vtbl+0x0010:	 0x00000000 0x00010cf0 0x00000000 0x00010ca8
0x00021418: __0dFCBaseG__vtbl+0x0020:	 0x00000000 0x00021340 0x00000000 0x00000000
0x00021428: __0dFCBaseG__vtbl+0x0030:	 0x00000000 0x00010cf0 0x00000000 0x00010ca8
0x00021438: __0dEIUnkG__vtbl       :	 0x00000000 0x00000000 0x00000000 0x00021340
0x00021448: __0dEIUnkG__vtbl+0x0010:	 0x00000000 0x00021340 0x00000000 0x00000000
(/ms/SUNWspro/bin/dbx) examine 0x10b78
0x00010b78: ~CDer       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10cf0
0x00010cf0: a       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10ca8
0x00010ca8: b       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10d38
0x00010d38: c       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10e00
0x00010e00: ~CBase       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx)
*/

#else

class CVoid
{
    virtual void _dummy() {}
};

/*
(/ms/SUNWspro/bin/dbx) examine 0x2142c /40
0x0002142c: __0dECDerG__vtbl       :	 0x00000000 0x00000000 0x00000000 0x00010ca8
0x0002143c: __0dECDerG__vtbl+0x0010:	 0x00000000 0x00010b78 0x00000000 0x00010d10
0x0002144c: __0dECDerG__vtbl+0x0020:	 0x00000000 0x00010cc8 0x00000000 0x00010d58
0x0002145c: __0dECDerG__vtbl+0x0030:	 0x00000000 0x00000000 0xfffc0000 0x00010d10
0x0002146c: __0dECDerG__vtbl+0x0040:	 0xfffc0000 0x00010cc8 0x00000000 0x00000000
0x0002147c: __0dFCBaseG__vtbl+0x0008:	 0x00000000 0x00010ca8 0x00000000 0x00010e38
0x0002148c: __0dFCBaseG__vtbl+0x0018:	 0x00000000 0x00010d10 0x00000000 0x00010cc8
0x0002149c: __0dFCBaseG__vtbl+0x0028:	 0x00000000 0x000213b0 0x00000000 0x00000000
0x000214ac: __0dFCBaseG__vtbl+0x0038:	 0xfffc0000 0x00010d10 0xfffc0000 0x00010cc8
0x000214bc: __0dFCVoidG__vtbl       :	 0x00000000 0x00000000 0x00000000 0x00010ca8
(/ms/SUNWspro/bin/dbx) examine 0x10ca8
0x00010ca8: _dummy       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10b78
0x00010b78: ~CDer       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10d10
0x00010d10: a       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10cc8
0x00010cc8: b       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) examine 0x10d58
0x00010d58: c       :	 0x9de3bfa0
(/ms/SUNWspro/bin/dbx) 
*/

#endif

class IUnk
{
public:
    virtual void ia() = 0;
    virtual void ib() = 0;
};

class IPrivUnk
{
public:
    virtual void a() = 0;
    virtual void b() = 0;
};


class CBase : public CVoid, IPrivUnk
{
public:

typedef void (CBase::*PFNTEAROFF)(void);

static PFNTEAROFF s_tearoff[];

public:
    int _a;
    int _b;

    CBase() : _a(1), _b(2) {}
    virtual ~CBase() {}

    virtual void a(){ printf ("%d\n", _a); }
    virtual void b(){ printf ("%d\n", _b); }

    virtual void c() = 0;

private:
    virtual void _dummy(){}
};

class CDer : public CBase
{
public:
    int _c;

    CDer() : _c(3) {}
    virtual ~CDer() {}

    virtual void c(){ printf ("%d\n", _c); }
};

void done(CDer *pd, CVoid *pv, CVoid *pvv, IUnk* pi)
{
    pi->ia();
    ((CBase*)pv)->a();

    printf ("Done\n");
}

CBase::PFNTEAROFF CBase::s_tearoff[] = { (PFNTEAROFF) (void(CVoid::*)())a };

int main()
{
    CDer d;
    done(&d, (CVoid*)&d, (CVoid*)(void*)&d, (IUnk*)&d);
}

