#include "fitz-base.h"

void fz_invert3x3(float *dst, float *m)
{
	float det;
	int i;

#define M3(m,i,j) (m)[3*i+j]
#define D2(a,b,c,d) (a * d - b * c)
#define D3(a1,a2,a3,b1,b2,b3,c1,c2,c3) \
	(a1 * D2(b2,b3,c2,c3)) - \
	(b1 * D2(a2,a3,c2,c3)) + \
	(c1 * D2(a2,a3,b2,b3))

	det = D3(M3(m,0,0), M3(m,1,0), M3(m,2,0),
			 M3(m,0,1), M3(m,1,1), M3(m,2,1),
			 M3(m,0,2), M3(m,1,2), M3(m,2,2));
	if (det == 0)
		det = 1.0;
	det = 1.0 / det;

	M3(dst,0,0) =  M3(m,1,1) * M3(m,2,2) - M3(m,1,2) * M3(m,2,1);
	M3(dst,0,1) = -M3(m,0,1) * M3(m,2,2) + M3(m,0,2) * M3(m,2,1);
	M3(dst,0,2) =  M3(m,0,1) * M3(m,1,2) - M3(m,0,2) * M3(m,1,1);

	M3(dst,1,0) = -M3(m,1,0) * M3(m,2,2) + M3(m,1,2) * M3(m,2,0);
	M3(dst,1,1) =  M3(m,0,0) * M3(m,2,2) - M3(m,0,2) * M3(m,2,0);
	M3(dst,1,2) = -M3(m,0,0) * M3(m,1,2) + M3(m,0,2) * M3(m,1,0);

	M3(dst,2,0) =  M3(m,1,0) * M3(m,2,1) - M3(m,1,1) * M3(m,2,0);
	M3(dst,2,1) = -M3(m,0,0) * M3(m,2,1) + M3(m,0,1) * M3(m,2,0);
	M3(dst,2,2) =  M3(m,0,0) * M3(m,1,1) - M3(m,0,1) * M3(m,1,0);

	for (i = 0; i < 9; i++)
		dst[i] *= det;
}

fz_matrix
fz_concat(fz_matrix one, fz_matrix two)
{
	fz_matrix dst;
	dst.a = one.a * two.a + one.b * two.c;
	dst.b = one.a * two.b + one.b * two.d;
	dst.c = one.c * two.a + one.d * two.c;
	dst.d = one.c * two.b + one.d * two.d;
	dst.e = one.e * two.a + one.f * two.c + two.e;
	dst.f = one.e * two.b + one.f * two.d + two.f;
	return dst;
}

fz_matrix
fz_identity(void)
{
	fz_matrix m;
	m.a =  1;  m.b =  0;
	m.c =  0;  m.d =  1;
	m.e =  0;  m.f =  0;
	return m;
}

fz_matrix
fz_scale(float sx, float sy)
{
	fz_matrix m;
	m.a = sx;  m.b =  0;
	m.c =  0;  m.d = sy;
	m.e =  0;  m.f =  0;
	return m;
}

fz_matrix
fz_rotate(float theta)
{
	fz_matrix m;
	float s = sin(theta * M_PI / 180.0);
	float c = cos(theta * M_PI / 180.0);
	m.a =  c;  m.b = s;
	m.c = -s;  m.d = c;
	m.e =  0;  m.f = 0;
	return m;
}

fz_matrix
fz_translate(float tx, float ty)
{
	fz_matrix m;
	m.a =  1;  m.b =  0;
	m.c =  0;  m.d =  1;
	m.e = tx;  m.f = ty;
	return m;
}

fz_matrix
fz_invertmatrix(fz_matrix src)
{
	fz_matrix dst;
	float rdet = 1.0 / (src.a * src.d - src.b * src.c);
	dst.a = src.d * rdet;
	dst.b = -src.b * rdet;
	dst.c = -src.c * rdet;
	dst.d = src.a * rdet;
	dst.e = -src.e * dst.a - src.f * dst.c;
	dst.f = -src.e * dst.b - src.f * dst.d;
	return dst;
}

int
fz_isrectilinear(fz_matrix m)
{
	return	(fabs(m.b) < FLT_EPSILON && fabs(m.c) < FLT_EPSILON) ||
			(fabs(m.a) < FLT_EPSILON && fabs(m.d) < FLT_EPSILON);
}

float
fz_matrixexpansion(fz_matrix m)
{
	return sqrt(fabs(m.a * m.d - m.b * m.c));
}

fz_point
fz_transformpoint(fz_matrix m, fz_point p)
{
	fz_point t;
	t.x = p.x * m.a + p.y * m.c + m.e;
	t.y = p.x * m.b + p.y * m.d + m.f;
	return t;
}

fz_rect
fz_transformaabb(fz_matrix m, fz_rect r)
{
	fz_point s, t, u, v;

	if (fz_isinfiniterect(r))
		return r;

	s.x = r.x0; s.y = r.y0;
	t.x = r.x0; t.y = r.y1;
	u.x = r.x1; u.y = r.y1;
	v.x = r.x1; v.y = r.y0;
	s = fz_transformpoint(m, s);
	t = fz_transformpoint(m, t);
	u = fz_transformpoint(m, u);
	v = fz_transformpoint(m, v);
	r.x0 = MIN4(s.x, t.x, u.x, v.x);
	r.y0 = MIN4(s.y, t.y, u.y, v.y);
	r.x1 = MAX4(s.x, t.x, u.x, v.x);
	r.y1 = MAX4(s.y, t.y, u.y, v.y);
	return r;
}

