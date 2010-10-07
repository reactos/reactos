#include "locale_test.h"

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <locale>
#  include <sstream>
#  include <stdexcept>

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

struct ref_monetary {
  const char *name;
  const char *money_int_prefix;
  const char *money_int_prefix_old;
  const char *money_prefix;
  const char *money_suffix;
  const char *money_decimal_point;
  const char *money_thousands_sep;
};

static const ref_monetary tested_locales[] = {
//{  name,         money_int_prefix, money_int_prefix_old, money_prefix, money_suffix, money_decimal_point, money_thousands_sep},
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  { "fr_FR",       "EUR ",           "FRF ",               "",           "",           ",",
#    if defined (WIN32) || defined (_WIN32)
                                                                                                            "\xa0" },
#    else
                                                                                                            " " },
#    endif
  { "ru_RU.koi8r", "RUB ",           "RUR ",               "",           "\xd2\xd5\xc2", ".",               " " },
  { "en_GB",       "GBP ",           "",                   "\xa3",       "",           ".",                 "," },
  { "en_US",       "USD ",           "",                   "$",          "",           ".",                 "," },
#  endif
  { "C",           "",               "",                   "",           "",           " ",                 " " },
};


const ref_monetary* LocaleTest::_get_ref_monetary(size_t i)
{
  if (i < sizeof(tested_locales) / sizeof(tested_locales[0])) {
    return tested_locales + i;
  }
  return 0;
}

const char* LocaleTest::_get_ref_monetary_name(const ref_monetary* _ref)
{
  return _ref->name;
}

void LocaleTest::_money_put_get( const locale& loc, const ref_monetary* rl )
{
  _money_put_get2(loc, loc, rl);
}

