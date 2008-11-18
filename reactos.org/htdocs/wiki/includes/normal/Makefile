.PHONY : all test testutf8 testclean icutest bench icubench clean distclean

## Latest greatest version of Unicode
## May cause confusion if running test suite from these files
## when the data was generated from a previous version.
#BASE=http://www.unicode.org/Public/UNIDATA

# Explicitly using Unicode 5.1
BASE=http://www.unicode.org/Public/5.1.0/ucd

# Can override to php-cli or php5 or whatevah
PHP=php
#PHP=php-cli

# Some nice tool to grab URLs with
FETCH=wget
#FETCH=fetch

all : UtfNormalData.inc Utf8Case.php

UtfNormalData.inc : UtfNormalGenerate.php UtfNormalUtil.php UnicodeData.txt CompositionExclusions.txt NormalizationCorrections.txt DerivedNormalizationProps.txt
	$(PHP) UtfNormalGenerate.php

Utf8Case.php : Utf8CaseGenerate.php UtfNormalUtil.php UnicodeData.txt
	$(PHP) Utf8CaseGenerate.php

test : testutf8 testclean UtfNormalTest.php UtfNormalData.inc NormalizationTest.txt
	$(PHP) UtfNormalTest.php

testutf8 : Utf8Test.php UTF-8-test.txt
	$(PHP) Utf8Test.php

testclean : CleanUpTest.php
	$(PHP) CleanUpTest.php

bench : UtfNormalData.inc testdata/washington.txt testdata/berlin.txt testdata/tokyo.txt testdata/young.txt testdata/bulgakov.txt
	$(PHP) UtfNormalBench.php

icutest : UtfNormalData.inc NormalizationTest.txt
	$(PHP) Utf8Test.php --icu
	$(PHP) CleanUpTest.php --icu
	$(PHP) UtfNormalTest.php --icu

icubench : UtfNormalData.inc testdata/washington.txt testdata/berlin.txt testdata/tokyo.txt testdata/young.txt testdata/bulgakov.txt
	$(PHP) UtfNormalBench.php --icu

clean :
	rm -f UtfNormalData.inc UtfNormalDataK.inc

distclean : clean
	rm -f CompositionExclusions.txt NormalizationTest.txt NormalizationCorrections.txt UnicodeData.txt DerivedNormalizationProps.txt UTF-8-test.txt

# The Unicode data files...
CompositionExclusions.txt :
	$(FETCH) $(BASE)/CompositionExclusions.txt

NormalizationTest.txt :
	$(FETCH) $(BASE)/NormalizationTest.txt

NormalizationCorrections.txt :
	$(FETCH) $(BASE)/NormalizationCorrections.txt

DerivedNormalizationProps.txt :
	$(FETCH) $(BASE)/DerivedNormalizationProps.txt

UnicodeData.txt :
	$(FETCH) $(BASE)/UnicodeData.txt

UTF-8-test.txt :
	$(FETCH) http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

testdata/berlin.txt :
	mkdir -p testdata && wget -U MediaWiki/test -O testdata/berlin.txt "http://de.wikipedia.org/w/index.php?title=Berlin&oldid=2775712&action=raw"

testdata/washington.txt :
	mkdir -p testdata && wget -U MediaWiki/test -O testdata/washington.txt "http://en.wikipedia.org/w/index.php?title=Washington%2C_D.C.&oldid=6370218&action=raw"

testdata/tokyo.txt :
	mkdir -p testdata && wget -U MediaWiki/test -O testdata/tokyo.txt "http://ja.wikipedia.org/w/index.php?title=%E6%9D%B1%E4%BA%AC%E9%83%BD&oldid=940880&action=raw"

testdata/young.txt :
	mkdir -p testdata && wget -U MediaWiki/test -O testdata/young.txt "http://ko.wikipedia.org/w/index.php?title=%EC%9D%B4%EC%88%98%EC%98%81&oldid=627688&action=raw"

testdata/bulgakov.txt :
	mkdir -p testdata && wget -U MediaWiki/test -O testdata/bulgakov.txt "http://ru.wikipedia.org/w/index.php?title=%D0%91%D1%83%D0%BB%D0%B3%D0%B0%D0%BA%D0%BE%D0%B2%2C_%D0%A1%D0%B5%D1%80%D0%B3%D0%B5%D0%B9_%D0%9D%D0%B8%D0%BA%D0%BE%D0%BB%D0%B0%D0%B5%D0%B2%D0%B8%D1%87&oldid=17704&action=raw"
