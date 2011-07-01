/***********************************************************************************
  Main.cpp

 * Copyright (c) 1997
 * Mark of the Unicorn, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Mark of the Unicorn makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.

 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Moscow Center for SPARC Technology makes
no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.

***********************************************************************************/
#include "Prefix.h"
#include "Tests.h"

#if defined (EH_NEW_IOSTREAMS)
# include <iostream>
# else
# include <iostream.h>
#endif

#if defined(macintosh)&&(!defined(__MRC__) && !defined(__SC__)) || defined (_MAC) && defined(__MWERKS__)

# include <console.h>
# include <Types.h>
# include <Strings.h>

# ifdef EH_NEW_HEADERS
#  include <cstdio>
#  include <cstring>
#  include <cassert>
# else
#  include <stdio.h>
#  include <string.h>
#  include <assert.h>
# endif

# if defined (_STL_DEBUG)

#  if defined ( EH_USE_SGI_STL )
// Override assertion behavior
#  include <cstdarg>
//#  include <stldebug.h>
void STLPORT::__stl_debug_message(const char * format_str, ...)
{
  std::va_list args;
  va_start( args, format_str );
  char msg[256];
  std::vsnprintf(msg, sizeof(msg)/sizeof(*msg) - 1, format_str, args );
  DebugStr( c2pstr(msg) );
}
#  else
/*===================================================================================
  __assertion_failed  (override standard library function)

  EFFECTS: Breaks into the debugger and shows the assertion. This implementation
           is Mac-specific; others could be added for other platforms.
====================================================================================*/
extern "C"
{
  void __assertion_failed(char *condition, char *testfilename, int lineno);
  void __assertion_failed(char *condition, char *testfilename, int lineno)
  {
      char msg[256];
      std::strncpy( msg, condition, 255 );
      std::strncat( msg, ": ", 255 );
      std::strncat(  msg, testfilename, 255 );
      std::strncat( msg, ", ", 255 );
      char line[20];
      std::sprintf( line, "%d", lineno );
      std::strncat(  msg, line, 255 );
      DebugStr( c2pstr( msg ) );
  }
}
#  endif

# endif

#endif

#include "nc_alloc.h"

#if defined (EH_NEW_HEADERS)
# include <vector>
# include <cstring>
# else
# include <vector.h>
# include <string.h>
#endif

#include "TestClass.h"
#include "LeakCheck.h"
#include "test_construct.h"
#ifdef __BORLANDC__
# include <except.h>
#endif

# if defined(EH_USE_NAMESPACES)
namespace  // dwa 1/21/00 - must use unnamed namespace here to avoid conflict under gcc using native streams
{
  using namespace std;
  // using std::cerr;
  // using std::endl;
}
# endif


/*===================================================================================
  usage  (file-static helper)

  EFFECTS: Prints a message describing the command-line parameters
====================================================================================*/
static void usage(const char* name)
{
    cerr<<"Usage : "<<name<<" [-n <iterations>] [-s <size>] [-l] [-e] [-q]/[-v] [-t] [test_name...]\n";
    cerr<<"\t[-n <iterations>] : number of test iterations, default==100;"<<endl;
    cerr<<"\t[-s <size>] : base value for random container sizes, default==1000;"<<endl;
    cerr<<"\t[-e] : don't throw exceptions, test for leak in normal conditions;"<<endl;
// This option was never actually used -- dwa 9/22/97
//    cerr<<"\t[-i] : ignore leak errors;"<<endl;
    cerr<<"\t[-q] : quiet mode;"<<endl;
    cerr<<"\t[-v] : verbose mode;"<<endl;
    cerr<<"\t[-t] : track each allocation;"<<endl;
    cerr<<"\t[test name [test name...]] : run only some of the tests by name (default==all tests):"<<endl;
    cerr<<"\t\tpossible test names are : algo vector bit_vector list slist deque set map hash_set hash_map rope string bitset valarray"<<endl;
    EH_CSTD::exit(1);
}

#ifdef EH_NEW_HEADERS
#  include <set>
#else
#  include <set.h>
#endif

#if defined(_WIN32_WCE)
#include <fstream>
#endif

