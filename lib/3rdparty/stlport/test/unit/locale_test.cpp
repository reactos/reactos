#include "locale_test.h"

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <sstream>
#  include <locale>
#  include <stdexcept>

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

static const char* tested_locales[] = {
//name,
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  "fr_FR",
  "ru_RU.koi8r",
  "en_GB",
  "en_US",
#  endif
  "",
  "C"
};

CPPUNIT_TEST_SUITE_REGISTRATION(LocaleTest);

//
// tests implementation
//
typedef void (LocaleTest::*_Test) (const locale&);
static void test_supported_locale(LocaleTest &inst, _Test __test) {
  size_t n = sizeof(tested_locales) / sizeof(tested_locales[0]);
  for (size_t i = 0; i < n; ++i) {
    locale loc;
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    try {
#  endif
      locale tmp(tested_locales[i]);
      loc = tmp;
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    }
    catch (runtime_error const&) {
      //This locale is not supported.
      continue;
    }
#  endif
    CPPUNIT_MESSAGE( loc.name().c_str() );
    (inst.*__test)(loc);
  }
}

void LocaleTest::locale_by_name() {
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  /*
   * Check of the 22.1.1.2.7 standard point. Construction of a locale
   * instance from a null pointer or an unknown name should result in
   * a runtime_error exception.
   */
  try {
    locale loc(static_cast<char const*>(0));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    locale loc("yasli_language");
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    string very_large_locale_name(1024, '?');
    locale loc(very_large_locale_name.c_str());
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#if defined (STLPORT) || !defined (_MSC_VER) || (_MSC_VER > 1400)
  try {
    string very_large_locale_name("LC_CTYPE=");
    very_large_locale_name.append(1024, '?');
    locale loc(very_large_locale_name.c_str());
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    string very_large_locale_name("LC_ALL=");
    very_large_locale_name.append(1024, '?');
    locale loc(very_large_locale_name.c_str());
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#endif

  try {
    locale loc("C");
  }
  catch (runtime_error const& /* e */) {
    /* CPPUNIT_MESSAGE( e.what() ); */
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    // On platform without real localization support we should rely on the "C" locale facet.
    locale loc("");
  }
  catch (runtime_error const& /* e */) {
    /* CPPUNIT_MESSAGE( e.what() ); */
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#  endif
}

void LocaleTest::loc_has_facet() {
  locale loc("C");
  typedef numpunct<char> implemented_facet;
  CPPUNIT_ASSERT( has_facet<implemented_facet>(loc) );
  /*
  typedef num_put<char, back_insert_iterator<string> > not_implemented_facet;
  CPPUNIT_ASSERT( !has_facet<not_implemented_facet>(loc) );
  */
}

void LocaleTest::locale_init_problem() {
#  if !defined (STLPORT) || !defined (_STLP_NO_MEMBER_TEMPLATES)
  test_supported_locale(*this, &LocaleTest::_locale_init_problem);
#  endif
}

/*
 * Creation of a locale instance imply initialization of some STLport internal
 * static objects first. We use a static instance of locale to check that this
 * initialization is done correctly.
 */
static locale global_loc;
static locale other_loc("");

#  if !defined (STLPORT) || !defined (_STLP_NO_MEMBER_TEMPLATES)
void LocaleTest::_locale_init_problem( const locale& loc)
{
#    if !defined (__APPLE__) && !defined (__FreeBSD__) || \
        !defined(__GNUC__) || ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__> 3)))
  typedef codecvt<char,char,mbstate_t> my_facet;
#    else
// std::mbstate_t required for gcc 3.3.2 on FreeBSD...
// I am not sure what key here---FreeBSD or 3.3.2...
//      - ptr 2005-04-04
  typedef codecvt<char,char,std::mbstate_t> my_facet;
#    endif

  locale loc_ref(global_loc);
  {
    locale gloc( loc_ref, new my_facet() );
    CPPUNIT_ASSERT( has_facet<my_facet>( gloc ) );
    //The following code is just here to try to confuse the reference counting underlying mecanism:
    locale::global( locale::classic() );
    locale::global( gloc );
  }

#      if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  try {
#      endif
    ostringstream os("test") ;
    locale loc2( loc, new my_facet() );
    CPPUNIT_ASSERT( has_facet<my_facet>( loc2 ) );
    os.imbue( loc2 );
#      if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  }
  catch ( runtime_error& ) {
    CPPUNIT_FAIL;
  }
  catch ( ... ) {
   CPPUNIT_FAIL;
  }
#      endif

#      if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  try {
#      endif
    ostringstream os2("test2");
#      if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  }
  catch ( runtime_error& ) {
    CPPUNIT_FAIL;
  }
  catch ( ... ) {
    CPPUNIT_FAIL;
  }
#  endif
}
#endif

