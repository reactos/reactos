# -*- makefile -*- Time-stamp: <05/03/29 23:39:01 ptr>
# $Id$

# this is the version number of needed STLport library for linking
LIBMAJOR = 5
LIBMINOR = 2

PRGNAME = eh_test
SRC_CPP = ../../../test/eh/TestClass.cpp \
          ../../../test/eh/main.cpp \
          ../../../test/eh/nc_alloc.cpp \
          ../../../test/eh/random_number.cpp \
          ../../../test/eh/test_algo.cpp \
          ../../../test/eh/test_algobase.cpp \
          ../../../test/eh/test_list.cpp \
          ../../../test/eh/test_slist.cpp \
          ../../../test/eh/test_bit_vector.cpp \
          ../../../test/eh/test_vector.cpp \
          ../../../test/eh/test_deque.cpp \
          ../../../test/eh/test_set.cpp \
          ../../../test/eh/test_map.cpp \
          ../../../test/eh/test_hash_map.cpp \
          ../../../test/eh/test_hash_set.cpp \
          ../../../test/eh/test_rope.cpp \
          ../../../test/eh/test_string.cpp \
          ../../../test/eh/test_bitset.cpp \
          ../../../test/eh/test_valarray.cpp

