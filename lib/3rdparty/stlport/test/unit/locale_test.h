#include <string>
#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <iosfwd>

#  include "cppunit/cppunit_proxy.h"
#  include <locale>

struct ref_monetary;
struct ref_locale;

#  if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
#    define STD std::
#  else
#    define STD
#  endif

//
// TestCase class
//
class LocaleTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(LocaleTest);
#  if defined (STLPORT) && !defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_IGNORE;
#  endif
  CPPUNIT_TEST(locale_by_name);
  CPPUNIT_TEST(moneypunct_by_name);
  CPPUNIT_TEST(time_by_name);
  CPPUNIT_TEST(numpunct_by_name);
  CPPUNIT_TEST(ctype_by_name);
  CPPUNIT_TEST(collate_by_name);
  CPPUNIT_TEST(messages_by_name);
  CPPUNIT_STOP_IGNORE;
  CPPUNIT_TEST(loc_has_facet);
  CPPUNIT_TEST(num_put_get);
  CPPUNIT_TEST(money_put_get);
  CPPUNIT_TEST(money_put_X_bug);
  CPPUNIT_TEST(time_put_get);
  CPPUNIT_TEST(collate_facet);
  CPPUNIT_TEST(ctype_facet);
#  if defined (STLPORT) && defined (_STLP_NO_MEMBER_TEMPLATES)
  CPPUNIT_IGNORE;
#  endif
  CPPUNIT_TEST(locale_init_problem);
  CPPUNIT_STOP_IGNORE;
  CPPUNIT_TEST(default_locale);
#  if !defined (STLPORT)
  CPPUNIT_IGNORE;
#  endif
  CPPUNIT_STOP_IGNORE;
#if (defined (STLPORT) && \
   (!defined (_STLP_USE_EXCEPTIONS) || defined (_STLP_NO_MEMBER_TEMPLATES) || defined (_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS)))
  CPPUNIT_IGNORE;
#  endif
  CPPUNIT_TEST(combine);
  CPPUNIT_TEST_SUITE_END();

public:
  void locale_by_name();
  void loc_has_facet();
  void num_put_get();
  void numpunct_by_name();
  void time_put_get();
  void time_by_name();
  void collate_facet();
  void collate_by_name();
  void ctype_facet();
  void ctype_by_name();
  void locale_init_problem();
  void money_put_get();
  void money_put_X_bug();
  void moneypunct_by_name();
  void default_locale();
  void combine();
  void messages_by_name();
private:
  void _loc_has_facet( const STD locale& );
  void _num_put_get( const STD locale&, const ref_locale* );
  void _time_put_get( const STD locale& );
  void _ctype_facet( const STD locale& );
  void _ctype_facet_w( const STD locale& );
  void _locale_init_problem( const STD locale& );

  static const ref_monetary* _get_ref_monetary(size_t);
  static const char* _get_ref_monetary_name(const ref_monetary*);

  void _money_put_get( const STD locale&, const ref_monetary* );
  void _money_put_get2( const STD locale& loc, const STD locale& streamLoc, const ref_monetary* );
  void _money_put_X_bug( const STD locale&, const ref_monetary* );
};

#  undef STD
#endif

