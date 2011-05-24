#include "locale_test.h"

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <locale>
#  include <sstream>
#  include <stdexcept>

#  include "complete_digits.h"

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

struct ref_locale {
  const char *name;
  const char *decimal_point;
  const char *thousands_sep;
};

static const ref_locale tested_locales[] = {
//{  name,         decimal_point, thousands_sepy_thousands_sep},
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  { "fr_FR",       ",",           "\xa0"},
  { "ru_RU.koi8r", ",",           "."},
  { "en_GB",       ".",           ","},
  { "en_US",       ".",           ","},
#  endif
  { "C",           ".",           ","},
};

//
// tests implementation
//
void LocaleTest::_num_put_get( const locale& loc, const ref_locale* prl ) {
  const ref_locale& rl = *prl;
  CPPUNIT_ASSERT( has_facet<numpunct<char> >(loc) );
  numpunct<char> const& npct = use_facet<numpunct<char> >(loc);
  CPPUNIT_ASSERT( npct.decimal_point() == *rl.decimal_point );

  float val = 1234.56f;
  ostringstream fostr;
  fostr.imbue(loc);
  fostr << val;

  string ref = "1";
  if (!npct.grouping().empty()) {
    ref += npct.thousands_sep();
  }
  ref += "234";
  ref += npct.decimal_point();
  ref += "56";
  //cout << "In " << loc.name() << " 1234.56 is written: " << fostr.str() << endl;
  CPPUNIT_ASSERT( fostr.str() == ref );

  val = 12345678.9f;
  ref = "1";
  ref += npct.decimal_point();
  ref += "23457e+";
  string digits = "7";
  complete_digits(digits);
  ref += digits;
  fostr.str("");
  fostr << val;
  CPPUNIT_ASSERT( fostr.str() == ref );

  val = 1000000000.0f;
  fostr.str("");
  fostr << val;
  digits = "9";
  complete_digits(digits);
  CPPUNIT_ASSERT( fostr.str() == string("1e+") + digits );

  val = 1234.0f;
  ref = "1";
  if (!npct.grouping().empty()) {
    ref += npct.thousands_sep();
  }
  ref += "234";
  fostr.str("");
  fostr << val;
  CPPUNIT_ASSERT( fostr.str() == ref );

  val = 10000001.0f;
  fostr.str("");
  fostr << val;
  digits = "7";
  complete_digits(digits);
  CPPUNIT_ASSERT( fostr.str() == string("1e+") + digits );

  if (npct.grouping().size() == 1 && npct.grouping()[0] == 3) {
    int ival = 1234567890;
    fostr.str("");
    fostr << ival;
    ref = "1";
    ref += npct.thousands_sep();
    ref += "234";
    ref += npct.thousands_sep();
    ref += "567";
    ref += npct.thousands_sep();
    ref += "890";
    CPPUNIT_ASSERT( fostr.str() == ref );
  }

#if defined (__BORLANDC__)
  num_put<char> const& nput = use_facet<num_put<char> >(loc);
  typedef numeric_limits<double> limd;
  fostr.setf(ios_base::uppercase | ios_base::showpos);

  if (limd::has_infinity) {
    double infinity = limd::infinity();
    fostr.str("");
    nput.put(fostr, fostr, ' ', infinity);
    CPPUNIT_ASSERT( fostr.str() == string("+Inf") );
  }

  if (limd::has_quiet_NaN) {
    /* Ignore FPU exceptions */
    unsigned int _float_control_word = _control87(0, 0);
    _control87(EM_INVALID|EM_INEXACT, MCW_EM);
    double qnan = limd::quiet_NaN();
    /* Reset floating point control word */
    _clear87();
    _control87(_float_control_word, MCW_EM);
    fostr.str("");
    nput.put(fostr, fostr, ' ', qnan);
    CPPUNIT_ASSERT( fostr.str() == string("+NaN") );
  }
#endif
}

typedef void (LocaleTest::*_Test) (const locale&, const ref_locale*);
static void test_supported_locale(LocaleTest& inst, _Test __test) {
  size_t n = sizeof(tested_locales) / sizeof(tested_locales[0]);
  for (size_t i = 0; i < n; ++i) {
    locale loc;
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    try
#  endif
    {
      locale tmp(tested_locales[i].name);
      loc = tmp;
    }
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    catch (runtime_error const&) {
      //This locale is not supported.
      continue;
    }
#  endif
    CPPUNIT_MESSAGE( loc.name().c_str() );
    (inst.*__test)(loc, tested_locales + i);

    {
      locale tmp(locale::classic(), tested_locales[i].name, locale::numeric);
      loc = tmp;
    }
    (inst.*__test)(loc, tested_locales + i);

    {
      locale tmp(locale::classic(), new numpunct_byname<char>(tested_locales[i].name));
      loc = tmp;
    }
    (inst.*__test)(loc, tested_locales + i);
  }
}

void LocaleTest::num_put_get()
{ test_supported_locale(*this, &LocaleTest::_num_put_get); }

void LocaleTest::numpunct_by_name()
{
  /*
   * Check of the 22.1.1.2.7 standard point. Construction of a locale
   * instance from a null pointer or an unknown name should result in
   * a runtime_error exception.
   */
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
#    if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new numpunct_byname<char>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#    endif

  try {
    locale loc(locale::classic(), new numpunct_byname<char>("yasli_language"));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    string veryLongFacetName("LC_NUMERIC=");
    veryLongFacetName.append(512, '?');
    locale loc(locale::classic(), new numpunct_byname<char>(veryLongFacetName.c_str()));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    locale loc(locale::classic(), "C", locale::numeric);
  }
  catch (runtime_error const& e) {
    CPPUNIT_MESSAGE( e.what() );
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    // On platform without real localization support we should rely on the "C" facet.
    locale loc(locale::classic(), "", locale::numeric);
  }
  catch (runtime_error const& e) {
    CPPUNIT_MESSAGE( e.what() );
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    locale loc(locale::classic(), new numpunct_byname<char>("C"));
    numpunct<char> const& cfacet_byname = use_facet<numpunct<char> >(loc);
    numpunct<char> const& cfacet = use_facet<numpunct<char> >(locale::classic());

    CPPUNIT_CHECK( cfacet_byname.decimal_point() == cfacet.decimal_point() );
    CPPUNIT_CHECK( cfacet_byname.grouping() == cfacet.grouping() );
    if (!cfacet.grouping().empty())
      CPPUNIT_CHECK( cfacet_byname.thousands_sep() == cfacet.thousands_sep() );
#    if !defined (STLPORT) || !defined (__GLIBC__)
    CPPUNIT_CHECK( cfacet_byname.truename() == cfacet.truename() );
    CPPUNIT_CHECK( cfacet_byname.falsename() == cfacet.falsename() );
#    endif
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    // On platform without real localization support we should rely on the "C" locale facet.
    locale loc(locale::classic(), new numpunct_byname<char>(""));
  }
  catch (runtime_error const& e) {
    CPPUNIT_MESSAGE( e.what() );
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#    if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
#      if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new numpunct_byname<wchar_t>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#      endif

  try {
    locale loc(locale::classic(), new numpunct_byname<wchar_t>("yasli_language"));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#    endif
#  endif
}

#endif
