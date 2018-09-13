#include "core.hxx"
#pragma hdrstop

#include "htmlent.hxx"

struct _EntityInfo
{
    const TCHAR* _pszName;
    WCHAR        _nCode;
};

#define ENTITY(name, code) { _T(#name), code },

int MyStrCmpN(const WCHAR* pszStr, const WCHAR* pwc, ULONG len)
{
    // For some reason SHLWAPI StrCmpN DOESN'T WORK.
    // It returns a negative number for StrCmpN("image","AAA",3)
    // and a positive number for StrCmpN("aagrave", "AAA", 3) !!!

    // This also has the added advantage of not also having to call
    // StrLen to make sure the lengths are the same.

/********** modelled after strncmp.

The return value indicates the relation of the substrings of string1 and string2 as follows.

Return Value    Description
-----------------------------------------------------------------
< 0             string1 substring less than string2 substring
0               string1 substring identical to string2 substring
> 0             string1 substring greater than string2 substring
 
******/

    for (ULONG i = 0; i < len && *pszStr != NULL; i++)
    {
        if (*pszStr < *pwc)
        {
            return -1;
        }
        else if (*pszStr > *pwc)
        {
            return 1;
        }
        pszStr++;
        pwc++;
    }

    if (i < len)
    {
        return -1;
    }
    else if (*pszStr > 0)
    {
        return 1;// must be greater then
    }

    return 0;
}

