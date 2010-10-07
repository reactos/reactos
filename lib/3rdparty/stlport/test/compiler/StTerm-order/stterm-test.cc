/*
 * The conversation with Matti Rintala on STLport forum 2005-08-24:
 *
 * Do you mean ISO/IEC 14882 3.6.3 [basic.start.term]?
 *
 * Yes. "Destructors (12.4) for initialized objects of static storage duration
 * (declared at block scope or at namespace scope) are called as a result
 * of returning from main and as a result of calling exit (18.3). These objects
 * are destroyed in the reverse order of the completion of their constructor
 * or of the completion of their dynamic initialization."
 *
 * I found a confirmation on the web that gcc may not strictly conform
 * to this behaviour in certains cases unless -fuse-cxa-atexit is used.
 *
 * Test below give (without -fuse-cxa-atexit)

Init::Init()
Init::use_it
It ctor done    <-- 0
Init::use_it done
Init ctor done  <-- 1
Init2 ctor done <-- 2
It dtor done    <-- 0
Init2 dtor done <-- 2
Init dtor done  <-- 1


 * but should:

Init::Init()
Init::use_it
It ctor done    <-- 0
Init::use_it done
Init ctor done  <-- 1
Init2 ctor done <-- 2
Init2 dtor done <-- 2
Init dtor done  <-- 1
It dtor done    <-- 0


 */
#include <stdio.h>

using namespace std;

class Init
{
  public:
    Init();
    ~Init();

    static void use_it();
};

class Init2
{
  public:
    Init2();
    ~Init2();

};

static Init init;
static Init2 init2;

class It
{
  public:
    It();
    ~It();
};

Init::Init()
{
  printf( "Init::Init()\n" );
  use_it();
  printf( "Init ctor done\n" );
}

Init::~Init()
{
  printf( "Init dtor done\n" );
}

void Init::use_it()
{
  printf( "Init::use_it\n" );

  static It it;

  printf( "Init::use_it done\n" );
}

Init2::Init2()
{
  printf( "Init2 ctor done\n" );
}

Init2::~Init2()
{
  printf( "Init2 dtor done\n" );
}

It::It()
{
  printf( "It ctor done\n" );
}

It::~It()
{
  printf( "It dtor done\n" );
}

int main()
{
  return 0;
}
