#ifndef _fib_h
#define _fib_h
class Fibonacci
{
  public:
    Fibonacci() : v1(0), v2(1) {}
    inline int operator()();
  private:
    int v1;
    int v2;
};

inline int
Fibonacci::operator()()
{
  int r = v1 + v2;
  v1 = v2;
  v2 = r;
  return v1;
}
#endif // _fib_h