/************
// These are the various built in entities that are supported when in HTML compatibility mode.
// I took these lists, combined them then pumped them through sort so that I can build a sorted
// static list and then use a binary search lookup.  This saves having to build a static
// Hashtable, and since it's for IE 4 compatibility mode, a binary search is good enough.

static _EntityInfo BuiltIn[] = {
    ENTITY(quot, 34)          // 
    ENTITY(amp, 38)           // & - ampersand
    ENTITY(apos, 39)          // ' - apostrophe //// not part of HTML!
    ENTITY(lt, 60)            // < less than
    ENTITY(gt, 62)            // > greater than
    { null, 0 }
};

static _EntityInfo BuiltInExt[] = {

    ENTITY(nbsp, 160)         // Non breaking space
    ENTITY(iexcl, 161)        //
    ENTITY(cent, 162)         // cent
    ENTITY(pound, 163)        // pound
    ENTITY(curren, 164)       // currency
    ENTITY(yen, 165)          // yen
    ENTITY(brvbar, 166)       // vertical bar
    ENTITY(sect, 167)         // section
    ENTITY(uml, 168)          //
    ENTITY(copy, 169)         // Copyright
    ENTITY(ordf, 170)         //
    ENTITY(laquo, 171)        //
    ENTITY(not, 172)          //
    ENTITY(shy, 173)          //
    ENTITY(reg, 174)          // Registered TradeMark
    ENTITY(macr, 175)         //
    ENTITY(deg, 176)          //
    ENTITY(plusmn, 177)       //
    ENTITY(sup2, 178)         //
    ENTITY(sup3, 179)         //
    ENTITY(acute, 180)        //
    ENTITY(micro, 181)        //
    ENTITY(para, 182)         //
    ENTITY(middot, 183)       //
    ENTITY(cedil, 184)        //
    ENTITY(sup1, 185)         //
    ENTITY(ordm, 186)         //
    ENTITY(raquo, 187)        //
    ENTITY(frac14, 188)       // 1/4
    ENTITY(frac12, 189)       // 1/2
    ENTITY(frac34, 190)       // 3/4
    ENTITY(iquest, 191)       // Inverse question mark
    ENTITY(Agrave, 192)       // Capital A grave accent
    ENTITY(Aacute, 193)       // Capital A acute accent
    ENTITY(Acirc, 194)        // Capital A circumflex accent
    ENTITY(Atilde, 195)       // Capital A tilde
    ENTITY(Auml, 196)         // Capital A dieresis or umlaut mark
    ENTITY(Aring, 197)        // Capital A ring
    ENTITY(AElig, 198)        // Capital AE dipthong (ligature)
    ENTITY(Ccedil, 199)       // Capital C cedilla
    ENTITY(Egrave, 200)       // Capital E grave accent
    ENTITY(Eacute, 201)       // Capital E acute accent
    ENTITY(Ecirc, 202)        // Capital E circumflex accent
    ENTITY(Euml, 203)         // Capital E dieresis or umlaut mark
    ENTITY(Igrave, 204)       // Capital I grave accent
    ENTITY(Iacute, 205)       // Capital I acute accent
    ENTITY(Icirc, 206)        // Capital I circumflex accent
    ENTITY(Iuml, 207)         // Capital I dieresis or umlaut mark
    ENTITY(ETH, 208)          // Capital Eth Icelandic
    ENTITY(Ntilde, 209)       // Capital N tilde
    ENTITY(Ograve, 210)       // Capital O grave accent
    ENTITY(Oacute, 211)       // Capital O acute accent
    ENTITY(Ocirc, 212)        // Capital O circumflex accent
    ENTITY(Otilde, 213)       // Capital O tilde
    ENTITY(Ouml, 214)         // Capital O dieresis or umlaut mark
    ENTITY(times, 215)        // multiply or times
    ENTITY(Oslash, 216)       // Capital O slash
    ENTITY(Ugrave, 217)       // Capital U grave accent
    ENTITY(Uacute, 218)       // Capital U acute accent
    ENTITY(Ucirc, 219)        // Capital U circumflex accent
    ENTITY(Uuml, 220)         // Capital U dieresis or umlaut mark;
    ENTITY(Yacute, 221)       // Capital Y acute accent
    ENTITY(THORN, 222)        // Capital THORN Icelandic
    ENTITY(szlig, 223)        // Small sharp s German (sz ligature)
    ENTITY(agrave, 224)       // Small a grave accent
    ENTITY(aacute, 225)       // Small a acute accent
    ENTITY(acirc, 226)        // Small a circumflex accent
    ENTITY(atilde, 227)       // Small a tilde
    ENTITY(auml, 228)         // Small a dieresis or umlaut mark
    ENTITY(aring, 229)        // Small a ring
    ENTITY(aelig, 230)        // Small ae dipthong (ligature)
    ENTITY(ccedil, 231)       // Small c cedilla
    ENTITY(egrave, 232)       // Small e grave accent
    ENTITY(eacute, 233)       // Small e acute accent
    ENTITY(ecirc, 234)        // Small e circumflex accent
    ENTITY(euml, 235)         // Small e dieresis or umlaut mark
    ENTITY(igrave, 236)       // Small i grave accent
    ENTITY(iacute, 237)       // Small i acute accent
    ENTITY(icirc, 238)        // Small i circumflex accent
    ENTITY(iuml, 239)         // Small i dieresis or umlaut mark
    ENTITY(eth, 240)          // Small eth Icelandic
    ENTITY(ntilde, 241)       // Small n tilde
    ENTITY(ograve, 242)       // Small o grave accent
    ENTITY(oacute, 243)       // Small o acute accent
    ENTITY(ocirc, 244)        // Small o circumflex accent
    ENTITY(otilde, 245)       // Small o tilde
    ENTITY(ouml, 246)         // Small o dieresis or umlaut mark
    ENTITY(divide, 247)       // divide
    ENTITY(oslash, 248)       // Small o slash
    ENTITY(ugrave, 249)       // Small u grave accent
    ENTITY(uacute, 250)       // Small u acute accent
    ENTITY(ucirc, 251)        // Small u circumflex accent
    ENTITY(uuml, 252)         // Small u dieresis or umlaut mark
    ENTITY(yacute, 253)       // Small y acute accent
    ENTITY(thorn, 254)        // Small thorn Icelandic
    ENTITY(yuml, 255)         // Small y dieresis or umlaut mark
    ENTITY(OElig, 338)        // latin capital ligature oe, U0152 ISOlat2
    ENTITY(oelig, 339)        // latin small ligature oe, U0153 ISOlat2
    ENTITY(Scaron, 352)       // latin capital letter s with caron, U0160 ISOlat2
    ENTITY(scaron, 353)       // latin small letter s with caron, U0161 ISOlat2
    ENTITY(Yuml, 376)         // latin capital letter y with diaeresis, U0178 ISOlat2
    ENTITY(fnof, 402)         // latin small f with hook, =function, =florin, U0192 ISOtech
    ENTITY(circ, 710)         // modifier letter circumflex accent, U02C6 ISOpub
    ENTITY(tilde, 732)        // small tilde, U02DC ISOdia
    ENTITY(Alpha, 913)        // greek capital letter alpha
    ENTITY(Beta, 914)         // greek capital letter beta
    ENTITY(Gamma, 915)        // greek capital letter gamma
    ENTITY(Delta, 916)        // greek capital letter delta
    ENTITY(Epsilon, 917)      // greek capital letter epsilon
    ENTITY(Zeta, 918)         // greek capital letter zeta
    ENTITY(Eta, 919)          // greek capital letter eta
    ENTITY(Theta, 920)        // greek capital letter theta
    ENTITY(Iota, 921)         // greek capital letter iota 
    ENTITY(Kappa, 922)        // greek capital letter kappa
    ENTITY(Lambda, 923)       // greek capital letter lambda
    ENTITY(Mu, 924)           // greek capital letter mu
    ENTITY(Nu, 925)           // greek capital letter nu
    ENTITY(Xi, 926)           // greek capital letter xi
    ENTITY(Omicron, 927)      // greek capital letter omicron
    ENTITY(Pi, 928)           // greek capital letter pi
    ENTITY(Rho, 929)          // greek capital letter rho
    ENTITY(Sigma, 931)        // greek capital letter sigma
    ENTITY(Tau, 932)          // greek capital letter tau
    ENTITY(Upsilon, 933)      // greek capital letter upsilon
    ENTITY(Phi, 934)          // greek capital letter phi
    ENTITY(Chi, 935)          // greek capital letter chi
    ENTITY(Psi, 936)          // greek capital letter psi   
    ENTITY(Omega, 937)        // greek capital letter omega
    ENTITY(alpha, 945)        // greek small letter alpha
    ENTITY(beta, 946)         // greek small letter beta
    ENTITY(gamma, 947)        // greek small letter gamma
    ENTITY(delta, 948)        // greek small letter delta
    ENTITY(epsilon, 949)      // greek small letter epsilon
    ENTITY(zeta, 950)         // greek small letter zeta
    ENTITY(eta, 951)          // greek small letter eta
    ENTITY(theta, 952)        // greek small letter theta
    ENTITY(iota, 953)         // greek small letter iota 
    ENTITY(kappa, 954)        // greek small letter kappa
    ENTITY(lambda, 955)       // greek small letter lambda
    ENTITY(mu, 956)           // greek small letter mu
    ENTITY(nu, 957)           // greek small letter nu
    ENTITY(xi, 958)           // greek small letter xi
    ENTITY(omicron, 959)      // greek small letter omicron
    ENTITY(pi, 960)           // greek small letter pi
    ENTITY(rho, 961)          // greek small letter rho
    ENTITY(sigmaf, 962)       // greek small final sigma
    ENTITY(sigma, 963)        // greek small letter sigma
    ENTITY(tau, 964)          // greek small letter tau
    ENTITY(upsilon, 965)      // greek small letter upsilon
    ENTITY(phi, 966)          // greek small letter phi
    ENTITY(chi, 967)          // greek small letter chi
    ENTITY(psi, 968)          // greek small letter psi   
    ENTITY(omega, 969)        // greek small letter omega
    ENTITY(thetasym, 977)     // greek small letter theta symbol, U03D1 NEW
    ENTITY(upsih, 978)        // greek upsilon with hook symbol
    ENTITY(piv, 982)          // greek pi symbol
    ENTITY(ensp, 8194)        // en space, U2002 ISOpub
    ENTITY(emsp, 8195)        // em space, U2003 ISOpub
    ENTITY(thinsp, 8201)      // thin space, U2009 ISOpub
    ENTITY(zwnj, 8204)        // zero width non-joiner, U200C NEW RFC 2070
    ENTITY(zwj, 8205)         // zero width joiner, U200D NEW RFC 2070
    ENTITY(lrm, 8206)         // left-to-right mark, U200E NEW RFC 2070
    ENTITY(rlm, 8207)         // right-to-left mark, U200F NEW RFC 2070
    ENTITY(ndash, 8211)       // en dash, U2013 ISOpub
    ENTITY(mdash, 8212)       // em dash, U2014 ISOpub
    ENTITY(lsquo, 8216)       // left single quotation mark, U2018 ISOnum
    ENTITY(rsquo, 8217)       // right single quotation mark, U2019 ISOnum
    ENTITY(sbquo, 8218)       // single low-9 quotation mark, U201A NEW
    ENTITY(ldquo, 8220)       // left double quotation mark, U201C ISOnum
    ENTITY(rdquo, 8221)       // right double quotation mark, U201D ISOnum
    ENTITY(bdquo, 8222)       // double low-9 quotation mark, U201E NEW
    ENTITY(dagger, 8224)      // dagger, U2020 ISOpub
    ENTITY(Dagger, 8225)      // double dagger, U2021 ISOpub
    ENTITY(bull, 8226)        // bullet, =black small circle, U2022 ISOpub
    ENTITY(hellip, 8230)      // horizontal ellipsis, =three dot leader, U2026 ISOpub
    ENTITY(permil, 8240)      // per mille sign, U2030 ISOtech
    ENTITY(prime, 8242)       // prime, =minutes, =feet, U2032 ISOtech
    ENTITY(Prime, 8243)       // double prime, =seconds, =inches, U2033 ISOtech
    ENTITY(lsaquo, 8249)      // single left-pointing angle quotation mark, U2039 ISO proposed
    ENTITY(rsaquo, 8250)      // single right-pointing angle quotation mark, U203A ISO proposed
    ENTITY(oline, 8254)       // overline, spacing overscore
    ENTITY(frasl, 8260)       // fraction slash
    ENTITY(image, 8465)       // blackletter capital I, =imaginary part, U2111 ISOamso 
    ENTITY(weierp, 8472)      // script capital P, =power set, =Weierstrass p, U2118 ISOamso 
    ENTITY(real, 8476)        // blackletter capital R, =real part symbol, U211C ISOamso 
    ENTITY(trade, 8482)       // trade mark sign, U2122 ISOnum 
    ENTITY(alefsym, 8501)     // alef symbol, =first transfinite cardinal, U2135 NEW 
    ENTITY(larr, 8592)        // leftwards arrow, U2190 ISOnum 
    ENTITY(uarr, 8593)        // upwards arrow, U2191 ISOnum
    ENTITY(rarr, 8594)        // rightwards arrow, U2192 ISOnum 
    ENTITY(darr, 8595)        // downwards arrow, U2193 ISOnum 
    ENTITY(harr, 8596)        // left right arrow, U2194 ISOamsa 
    ENTITY(crarr, 8629)       // downwards arrow with corner leftwards, =carriage return, U21B5 NEW 
    ENTITY(lArr, 8656)        // leftwards double arrow, U21D0 ISOtech 
    ENTITY(uArr, 8657)        // upwards double arrow, U21D1 ISOamsa 
    ENTITY(rArr, 8658)        // rightwards double arrow, U21D2 ISOtech 
    ENTITY(dArr, 8659)        // downwards double arrow, U21D3 ISOamsa 
    ENTITY(hArr, 8660)        // left right double arrow, U21D4 ISOamsa 
    ENTITY(forall, 8704)      // for all, U2200 ISOtech 
    ENTITY(part, 8706)        // partial differential, U2202 ISOtech  
    ENTITY(exist, 8707)       // there exists, U2203 ISOtech 
    ENTITY(empty, 8709)       // empty set, =null set, =diameter, U2205 ISOamso 
    ENTITY(nabla, 8711)       // nabla, =backward difference, U2207 ISOtech 
    ENTITY(isin, 8712)        // element of, U2208 ISOtech 
    ENTITY(notin, 8713)       // not an element of, U2209 ISOtech 
    ENTITY(ni, 8715)          // contains as member, U220B ISOtech 
    ENTITY(prod, 8719)        // n-ary product, =product sign, U220F ISOamsb 
    ENTITY(sum, 8721)         // n-ary sumation, U2211 ISOamsb 
    ENTITY(minus, 8722)       // minus sign, U2212 ISOtech 
    ENTITY(lowast, 8727)      // asterisk operator, U2217 ISOtech 
    ENTITY(radic, 8730)       // square root, =radical sign, U221A ISOtech 
    ENTITY(prop, 8733)        // proportional to, U221D ISOtech 
    ENTITY(infin, 8734)       // infinity, U221E ISOtech 
    ENTITY(ang, 8736)         // angle, U2220 ISOamso 
    ENTITY(and, 8743)         // logical and, =wedge, U2227 ISOtech 
    ENTITY(or, 8744)          // logical or, =vee, U2228 ISOtech 
    ENTITY(cap, 8745)         // intersection, =cap, U2229 ISOtech 
    ENTITY(cup, 8746)         // union, =cup, U222A ISOtech 
    ENTITY(int, 8747)         // integral, U222B ISOtech 
    ENTITY(there4, 8756)      // therefore, U2234 ISOtech 
    ENTITY(sim, 8764)         // tilde operator, =varies with, =similar to, U223C ISOtech 
    ENTITY(cong, 8773)        // approximately equal to, U2245 ISOtech 
    ENTITY(asymp, 8776)       // almost equal to, =asymptotic to, U2248 ISOamsr 
    ENTITY(ne, 8800)          // not equal to, U2260 ISOtech 
    ENTITY(equiv, 8801)       // identical to, U2261 ISOtech 
    ENTITY(le, 8804)          // less-than or equal to, U2264 ISOtech 
    ENTITY(ge, 8805)          // greater-than or equal to, U2265 ISOtech 
    ENTITY(sub, 8834)         // subset of, U2282 ISOtech 
    ENTITY(sup, 8835)         // superset of, U2283 ISOtech 
    ENTITY(nsub, 8836)        // not a subset of, U2284 ISOamsn 
    ENTITY(sube, 8838)        // subset of or equal to, U2286 ISOtech 
    ENTITY(supe, 8839)        // superset of or equal to, U2287 ISOtech 
    ENTITY(oplus, 8853)       // circled plus, =direct sum, U2295 ISOamsb 
    ENTITY(otimes, 8855)      // circled times, =vector product, U2297 ISOamsb 
    ENTITY(perp, 8869)        // up tack, =orthogonal to, =perpendicular, U22A5 ISOtech 
    ENTITY(sdot, 8901)        // dot operator, U22C5 ISOamsb 
    ENTITY(lceil, 8968)       // left ceiling, =apl upstile, U2308, ISOamsc  
    ENTITY(rceil, 8969)       // right ceiling, U2309, ISOamsc  
    ENTITY(lfloor, 8970)      // left floor, =apl downstile, U230A, ISOamsc  
    ENTITY(rfloor, 8971)      // right floor, U230B, ISOamsc  
    ENTITY(lang, 9001)        // left-pointing angle bracket, =bra, U2329 ISOtech 
    ENTITY(rang, 9002)        // right-pointing angle bracket, =ket, U232A ISOtech 
    ENTITY(loz, 9674)         // lozenge, U25CA ISOpub 
    ENTITY(spades, 9824)      // black spade suit, U2660 ISOpub 
    ENTITY(clubs, 9827)       // black club suit, =shamrock, U2663 ISOpub 
    ENTITY(hearts, 9829)      // black heart suit, =valentine, U2665 ISOpub 
    ENTITY(diams, 9830)       // black diamond suit, U2666 ISOpub 

    // These are Netscape-compatible synonyms for some above entities.

    ENTITY(QUOT, 34)          // For Netscape compatibility
    ENTITY(AMP, 38)           // For Netscape compatibility
    ENTITY(LT, 60)            // For Netscape compatibility
    ENTITY(GT, 62)            // For Netscape compatibility
    ENTITY(COPY, 169)         // For Netscape compatibility
    ENTITY(REG, 174)          // For Netscape compatibility
    ENTITY(TRADE, 8482)       // For IE3 compatibility

    // These are IE5 entities

    ENTITY(zwsp, 0203)    // zero width space             U200B NEW RFC 2070
    ENTITY(lre, 8234)    // Left-to-right embedding      U200F NEW RFC 2070
    ENTITY(rle, 8235)    // Right-to-left embedding      U200F NEW RFC 2070
    ENTITY(pdf, 8236)    // Pop direction format         U200F NEW RFC 2070
    ENTITY(lro, 8237)    // Left-to-right override       U200F NEW RFC 2070
    ENTITY(rlo, 8238)    // Right-to-left override       U200F NEW RFC 2070
    ENTITY(iss, 8298)    // Inhibit symmetric            U200F NEW RFC 2070 swapping
    ENTITY(ass, 8299)    // Activate symmetric           U200F NEW RFC 2070 swapping
    ENTITY(iafs, 8300)    // Inhibit Arabic form          U200F NEW RFC 2070 shaping
    ENTITY(aafs, 8301)    // Activate Arabic form         U200F NEW RFC 2070 shaping
    ENTITY(nads, 8302)    // National digit shapes        U200F NEW RFC 2070
    ENTITY(nods, 8303)    // Nominal digit shapes         U200F NEW RFC 2070

    {null, 0}
};

*************/