void LocaleTest::_money_put_get2( const locale& loc, const locale& streamLoc, const ref_monetary* prl )
{
  const ref_monetary &rl = *prl;
  CPPUNIT_ASSERT( has_facet<money_put<char> >(loc) );
  money_put<char> const& fmp = use_facet<money_put<char> >(loc);
  CPPUNIT_ASSERT( has_facet<money_get<char> >(loc) );
  money_get<char> const& fmg = use_facet<money_get<char> >(loc);

  ostringstream ostr;
  ostr.imbue(streamLoc);
  ostr << showbase;

  //Check a positive value (international format)
  {
    string str_res;
    //money_put
    {
      CPPUNIT_ASSERT( (has_facet<moneypunct<char, true> >(loc)) );
      moneypunct<char, true> const& intl_fmp = use_facet<moneypunct<char, true> >(loc);

      ostreambuf_iterator<char, char_traits<char> > res = fmp.put(ostr, true, ostr, ' ', 123456);

      CPPUNIT_ASSERT( !res.failed() );
      str_res = ostr.str();
      //CPPUNIT_MESSAGE(str_res.c_str());

      size_t fieldIndex = 0;
      size_t index = 0;

      //On a positive value we skip the sign field if exists:
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        ++fieldIndex;
      }
      // international currency abbreviation, if it is before value

      /*
       * int_curr_symbol
       *
       *   The international currency symbol. The operand is a four-character
       *   string, with the first three characters containing the alphabetic
       *   international currency symbol in accordance with those specified
       *   in the ISO 4217 specification. The fourth character is the character used
       *   to separate the international currency symbol from the monetary quantity.
       *
       * (http://www.opengroup.org/onlinepubs/7990989775/xbd/locale.html)
       */
      string::size_type p = strlen( rl.money_int_prefix );
      if (p != 0) {
        CPPUNIT_ASSERT( intl_fmp.pos_format().field[fieldIndex] == money_base::symbol );
        string::size_type p_old = strlen( rl.money_int_prefix_old );
        CPPUNIT_ASSERT( (str_res.substr(index, p) == rl.money_int_prefix) ||
                        ((p_old != 0) &&
                         (str_res.substr(index, p_old) == rl.money_int_prefix_old)) );
        if ( str_res.substr(index, p) == rl.money_int_prefix ) {
          index += p;
        } else {
          index += p_old;
        }
        ++fieldIndex;
      }

      // space after currency
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::space ||
          intl_fmp.pos_format().field[fieldIndex] == money_base::none) {
        // iternational currency symobol has four chars, one of these chars
        // is separator, so if format has space on this place, it should
        // be skipped.
        ++fieldIndex;
      }

      // sign
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        ++fieldIndex;
      }

      // value
      CPPUNIT_ASSERT( str_res[index++] == '1' );
      if (!intl_fmp.grouping().empty()) {
        CPPUNIT_ASSERT( str_res[index++] == /* intl_fmp.thousands_sep() */ *rl.money_thousands_sep );
      }
      CPPUNIT_ASSERT( str_res[index++] == '2' );
      CPPUNIT_ASSERT( str_res[index++] == '3' );
      CPPUNIT_ASSERT( str_res[index++] == '4' );
      if (intl_fmp.frac_digits() != 0) {
        CPPUNIT_ASSERT( str_res[index++] == /* intl_fmp.decimal_point() */ *rl.money_decimal_point );
      }
      CPPUNIT_ASSERT( str_res[index++] == '5' );
      CPPUNIT_ASSERT( str_res[index++] == '6' );
      ++fieldIndex;

      // sign
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        ++fieldIndex;
      }

      // space
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::space ) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }

      // sign
      if (intl_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        ++fieldIndex;
      }

      //as space cannot be last the only left format can be none:
      while ( fieldIndex < 3 ) {
        CPPUNIT_ASSERT( intl_fmp.pos_format().field[fieldIndex] == money_base::none );
        ++fieldIndex;
      }
    }

    //money_get
    {
      ios_base::iostate err = ios_base::goodbit;
      string digits;

      istringstream istr(str_res);
      ostr.str( "" );
      ostr.clear();
      fmg.get(istr, istreambuf_iterator<char, char_traits<char> >(), true, ostr, err, digits);
      CPPUNIT_ASSERT( (err & (ios_base::failbit | ios_base::badbit)) == 0 );
      CPPUNIT_ASSERT( digits == "123456" );
    }
  }

  ostr.str("");
  //Check a negative value (national format)
  {
    CPPUNIT_ASSERT( (has_facet<moneypunct<char, false> >(loc)) );
    moneypunct<char, false> const& dom_fmp = use_facet<moneypunct<char, false> >(loc);
    string str_res;
    //Check money_put
    {
      ostreambuf_iterator<char, char_traits<char> > res = fmp.put(ostr, false, ostr, ' ', -123456);

      CPPUNIT_ASSERT( !res.failed() );
      str_res = ostr.str();
      //CPPUNIT_MESSAGE(str_res.c_str());

      size_t fieldIndex = 0;
      size_t index = 0;

      if (dom_fmp.neg_format().field[fieldIndex] == money_base::sign) {
        CPPUNIT_ASSERT( str_res.substr(index, dom_fmp.negative_sign().size()) == dom_fmp.negative_sign() );
        index += dom_fmp.negative_sign().size();
        ++fieldIndex;
      }

      string::size_type p = strlen( rl.money_prefix );
      if (p != 0) {
        CPPUNIT_ASSERT( str_res.substr(index, p) == rl.money_prefix );
        index += p;
        ++fieldIndex;
      }
      if (dom_fmp.neg_format().field[fieldIndex] == money_base::space ||
          dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }

      CPPUNIT_ASSERT( str_res[index++] == '1' );
      if (!dom_fmp.grouping().empty()) {
        CPPUNIT_ASSERT( str_res[index++] == dom_fmp.thousands_sep() );
      }
      CPPUNIT_ASSERT( str_res[index++] == '2' );
      CPPUNIT_ASSERT( str_res[index++] == '3' );
      CPPUNIT_ASSERT( str_res[index++] == '4' );
      if (dom_fmp.frac_digits() != 0) {
        CPPUNIT_ASSERT( str_res[index++] == dom_fmp.decimal_point() );
      }
      CPPUNIT_ASSERT( str_res[index++] == '5' );
      CPPUNIT_ASSERT( str_res[index++] == '6' );
      ++fieldIndex;

      //space cannot be last:
      if ((fieldIndex < 3) &&
          dom_fmp.neg_format().field[fieldIndex] == money_base::space) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }

      if (fieldIndex == 3) {
        //If none is last we should not add anything to the resulting string:
        if (dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
          CPPUNIT_ASSERT( index == str_res.size() );
        } else {
          CPPUNIT_ASSERT( dom_fmp.neg_format().field[fieldIndex] == money_base::symbol );
          CPPUNIT_ASSERT( str_res.substr(index, strlen(rl.money_suffix)) == rl.money_suffix );
        }
      }
    }

    //money_get
    {
      ios_base::iostate err = ios_base::goodbit;
#  if defined (STLPORT)
      _STLP_LONGEST_FLOAT_TYPE val;
#  else
      long double val;
#  endif

      istringstream istr(str_res);
      fmg.get(istr, istreambuf_iterator<char, char_traits<char> >(), false, ostr, err, val);
      CPPUNIT_ASSERT( (err & (ios_base::failbit | ios_base::badbit)) == 0 );
      if (dom_fmp.negative_sign().empty()) {
        //Without negative sign there is no way to guess the resulting amount sign ("C" locale):
        CPPUNIT_ASSERT( val == 123456 );
      }
      else {
        CPPUNIT_ASSERT( val == -123456 );
      }
    }
  }
}