void LocaleTest::default_locale()
{
  locale loc( "" );
}

class dummy_facet : public locale::facet {
public:
  static locale::id id;
};

locale::id dummy_facet::id;

void LocaleTest::combine()
{
#  if (!defined (STLPORT) || \
       (defined (_STLP_USE_EXCEPTIONS) && !defined (_STLP_NO_MEMBER_TEMPLATES) && !defined (_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS)))
  {
    try {
      locale loc("");
      if (!has_facet<messages<char> >(loc)) {
        loc.combine<messages<char> >(loc);
        CPPUNIT_FAIL;
      }
    }
    catch (const runtime_error & /* e */) {
      /* CPPUNIT_MESSAGE( e.what() ); */
    }

    try {
      locale loc;
      if (!has_facet<dummy_facet>(loc)) {
        loc.combine<dummy_facet>(loc);
        CPPUNIT_FAIL;
      }
    }
    catch (const runtime_error & /* e */) {
      /* CPPUNIT_MESSAGE( e.what() ); */
    }
  }

  locale loc1(locale::classic()), loc2;
  size_t loc1_index = 0;
  for (size_t i = 0; _get_ref_monetary(i) != 0; ++i) {
    try {
      {
        locale loc(_get_ref_monetary_name(_get_ref_monetary(i)));
        if (loc1 == locale::classic())
        {
          loc1 = loc;
          loc1_index = i;
          continue;
        }
        else
        {
          loc2 = loc;
        }
      }

      //We can start the test
      ostringstream ostr;
      ostr << "combining '" << loc2.name() << "' money facets with '" << loc1.name() << "'";
      CPPUNIT_MESSAGE( ostr.str().c_str() );

      //We are going to combine money facets as all formats are different.
      {
        //We check that resulting locale has correctly acquire loc2 facets.
        locale loc = loc1.combine<moneypunct<char, true> >(loc2);
        loc = loc.combine<moneypunct<char, false> >(loc2);
        loc = loc.combine<money_put<char> >(loc2);
        loc = loc.combine<money_get<char> >(loc2);

        //Check loc has the correct facets:
        _money_put_get2(loc2, loc, _get_ref_monetary(i));

        //Check loc1 has not been impacted:
        _money_put_get2(loc1, loc1, _get_ref_monetary(loc1_index));

        //Check loc2 has not been impacted:
        _money_put_get2(loc2, loc2, _get_ref_monetary(i));
      }
      {
        //We check that resulting locale has not wrongly acquire loc1 facets that hasn't been combine:
        locale loc = loc2.combine<numpunct<char> >(loc1);
        loc = loc.combine<time_put<char> >(loc1);
        loc = loc.combine<time_get<char> >(loc1);

        //Check loc has the correct facets:
        _money_put_get2(loc2, loc, _get_ref_monetary(i));

        //Check loc1 has not been impacted:
        _money_put_get2(loc1, loc1, _get_ref_monetary(loc1_index));

        //Check loc2 has not been impacted:
        _money_put_get2(loc2, loc2, _get_ref_monetary(i));
      }

      {
        // Check auto combination do not result in weird reference counting behavior 
        // (might generate a crash).
        loc1.combine<numpunct<char> >(loc1);
      }

      loc1 = loc2;
      loc1_index = i;
    }
    catch (runtime_error const&) {
      //This locale is not supported.
      continue;
    }
  }
#  endif
}

#endif
