
#include <math.h>


inline _D3DVECTOR::_D3DVECTOR(D3DVALUE f)
{
  x = y = z = f;
}

inline
_D3DVECTOR::_D3DVECTOR(D3DVALUE _x, D3DVALUE _y, D3DVALUE _z)
{
  x = _x;
  y = _y;
  z = _z;
}

inline _D3DVECTOR::_D3DVECTOR(const D3DVALUE f[3])
{
  x = f[0];
  y = f[1];
  z = f[2];
}

inline const D3DVALUE& _D3DVECTOR::operator[](int i) const
{
  return (&x)[i];
}

inline D3DVALUE& _D3DVECTOR::operator[](int i)
{
  return (&x)[i];
}

inline _D3DVECTOR& _D3DVECTOR::operator += (const _D3DVECTOR& v)
{
  x += v.x;
  y += v.y;
  z += v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator -= (const _D3DVECTOR& v)
{
  x -= v.x;
  y -= v.y;
  z -= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator *= (const _D3DVECTOR& v)
{
  x *= v.x;
  y *= v.y;
  z *= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator /= (const _D3DVECTOR& v)
{
  x /= v.x;
  y /= v.y;
  z /= v.z;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator *= (D3DVALUE s)
{
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

inline _D3DVECTOR& _D3DVECTOR::operator /= (D3DVALUE s)
{
  x /= s;
  y /= s;
  z /= s;
   return *this;
}

inline _D3DVECTOR operator + (const _D3DVECTOR& v)
{
  return v;
}

inline _D3DVECTOR operator - (const _D3DVECTOR& v)
{
  return _D3DVECTOR(-v.x, -v.y, -v.z);
}

inline _D3DVECTOR operator + (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z);
}

inline _D3DVECTOR operator - (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z);
}

inline _D3DVECTOR operator * (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z);
}

inline _D3DVECTOR operator / (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR(v1.x/v2.x, v1.y/v2.y, v1.z/v2.z);
}

inline int operator < (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return v1[0] < v2[0] && v1[1] < v2[1] && v1[2] < v2[2];
}

inline int operator <= (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return v1[0] <= v2[0] && v1[1] <= v2[1] && v1[2] <= v2[2];
}

inline _D3DVECTOR operator * (const _D3DVECTOR& v, D3DVALUE s)
{
  return _D3DVECTOR(s*v.x, s*v.y, s*v.z);
}

inline _D3DVECTOR operator * (D3DVALUE s, const _D3DVECTOR& v)
{
  return _D3DVECTOR(s*v.x, s*v.y, s*v.z);
}

inline _D3DVECTOR operator / (const _D3DVECTOR& v, D3DVALUE s)
{
  return _D3DVECTOR(v.x/s, v.y/s, v.z/s);
}

inline int operator == (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return v1.x==v2.x && v1.y==v2.y && v1.z == v2.z;
}

inline D3DVALUE Magnitude (const _D3DVECTOR& v)
{
  return (D3DVALUE) sqrt(SquareMagnitude(v));
}

inline D3DVALUE SquareMagnitude (const _D3DVECTOR& v)
{
  return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline _D3DVECTOR Normalize (const _D3DVECTOR& v)
{
  return v / Magnitude(v);
}

inline D3DVALUE Min (const _D3DVECTOR& v)
{
  D3DVALUE ret = v.x;
  if (v.y < ret)
      ret = v.y;

  if (v.z < ret)
      ret = v.z;

  return ret;
}

inline D3DVALUE Max (const _D3DVECTOR& v)
{
  D3DVALUE ret = v.x;
  if (ret < v.y)
      ret = v.y;

  if (ret < v.z)
      ret = v.z;

  return ret;
}

inline _D3DVECTOR Minimize (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR( v1[0] < v2[0] ? v1[0] : v2[0],
                     v1[1] < v2[1] ? v1[1] : v2[1],
                     v1[2] < v2[2] ? v1[2] : v2[2]);
}

inline _D3DVECTOR Maximize (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return _D3DVECTOR( v1[0] > v2[0] ? v1[0] : v2[0],
                     v1[1] > v2[1] ? v1[1] : v2[1],
                     v1[2] > v2[2] ? v1[2] : v2[2]);
}

inline D3DVALUE DotProduct (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  return v1.x*v2.x + v1.y * v2.y + v1.z*v2.z;
}

inline _D3DVECTOR CrossProduct (const _D3DVECTOR& v1, const _D3DVECTOR& v2)
{
  _D3DVECTOR result;
  result[0] = v1[1] * v2[2] - v1[2] * v2[1];
  result[1] = v1[2] * v2[0] - v1[0] * v2[2];
  result[2] = v1[0] * v2[1] - v1[1] * v2[0];

  return result;
}

inline _D3DMATRIX operator* (const _D3DMATRIX& a, const _D3DMATRIX& b)
{
  _D3DMATRIX ret;
  for (int i=0; i<4; i++)
  {
    for (int j=0; j<4; j++)
    {
      ret(i, j) = 0.0f;
      for (int k=0; k<4; k++)
      {
        ret(i, j) += a(i, k) * b(k, j);
      }
    }
}
  return ret;
}