// Test for bug in case when number of digits in value less then number
// of digits in fraction. I.e. '9' should be printed as '0.09',
// if x.frac_digits() == 2.

void LocaleTest::_money_put_X_bug( const locale& loc, const ref_monetary* prl )
{
  const ref_monetary &rl = *prl;
  CPPUNIT_ASSERT( has_facet<money_put<char> >(loc) );
  money_put<char> const& fmp = use_facet<money_put<char> >(loc);

  ostringstream ostr;
  ostr.imbue(loc);
  ostr << showbase;

  // ostr.str("");
  // Check value with one decimal digit:
  {
    CPPUNIT_ASSERT( (has_facet<moneypunct<char, false> >(loc)) );
    moneypunct<char, false> const& dom_fmp = use_facet<moneypunct<char, false> >(loc);
    string str_res;
    // Check money_put
    {
      ostreambuf_iterator<char, char_traits<char> > res = fmp.put(ostr, false, ostr, ' ', 9);

      CPPUNIT_ASSERT( !res.failed() );
      str_res = ostr.str();

      size_t fieldIndex = 0;
      size_t index = 0;

      if (dom_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        CPPUNIT_ASSERT( str_res.substr(index, dom_fmp.positive_sign().size()) == dom_fmp.positive_sign() );
        index += dom_fmp.positive_sign().size();
        ++fieldIndex;
      }

      string::size_type p = strlen( rl.money_prefix );
      if (p != 0) {
        CPPUNIT_ASSERT( str_res.substr(index, p) == rl.money_prefix );
        index += p;
        ++fieldIndex;
      }
      if (dom_fmp.neg_format().field[fieldIndex] == money_base::space ||
          dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }
      if (dom_fmp.frac_digits() != 0) {
        CPPUNIT_ASSERT( str_res[index++] == '0' );
        CPPUNIT_ASSERT( str_res[index++] == dom_fmp.decimal_point() );
        for ( int fd = 1; fd < dom_fmp.frac_digits(); ++fd ) {
          CPPUNIT_ASSERT( str_res[index++] == '0' );
        }
      }
      CPPUNIT_ASSERT( str_res[index++] == '9' );
      ++fieldIndex;

      //space cannot be last:
      if ((fieldIndex < 3) &&
          dom_fmp.neg_format().field[fieldIndex] == money_base::space) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }

      if (fieldIndex == 3) {
        //If none is last we should not add anything to the resulting string:
        if (dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
          CPPUNIT_ASSERT( index == str_res.size() );
        } else {
          CPPUNIT_ASSERT( dom_fmp.neg_format().field[fieldIndex] == money_base::symbol );
          CPPUNIT_ASSERT( str_res.substr(index, strlen(rl.money_suffix)) == rl.money_suffix );
        }
      }
    }
  }

  ostr.str("");
  // Check value with two decimal digit:
  {
    CPPUNIT_ASSERT( (has_facet<moneypunct<char, false> >(loc)) );
    moneypunct<char, false> const& dom_fmp = use_facet<moneypunct<char, false> >(loc);
    string str_res;
    // Check money_put
    {
      ostreambuf_iterator<char, char_traits<char> > res = fmp.put(ostr, false, ostr, ' ', 90);

      CPPUNIT_ASSERT( !res.failed() );
      str_res = ostr.str();

      size_t fieldIndex = 0;
      size_t index = 0;

      if (dom_fmp.pos_format().field[fieldIndex] == money_base::sign) {
        CPPUNIT_ASSERT( str_res.substr(index, dom_fmp.positive_sign().size()) == dom_fmp.positive_sign() );
        index += dom_fmp.positive_sign().size();
        ++fieldIndex;
      }

      string::size_type p = strlen( rl.money_prefix );
      if (p != 0) {
        CPPUNIT_ASSERT( str_res.substr(index, p) == rl.money_prefix );
        index += p;
        ++fieldIndex;
      }
      if (dom_fmp.neg_format().field[fieldIndex] == money_base::space ||
          dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }
      if (dom_fmp.frac_digits() != 0) {
        CPPUNIT_ASSERT( str_res[index++] == '0' );
        CPPUNIT_ASSERT( str_res[index++] == dom_fmp.decimal_point() );
        for ( int fd = 1; fd < dom_fmp.frac_digits() - 1; ++fd ) {
          CPPUNIT_ASSERT( str_res[index++] == '0' );
        }
      }
      CPPUNIT_ASSERT( str_res[index++] == '9' );
      if (dom_fmp.frac_digits() != 0) {
        CPPUNIT_ASSERT( str_res[index++] == '0' );
      }
      ++fieldIndex;

      //space cannot be last:
      if ((fieldIndex < 3) &&
          dom_fmp.neg_format().field[fieldIndex] == money_base::space) {
        CPPUNIT_ASSERT( str_res[index++] == ' ' );
        ++fieldIndex;
      }

      if (fieldIndex == 3) {
        //If none is last we should not add anything to the resulting string:
        if (dom_fmp.neg_format().field[fieldIndex] == money_base::none) {
          CPPUNIT_ASSERT( index == str_res.size() );
        } else {
          CPPUNIT_ASSERT( dom_fmp.neg_format().field[fieldIndex] == money_base::symbol );
          CPPUNIT_ASSERT( str_res.substr(index, strlen(rl.money_suffix)) == rl.money_suffix );
        }
      }
    }
  }
}

