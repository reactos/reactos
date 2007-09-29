noinst_LTLIBRARIES = libgoo.la

if ENABLE_XPDF_HEADERS

poppler_goo_includedir = $(includedir)/poppler/goo
poppler_goo_include_HEADERS =			\
	GooHash.h				\
	GooList.h				\
	GooTimer.h				\
	GooMutex.h				\
	GooString.h				\
	GooVector.h				\
	gmem.h					\
	gtypes.h				\
	gmem.h					\
	gfile.h					\
	FixedPoint.h

endif

libgoo_la_SOURCES =				\
	gfile.cc				\
	gmempp.cc				\
	GooHash.cc				\
	GooList.cc				\
	GooTimer.cc				\
	GooString.cc				\
	gmem.c					\
	FixedPoint.cc
