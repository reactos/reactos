/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef GEOM_UTIL_H_
#define GEOM_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct RectI {
    int x, y;
    int dx, dy;
} RectI;

typedef struct RectD {
    double x,y;
    double dx,dy;
} RectD;

int    RectI_Intersect(RectI *r1, RectI *r2, RectI *rIntersectOut);
void   RectI_FromXY(RectI *rOut, int xs, int xe, int ys, int ye);
int    RectI_Inside(RectI *r, int x, int y);
void   RectD_FromXY(RectD *rOut, double xs, double xe,  double ys, double ye);
void   RectD_FromRectI(RectD *rOut, RectI *rIn);
void   RectI_FromRectD(RectI *rOut, RectD *rIn);
void   RectD_Copy(RectD *rOut, RectD *rIn);
void   u_RectI_Intersect(void);

#ifdef __cplusplus
}
#endif

/* allow using from both C and C++ code */
#ifdef __cplusplus
class PointD {
public:
    PointD() { x = 0; y = 0; }
    PointD(double _x, double _y) { x = _x; y = _y; }
    void set(double _x, double _y) { x = _x; y = _y; }
    double x;
    double y;
};

class SizeI {
public:
    SizeI(int _dx, int _dy) { dx = _dx; dy = _dy; }
    void set(int _dx, int _dy) { dx = _dx; dy = _dy; }
    int dx;
    int dy;
};

class SizeD {
public:
    SizeD(double dx, double dy) { m_dx = dx; m_dy = dy; }
    SizeD(int dx, int dy) { m_dx = (double)dx; m_dy = (double)dy; }
    SizeD(SizeI si) { m_dx = (double)si.dx; m_dy = (double)si.dy; }
    SizeD() { m_dx = 0; m_dy = 0; }
    int dxI() { return (int)m_dx; }
    int dyI() { return (int)m_dy; }
    double dx() { return m_dx; }
    double dy() { return m_dy; }
    void setDx(double dx) { m_dx = dx; }
    void setDy(double dy) { m_dy = dy; }
    SizeI size() { return SizeI((int)dx(), (int)dy()); } /* @note: int casts */
private:
    double m_dx;
    double m_dy;
};

#endif

#endif