typedef void (LocaleTest::*_Test) (const locale&, const ref_monetary*);
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
      locale tmp(locale::classic(), tested_locales[i].name, locale::monetary);
      loc = tmp;
    }
    (inst.*__test)(loc, tested_locales + i);

    {
      locale tmp0(locale::classic(), new moneypunct_byname<char, true>(tested_locales[i].name));
      locale tmp1(tmp0, new moneypunct_byname<char, false>(tested_locales[i].name));
      loc = tmp1;
    }
    (inst.*__test)(loc, tested_locales + i);
  }
}

void LocaleTest::money_put_get()
{ test_supported_locale(*this, &LocaleTest::_money_put_get); }

void LocaleTest::money_put_X_bug()
{ test_supported_locale(*this, &LocaleTest::_money_put_X_bug); }

void LocaleTest::moneypunct_by_name()
{
  /*
   * Check of the 22.1.1.2.7 standard point. Construction of a locale
   * instance from a null pointer or an unknown name should result in
   * a runtime_error exception.
   */
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
#    if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new moneypunct_byname<char, true>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#    endif

  try {
    locale loc(locale::classic(), new moneypunct_byname<char, true>("yasli_language"));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    string veryLongFacetName("LC_MONETARY=");
    veryLongFacetName.append(512, '?');
    locale loc(locale::classic(), new moneypunct_byname<char, true>(veryLongFacetName.c_str()));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#    if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new moneypunct_byname<char, false>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#    endif

  try {
    locale loc(locale::classic(), new moneypunct_byname<char, false>("yasli_language"));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    string veryLongFacetName("LC_MONETARY=");
    veryLongFacetName.append(512, '?');
    locale loc(locale::classic(), new moneypunct_byname<char, false>(veryLongFacetName.c_str()));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const& /* e */) {
    //CPPUNIT_MESSAGE( e.what() );
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    locale loc(locale::classic(), new moneypunct_byname<char, false>("C"));
    moneypunct<char, false> const& cfacet_byname = use_facet<moneypunct<char, false> >(loc);
    moneypunct<char, false> const& cfacet = use_facet<moneypunct<char, false> >(locale::classic());

    money_base::pattern cp = cfacet.pos_format();
    money_base::pattern cp_bn = cfacet_byname.pos_format();
    CPPUNIT_CHECK( cp_bn.field[0] == cp.field[0] );
    CPPUNIT_CHECK( cp_bn.field[1] == cp.field[1] );
    CPPUNIT_CHECK( cp_bn.field[2] == cp.field[2] );
    CPPUNIT_CHECK( cp_bn.field[3] == cp.field[3] );

    CPPUNIT_CHECK( cfacet_byname.frac_digits() == cfacet.frac_digits() );
    if (cfacet_byname.frac_digits() != 0)
      CPPUNIT_CHECK( cfacet_byname.decimal_point() == cfacet.decimal_point() );
    CPPUNIT_CHECK( cfacet_byname.grouping() == cfacet.grouping() );
    if (!cfacet_byname.grouping().empty())
      CPPUNIT_CHECK( cfacet_byname.thousands_sep() == cfacet.thousands_sep() );
    CPPUNIT_CHECK( cfacet_byname.positive_sign() == cfacet.positive_sign() );
    CPPUNIT_CHECK( cfacet_byname.negative_sign() == cfacet.negative_sign() );
  }
  catch (runtime_error const& /* e */) {
    /* CPPUNIT_MESSAGE( e.what() ); */
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

  try {
    locale loc(locale::classic(), new moneypunct_byname<char, true>("C"));
    moneypunct<char, true> const& cfacet_byname = use_facet<moneypunct<char, true> >(loc);
    moneypunct<char, true> const& cfacet = use_facet<moneypunct<char, true> >(locale::classic());

    money_base::pattern cp = cfacet.pos_format();
    money_base::pattern cp_bn = cfacet_byname.pos_format();
    CPPUNIT_CHECK( cp_bn.field[0] == cp.field[0] );
    CPPUNIT_CHECK( cp_bn.field[1] == cp.field[1] );
    CPPUNIT_CHECK( cp_bn.field[2] == cp.field[2] );
    CPPUNIT_CHECK( cp_bn.field[3] == cp.field[3] );

    CPPUNIT_CHECK( cfacet_byname.frac_digits() == cfacet.frac_digits() );
    if (cfacet_byname.frac_digits() != 0)
      CPPUNIT_CHECK( cfacet_byname.decimal_point() == cfacet.decimal_point() );
    CPPUNIT_CHECK( cfacet_byname.grouping() == cfacet.grouping() );
    if (!cfacet_byname.grouping().empty())
      CPPUNIT_CHECK( cfacet_byname.thousands_sep() == cfacet.thousands_sep() );
    CPPUNIT_CHECK( cfacet_byname.positive_sign() == cfacet.positive_sign() );
    CPPUNIT_CHECK( cfacet_byname.negative_sign() == cfacet.negative_sign() );
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
    locale loc(locale::classic(), new moneypunct_byname<char, false>(""));
  }
  catch (runtime_error const& /* e */) {
    /* CPPUNIT_MESSAGE( e.what() ); */
    CPPUNIT_FAIL;
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#    if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
#      if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new moneypunct_byname<wchar_t, true>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#      endif

  try {
    locale loc(locale::classic(), new moneypunct_byname<wchar_t, true>("yasli_language"));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }

#      if defined (STLPORT) || !defined (__GNUC__)
  try {
    locale loc(locale::classic(), new moneypunct_byname<wchar_t, false>(static_cast<char const*>(0)));
    CPPUNIT_FAIL;
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_FAIL;
  }
#      endif

  try {
    locale loc(locale::classic(), new moneypunct_byname<wchar_t, false>("yasli_language"));
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
