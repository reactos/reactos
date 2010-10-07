#ifndef ALIGNED_BUFFER_H
#define ALIGNED_BUFFER_H

_STLP_BEGIN_NAMESPACE
// this is for fake initialization
template<class T>
union _Stl_aligned_buffer {
  char buf[sizeof(T)];
  struct { double a; double b; } padding;

  T* operator&() {
    return __REINTERPRET_CAST(T*, this);
  }

  T const* operator&() const {
    return __REINTERPRET_CAST(T const*, this);
  }
};
_STLP_END_NAMESPACE

#endif