static const _EntityInfo s_BuiltInEntites[] = 
{
    ENTITY(AElig, 198)        // Capital AE dipthong (ligature)
    ENTITY(AMP, 38)           // For Netscape compatibility
    ENTITY(Aacute, 193)       // Capital A acute accent
    ENTITY(Acirc, 194)        // Capital A circumflex accent
    ENTITY(Agrave, 192)       // Capital A grave accent
    ENTITY(Alpha, 913)        // greek capital letter alpha
    ENTITY(Aring, 197)        // Capital A ring
    ENTITY(Atilde, 195)       // Capital A tilde
    ENTITY(Auml, 196)         // Capital A dieresis or umlaut mark
    ENTITY(Beta, 914)         // greek capital letter beta
    ENTITY(COPY, 169)         // For Netscape compatibility
    ENTITY(Ccedil, 199)       // Capital C cedilla
    ENTITY(Chi, 935)          // greek capital letter chi
    ENTITY(Dagger, 8225)      // double dagger, U2021 ISOpub
    ENTITY(Delta, 916)        // greek capital letter delta
    ENTITY(ETH, 208)          // Capital Eth Icelandic
    ENTITY(Eacute, 201)       // Capital E acute accent
    ENTITY(Ecirc, 202)        // Capital E circumflex accent
    ENTITY(Egrave, 200)       // Capital E grave accent
    ENTITY(Epsilon, 917)      // greek capital letter epsilon
    ENTITY(Eta, 919)          // greek capital letter eta
    ENTITY(Euml, 203)         // Capital E dieresis or umlaut mark
    ENTITY(GT, 62)            // For Netscape compatibility
    ENTITY(Gamma, 915)        // greek capital letter gamma
    ENTITY(Iacute, 205)       // Capital I acute accent
    ENTITY(Icirc, 206)        // Capital I circumflex accent
    ENTITY(Igrave, 204)       // Capital I grave accent
    ENTITY(Iota, 921)         // greek capital letter iota 
    ENTITY(Iuml, 207)         // Capital I dieresis or umlaut mark
    ENTITY(Kappa, 922)        // greek capital letter kappa
    ENTITY(LT, 60)            // For Netscape compatibility
    ENTITY(Lambda, 923)       // greek capital letter lambda
    ENTITY(Mu, 924)           // greek capital letter mu
    ENTITY(Ntilde, 209)       // Capital N tilde
    ENTITY(Nu, 925)           // greek capital letter nu
    ENTITY(OElig, 338)        // latin capital ligature oe, U0152 ISOlat2
    ENTITY(Oacute, 211)       // Capital O acute accent
    ENTITY(Ocirc, 212)        // Capital O circumflex accent
    ENTITY(Ograve, 210)       // Capital O grave accent
    ENTITY(Omega, 937)        // greek capital letter omega
    ENTITY(Omicron, 927)      // greek capital letter omicron
    ENTITY(Oslash, 216)       // Capital O slash
    ENTITY(Otilde, 213)       // Capital O tilde
    ENTITY(Ouml, 214)         // Capital O dieresis or umlaut mark
    ENTITY(Phi, 934)          // greek capital letter phi
    ENTITY(Pi, 928)           // greek capital letter pi
    ENTITY(Prime, 8243)       // double prime, =seconds, =inches, U2033 ISOtech
    ENTITY(Psi, 936)          // greek capital letter psi   
    ENTITY(QUOT, 34)          // For Netscape compatibility
    ENTITY(REG, 174)          // For Netscape compatibility
    ENTITY(Rho, 929)          // greek capital letter rho
    ENTITY(Scaron, 352)       // latin capital letter s with caron, U0160 ISOlat2
    ENTITY(Sigma, 931)        // greek capital letter sigma
    ENTITY(THORN, 222)        // Capital THORN Icelandic
    ENTITY(TRADE, 8482)       // For IE3 compatibility
    ENTITY(Tau, 932)          // greek capital letter tau
    ENTITY(Theta, 920)        // greek capital letter theta
    ENTITY(Uacute, 218)       // Capital U acute accent
    ENTITY(Ucirc, 219)        // Capital U circumflex accent
    ENTITY(Ugrave, 217)       // Capital U grave accent
    ENTITY(Upsilon, 933)      // greek capital letter upsilon
    ENTITY(Uuml, 220)         // Capital U dieresis or umlaut mark;
    ENTITY(Xi, 926)           // greek capital letter xi
    ENTITY(Yacute, 221)       // Capital Y acute accent
    ENTITY(Yuml, 376)         // latin capital letter y with diaeresis, U0178 ISOlat2
    ENTITY(Zeta, 918)         // greek capital letter zeta
    ENTITY(aacute, 225)       // Small a acute accent
    ENTITY(aafs, 8301)    // Activate Arabic form         U200F NEW RFC 2070 shaping
    ENTITY(acirc, 226)        // Small a circumflex accent
    ENTITY(acute, 180)        //
    ENTITY(aelig, 230)        // Small ae dipthong (ligature)
    ENTITY(agrave, 224)       // Small a grave accent
    ENTITY(alefsym, 8501)     // alef symbol, =first transfinite cardinal, U2135 NEW 
    ENTITY(alpha, 945)        // greek small letter alpha
    ENTITY(amp, 38)           // & - ampersand
    ENTITY(and, 8743)         // logical and, =wedge, U2227 ISOtech 
    ENTITY(ang, 8736)         // angle, U2220 ISOamso 
    ENTITY(apos, 39)          // ' - apostrophe //// not part of HTML!
    ENTITY(aring, 229)        // Small a ring
    ENTITY(ass, 8299)    // Activate symmetric           U200F NEW RFC 2070 swapping
    ENTITY(asymp, 8776)       // almost equal to, =asymptotic to, U2248 ISOamsr 
    ENTITY(atilde, 227)       // Small a tilde
    ENTITY(auml, 228)         // Small a dieresis or umlaut mark
    ENTITY(bdquo, 8222)       // double low-9 quotation mark, U201E NEW
    ENTITY(beta, 946)         // greek small letter beta
    ENTITY(brvbar, 166)       // vertical bar
    ENTITY(bull, 8226)        // bullet, =black small circle, U2022 ISOpub
    ENTITY(cap, 8745)         // intersection, =cap, U2229 ISOtech 
    ENTITY(ccedil, 231)       // Small c cedilla
    ENTITY(cedil, 184)        //
    ENTITY(cent, 162)         // cent
    ENTITY(chi, 967)          // greek small letter chi
    ENTITY(circ, 710)         // modifier letter circumflex accent, U02C6 ISOpub
    ENTITY(clubs, 9827)       // black club suit, =shamrock, U2663 ISOpub 
    ENTITY(cong, 8773)        // approximately equal to, U2245 ISOtech 
    ENTITY(copy, 169)         // Copyright
    ENTITY(crarr, 8629)       // downwards arrow with corner leftwards, =carriage return, U21B5 NEW 
    ENTITY(cup, 8746)         // union, =cup, U222A ISOtech 
    ENTITY(curren, 164)       // currency
    ENTITY(dArr, 8659)        // downwards double arrow, U21D3 ISOamsa 
    ENTITY(dagger, 8224)      // dagger, U2020 ISOpub
    ENTITY(darr, 8595)        // downwards arrow, U2193 ISOnum 
    ENTITY(deg, 176)          //
    ENTITY(delta, 948)        // greek small letter delta
    ENTITY(diams, 9830)       // black diamond suit, U2666 ISOpub 
    ENTITY(divide, 247)       // divide
    ENTITY(eacute, 233)       // Small e acute accent
    ENTITY(ecirc, 234)        // Small e circumflex accent
    ENTITY(egrave, 232)       // Small e grave accent
    ENTITY(empty, 8709)       // empty set, =null set, =diameter, U2205 ISOamso 
    ENTITY(emsp, 8195)        // em space, U2003 ISOpub
    ENTITY(ensp, 8194)        // en space, U2002 ISOpub
    ENTITY(epsilon, 949)      // greek small letter epsilon
    ENTITY(equiv, 8801)       // identical to, U2261 ISOtech 
    ENTITY(eta, 951)          // greek small letter eta
    ENTITY(eth, 240)          // Small eth Icelandic
    ENTITY(euml, 235)         // Small e dieresis or umlaut mark
    ENTITY(exist, 8707)       // there exists, U2203 ISOtech 
    ENTITY(fnof, 402)         // latin small f with hook, =function, =florin, U0192 ISOtech
    ENTITY(forall, 8704)      // for all, U2200 ISOtech 
    ENTITY(frac12, 189)       // 1/2
    ENTITY(frac14, 188)       // 1/4
    ENTITY(frac34, 190)       // 3/4
    ENTITY(frasl, 8260)       // fraction slash
    ENTITY(gamma, 947)        // greek small letter gamma
    ENTITY(ge, 8805)          // greater-than or equal to, U2265 ISOtech 
    ENTITY(gt, 62)            // > greater than
    ENTITY(hArr, 8660)        // left right double arrow, U21D4 ISOamsa 
    ENTITY(harr, 8596)        // left right arrow, U2194 ISOamsa 
    ENTITY(hearts, 9829)      // black heart suit, =valentine, U2665 ISOpub 
    ENTITY(hellip, 8230)      // horizontal ellipsis, =three dot leader, U2026 ISOpub
    ENTITY(iacute, 237)       // Small i acute accent
    ENTITY(iafs, 8300)    // Inhibit Arabic form          U200F NEW RFC 2070 shaping
    ENTITY(icirc, 238)        // Small i circumflex accent
    ENTITY(iexcl, 161)        //
    ENTITY(igrave, 236)       // Small i grave accent
    ENTITY(image, 8465)       // blackletter capital I, =imaginary part, U2111 ISOamso 
    ENTITY(infin, 8734)       // infinity, U221E ISOtech 
    ENTITY(int, 8747)         // integral, U222B ISOtech 
    ENTITY(iota, 953)         // greek small letter iota 
    ENTITY(iquest, 191)       // Inverse question mark
    ENTITY(isin, 8712)        // element of, U2208 ISOtech 
    ENTITY(iss, 8298)    // Inhibit symmetric            U200F NEW RFC 2070 swapping
    ENTITY(iuml, 239)         // Small i dieresis or umlaut mark
    ENTITY(kappa, 954)        // greek small letter kappa
    ENTITY(lArr, 8656)        // leftwards double arrow, U21D0 ISOtech 
    ENTITY(lambda, 955)       // greek small letter lambda
    ENTITY(lang, 9001)        // left-pointing angle bracket, =bra, U2329 ISOtech 
    ENTITY(laquo, 171)        //
    ENTITY(larr, 8592)        // leftwards arrow, U2190 ISOnum 
    ENTITY(lceil, 8968)       // left ceiling, =apl upstile, U2308, ISOamsc  
    ENTITY(ldquo, 8220)       // left double quotation mark, U201C ISOnum
    ENTITY(le, 8804)          // less-than or equal to, U2264 ISOtech 
    ENTITY(lfloor, 8970)      // left floor, =apl downstile, U230A, ISOamsc  
    ENTITY(lowast, 8727)      // asterisk operator, U2217 ISOtech 
    ENTITY(loz, 9674)         // lozenge, U25CA ISOpub 
    ENTITY(lre, 8234)    // Left-to-right embedding      U200F NEW RFC 2070
    ENTITY(lrm, 8206)         // left-to-right mark, U200E NEW RFC 2070
    ENTITY(lro, 8237)    // Left-to-right override       U200F NEW RFC 2070
    ENTITY(lsaquo, 8249)      // single left-pointing angle quotation mark, U2039 ISO proposed
    ENTITY(lsquo, 8216)       // left single quotation mark, U2018 ISOnum
    ENTITY(lt, 60)            // < less than
    ENTITY(macr, 175)         //
    ENTITY(mdash, 8212)       // em dash, U2014 ISOpub
    ENTITY(micro, 181)        //
    ENTITY(middot, 183)       //
    ENTITY(minus, 8722)       // minus sign, U2212 ISOtech 
    ENTITY(mu, 956)           // greek small letter mu
    ENTITY(nabla, 8711)       // nabla, =backward difference, U2207 ISOtech 
    ENTITY(nads, 8302)    // National digit shapes        U200F NEW RFC 2070
    ENTITY(nbsp, 160)         // Non breaking space
    ENTITY(ndash, 8211)       // en dash, U2013 ISOpub
    ENTITY(ne, 8800)          // not equal to, U2260 ISOtech 
    ENTITY(ni, 8715)          // contains as member, U220B ISOtech 
    ENTITY(nods, 8303)    // Nominal digit shapes         U200F NEW RFC 2070
    ENTITY(not, 172)          //
    ENTITY(notin, 8713)       // not an element of, U2209 ISOtech 
    ENTITY(nsub, 8836)        // not a subset of, U2284 ISOamsn 
    ENTITY(ntilde, 241)       // Small n tilde
    ENTITY(nu, 957)           // greek small letter nu
    ENTITY(oacute, 243)       // Small o acute accent
    ENTITY(ocirc, 244)        // Small o circumflex accent
    ENTITY(oelig, 339)        // latin small ligature oe, U0153 ISOlat2
    ENTITY(ograve, 242)       // Small o grave accent
    ENTITY(oline, 8254)       // overline, spacing overscore
    ENTITY(omega, 969)        // greek small letter omega
    ENTITY(omicron, 959)      // greek small letter omicron
    ENTITY(oplus, 8853)       // circled plus, =direct sum, U2295 ISOamsb 
    ENTITY(or, 8744)          // logical or, =vee, U2228 ISOtech 
    ENTITY(ordf, 170)         //
    ENTITY(ordm, 186)         //
    ENTITY(oslash, 248)       // Small o slash
    ENTITY(otilde, 245)       // Small o tilde
    ENTITY(otimes, 8855)      // circled times, =vector product, U2297 ISOamsb 
    ENTITY(ouml, 246)         // Small o dieresis or umlaut mark
    ENTITY(para, 182)         //
    ENTITY(part, 8706)        // partial differential, U2202 ISOtech  
    ENTITY(pdf, 8236)    // Pop direction format         U200F NEW RFC 2070
    ENTITY(permil, 8240)      // per mille sign, U2030 ISOtech
    ENTITY(perp, 8869)        // up tack, =orthogonal to, =perpendicular, U22A5 ISOtech 
    ENTITY(phi, 966)          // greek small letter phi
    ENTITY(pi, 960)           // greek small letter pi
    ENTITY(piv, 982)          // greek pi symbol
    ENTITY(plusmn, 177)       //
    ENTITY(pound, 163)        // pound
    ENTITY(prime, 8242)       // prime, =minutes, =feet, U2032 ISOtech
    ENTITY(prod, 8719)        // n-ary product, =product sign, U220F ISOamsb 
    ENTITY(prop, 8733)        // proportional to, U221D ISOtech 
    ENTITY(psi, 968)          // greek small letter psi   
    ENTITY(quot, 34)            // built in
    ENTITY(rArr, 8658)        // rightwards double arrow, U21D2 ISOtech 
    ENTITY(radic, 8730)       // square root, =radical sign, U221A ISOtech 
    ENTITY(rang, 9002)        // right-pointing angle bracket, =ket, U232A ISOtech 
    ENTITY(raquo, 187)        //
    ENTITY(rarr, 8594)        // rightwards arrow, U2192 ISOnum 
    ENTITY(rceil, 8969)       // right ceiling, U2309, ISOamsc  
    ENTITY(rdquo, 8221)       // right double quotation mark, U201D ISOnum
    ENTITY(real, 8476)        // blackletter capital R, =real part symbol, U211C ISOamso 
    ENTITY(reg, 174)          // Registered TradeMark
    ENTITY(rfloor, 8971)      // right floor, U230B, ISOamsc  
    ENTITY(rho, 961)          // greek small letter rho
    ENTITY(rle, 8235)    // Right-to-left embedding      U200F NEW RFC 2070
    ENTITY(rlm, 8207)         // right-to-left mark, U200F NEW RFC 2070
    ENTITY(rlo, 8238)    // Right-to-left override       U200F NEW RFC 2070
    ENTITY(rsaquo, 8250)      // single right-pointing angle quotation mark, U203A ISO proposed
    ENTITY(rsquo, 8217)       // right single quotation mark, U2019 ISOnum
    ENTITY(sbquo, 8218)       // single low-9 quotation mark, U201A NEW
    ENTITY(scaron, 353)       // latin small letter s with caron, U0161 ISOlat2
    ENTITY(sdot, 8901)        // dot operator, U22C5 ISOamsb 
    ENTITY(sect, 167)         // section
    ENTITY(shy, 173)          //
    ENTITY(sigma, 963)        // greek small letter sigma
    ENTITY(sigmaf, 962)       // greek small final sigma
    ENTITY(sim, 8764)         // tilde operator, =varies with, =similar to, U223C ISOtech 
    ENTITY(spades, 9824)      // black spade suit, U2660 ISOpub 
    ENTITY(sub, 8834)         // subset of, U2282 ISOtech 
    ENTITY(sube, 8838)        // subset of or equal to, U2286 ISOtech 
    ENTITY(sum, 8721)         // n-ary sumation, U2211 ISOamsb 
    ENTITY(sup, 8835)         // superset of, U2283 ISOtech 
    ENTITY(sup1, 185)         //
    ENTITY(sup2, 178)         //
    ENTITY(sup3, 179)         //
    ENTITY(supe, 8839)        // superset of or equal to, U2287 ISOtech 
    ENTITY(szlig, 223)        // Small sharp s German (sz ligature)
    ENTITY(tau, 964)          // greek small letter tau
    ENTITY(there4, 8756)      // therefore, U2234 ISOtech 
    ENTITY(theta, 952)        // greek small letter theta
    ENTITY(thetasym, 977)     // greek small letter theta symbol, U03D1 NEW
    ENTITY(thinsp, 8201)      // thin space, U2009 ISOpub
    ENTITY(thorn, 254)        // Small thorn Icelandic
    ENTITY(tilde, 732)        // small tilde, U02DC ISOdia
    ENTITY(times, 215)        // multiply or times
    ENTITY(trade, 8482)       // trade mark sign, U2122 ISOnum 
    ENTITY(uArr, 8657)        // upwards double arrow, U21D1 ISOamsa 
    ENTITY(uacute, 250)       // Small u acute accent
    ENTITY(uarr, 8593)        // upwards arrow, U2191 ISOnum
    ENTITY(ucirc, 251)        // Small u circumflex accent
    ENTITY(ugrave, 249)       // Small u grave accent
    ENTITY(uml, 168)          //
    ENTITY(upsih, 978)        // greek upsilon with hook symbol
    ENTITY(upsilon, 965)      // greek small letter upsilon
    ENTITY(uuml, 252)         // Small u dieresis or umlaut mark
    ENTITY(weierp, 8472)      // script capital P, =power set, =Weierstrass p, U2118 ISOamso 
    ENTITY(xi, 958)           // greek small letter xi
    ENTITY(yacute, 253)       // Small y acute accent
    ENTITY(yen, 165)          // yen
    ENTITY(yuml, 255)         // Small y dieresis or umlaut mark
    ENTITY(zeta, 950)         // greek small letter zeta
    ENTITY(zwj, 8205)         // zero width joiner, U200D NEW RFC 2070
    ENTITY(zwnj, 8204)        // zero width non-joiner, U200C NEW RFC 2070
    ENTITY(zwsp, 0203)    // zero width space             U200B NEW RFC 2070
};

// This implements a binary search lookup on the sorted entity table.
WCHAR LookupBuiltinEntity(const WCHAR* name, ULONG len)
{
    const _EntityInfo* table = s_BuiltInEntites;
    ULONG size = sizeof(s_BuiltInEntites) / sizeof(_EntityInfo);

    // initial range is the entire table - which we have to narrow down.
    ULONG top = 0;
    ULONG bottom = size;

    while (top < bottom)
    {
        ULONG i = top + (bottom - top) / 2;

        const _EntityInfo* pent = &table[i];
        const WCHAR* entname = pent->_pszName;

        // found a matnching character, so see if we're done.
        int rc = MyStrCmpN(entname, name, len);
        if (rc < 0)
        {
            if (top == i)
                top++;
            else
                top = i;
        }
        else if (rc > 0)
        {
            if (bottom == i)
                bottom--;
            else
                bottom = i;
        }
        else
        {
            return pent->_nCode;
        }
    }
    return 0xFFFF;
}