int _STLP_CALL main(int argc, char** argv)
{
#if defined(_WIN32_WCE)
  std::ofstream file( "\\eh_test.txt" );
  std::streambuf* old_cout_buf = cout.rdbuf(file.rdbuf());
  std::streambuf* old_cerr_buf = cerr.rdbuf(file.rdbuf());
#endif
#if defined( __MWERKS__ ) && defined( macintosh )  // Get command line.
  argc = ccommand(&argv);
  // Allow the i/o window to be repositioned.
//  EH_STD::string s;
//  getline(EH_STD::cin, s);
#endif
    unsigned int niters=2;
    bool run_all=true;
    bool run_slist = false;
    bool run_list = false;
    bool run_vector = false;
    bool run_bit_vector = false;
    bool run_deque = false;
    bool run_hash_map = false;
    bool run_hash_set = false;
    bool run_set = false;
    bool run_map = false;
    bool run_algo = false;
    bool run_algobase = false;
    bool run_rope = false;
    bool run_string = false;
    bool run_bitset = false;
    bool run_valarray = false;

    int cur_argv;
    char *p, *p1;
#if defined (EH_NEW_IOSTREAMS)
    std::ios_base::sync_with_stdio(false);
#endif

    cerr << argv[0]<<" : Exception handling testsuite.\n";
    cerr.flush();

  bool track_allocations = false;
    // parse parameters :
    // leak_test [-iterations] [-test] ...
    for (cur_argv=1; cur_argv<argc; cur_argv++) {
        p = argv[cur_argv];
        if (*p == '-') {
            switch (p[1]) {
            case 'q':
                gTestController.SetVerbose(false);
                break;
            case 'v':
                gTestController.SetVerbose(true);
                break;
#if 0  // This option was never actually used -- dwa 9/22/97
            case 'i':
                gTestController.IgnoreLeaks(true);
                break;
#endif
            case 'n':
                p1 = argv[++cur_argv];
                if (p1 && EH_CSTD::sscanf(p1, "%i", &niters)==1)
                    cerr <<" Doing "<<niters<<" iterations\n";
                else
                    usage(argv[0]);
                break;
            case 't':
              track_allocations = true;
              break;
            case 'e':
                gTestController.TurnOffExceptions();
                break;
            case 's':
                p1 = argv[++cur_argv];
                if (p1 && EH_CSTD::sscanf(p1, "%i", &random_base)==1)
                    cerr <<" Setting  "<<random_base<<" as base for random sizes.\n";
                else
                    usage(argv[0]);
                break;
            default:
                usage(argv[0]);
                break;
            }
        } else {
            run_all = false;
            // test name
            if (EH_CSTD::strcmp(p, "algo")==0) {
                run_algo=true;
            } else if (EH_CSTD::strcmp(p, "vector")==0) {
                run_vector=true;
            } else if (EH_CSTD::strcmp(p, "bit_vector")==0) {
                run_bit_vector=true;
            } else if (EH_CSTD::strcmp(p, "list")==0) {
                run_list=true;
            } else if (EH_CSTD::strcmp(p, "slist")==0) {
                run_slist=true;
            } else if (EH_CSTD::strcmp(p, "deque")==0) {
                run_deque=true;
            } else if (EH_CSTD::strcmp(p, "set")==0) {
                run_set=true;
            } else if (EH_CSTD::strcmp(p, "map")==0) {
                run_map=true;
            } else if (EH_CSTD::strcmp(p, "hash_set")==0) {
                run_hash_set=true;
            } else if (EH_CSTD::strcmp(p, "hash_map")==0) {
                run_hash_map=true;
            } else if (EH_CSTD::strcmp(p, "rope")==0) {
                run_rope=true;
            } else if (EH_CSTD::strcmp(p, "string")==0) {
                run_string=true;
            } else if (EH_CSTD::strcmp(p, "bitset")==0) {
                run_bitset=true;
            } else if (EH_CSTD::strcmp(p, "valarray")==0) {
                run_valarray=true;
            } else {
                usage(argv[0]);
            }

        }
    }

  gTestController.TrackAllocations( track_allocations );

    // Over and over...
    for ( unsigned i = 0; i < niters ; i++ )
    {
     cerr << "iteration #" << i << "\n";
        if (run_all || run_algobase) {
            gTestController.SetCurrentContainer("algobase");
            cerr << "EH test : algobase" << endl;
            test_algobase();
        }
        if (run_all || run_algo) {
            gTestController.SetCurrentContainer("algo");
            cerr << "EH test : algo" << endl;
            test_algo();
        }

        if (run_all || run_vector) {
            gTestController.SetCurrentContainer("vector");
            cerr << "EH test : vector" << endl;
            test_vector();
        }

#if defined( EH_BIT_VECTOR_IMPLEMENTED )
        if (run_all || run_bit_vector) {
            gTestController.SetCurrentContainer("bit_vector");
           cerr << "EH test : bit_vector" << endl;
            test_bit_vector();
        }
#endif

        if (run_all || run_list) {
            gTestController.SetCurrentContainer("list");
            cerr << "EH test : list" << endl;
            test_list();
        }

#if defined( EH_SLIST_IMPLEMENTED )
        if (run_all || run_slist) {
            gTestController.SetCurrentContainer("slist");
            cerr << "EH test : slist" << endl;
            test_slist();
        }
#endif // EH_SLIST_IMPLEMENTED

        if (run_all || run_deque) {
            gTestController.SetCurrentContainer("deque");
            cerr << "EH test : deque" << endl;
            test_deque();
        }
        if (run_all || run_set) {
            gTestController.SetCurrentContainer("set");
            cerr << "EH test : set" << endl;
            test_set();
            gTestController.SetCurrentContainer("multiset");
            cerr << "EH test : multiset" << endl;
            test_multiset();
        }

        if (run_all || run_map) {
            gTestController.SetCurrentContainer("map");
            cerr << "EH test : map" << endl;
            test_map();
            gTestController.SetCurrentContainer("multimap");
            cerr << "EH test : multimap" << endl;
            test_multimap();
        }

#if defined( EH_HASHED_CONTAINERS_IMPLEMENTED )
        if (run_all || run_hash_map) {
            gTestController.SetCurrentContainer("hash_map");
            cerr << "EH test : hash_map" << endl;
            test_hash_map();
            gTestController.SetCurrentContainer("hash_multimap");
            cerr << "EH test : hash_multimap" << endl;
            test_hash_multimap();
        }

        if (run_all || run_hash_set) {
            gTestController.SetCurrentContainer("hash_set");
            cerr << "EH test : hash_set" << endl;
            test_hash_set();
            gTestController.SetCurrentContainer("hash_multiset");
            cerr << "EH test : hash_multiset" << endl;
            test_hash_multiset();
        }
#endif // EH_HASHED_CONTAINERS_IMPLEMENTED

#if defined( EH_ROPE_IMPLEMENTED )
  // CW1.8 can't compile this for some reason!
#if !( defined(__MWERKS__) && __MWERKS__ < 0x1900 )
        if (run_all || run_rope) {
            gTestController.SetCurrentContainer("rope");
            cerr << "EH test : rope" << endl;
            test_rope();
        }
#endif
#endif // EH_ROPE_IMPLEMENTED
#if defined( EH_STRING_IMPLEMENTED )
        if (run_all || run_string) {
            gTestController.SetCurrentContainer("string");
            cerr << "EH test : string" << endl;
            test_string();
        }
#endif
#if defined( EH_BITSET_IMPLEMENTED )
        if (run_all || run_bitset) {
            gTestController.SetCurrentContainer("bitset");
            cerr << "EH test : bitset" << endl;
            test_bitset();
        }
#endif
#if defined( EH_VALARRAY_IMPLEMENTED )
        if (run_all || run_bitset) {
            gTestController.SetCurrentContainer("valarray");
            cerr << "EH test : valarray" << endl;
            test_valarray();
        }
#endif
    }

  gTestController.TrackAllocations( false );

  cerr << "EH test : Done\n";

#if defined(_WIN32_WCE)
  cout.rdbuf(old_cout_buf);
  cerr.rdbuf(old_cerr_buf);
  file.close();
#endif

  return 0;
}

