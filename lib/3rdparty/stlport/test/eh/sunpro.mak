# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.


SHELL=/bin/sh

# point this to proper location
STL_INCL= -I${PWD}/../../stlport

# STL_INCL= -DEH_NO_SGI_STL

AUX_LIST=TestClass.cpp main.cpp nc_alloc.cpp random_number.cpp

TEST_LIST=test_algo.cpp  \
test_algobase.cpp     test_list.cpp test_slist.cpp \
test_bit_vector.cpp   test_vector.cpp \
test_deque.cpp test_set.cpp test_map.cpp \
test_hash_map.cpp  test_hash_set.cpp test_rope.cpp \
test_string.cpp test_bitset.cpp test_valarray.cpp

LIST=${AUX_LIST} ${TEST_LIST}

OBJECTS = $(LIST:%.cpp=%.o) $(STAT_MODULE)
EXECS = $(LIST:%.cpp=%)
TESTS = $(LIST:%.cpp=%.out)
TEST_EXE  = eh_test
TEST  = eh_test.out

CC = CC
CXX = $(CC)

# CXXFLAGS = ${STL_INCL} -library=no%Cstd -qoption ccfe -instlib=../../lib/libstlport_sunpro.so -features=rtti -DEH_VECTOR_OPERATOR_NEW -DEH_DELETE_HAS_THROW_SPEC

CXXFLAGS = ${STL_INCL} -library=no%Cstd -features=rtti -DEH_VECTOR_OPERATOR_NEW -DEH_DELETE_HAS_THROW_SPEC

# This is to test with native STL
# CXXFLAGS = +w2 -xildoff -D_STLP_USE_NEWALLOC -DEH_NO_SGI_STL -DEH_NEW_HEADERS -DEH_VECTOR_OPERATOR_NEW -DEH_DELETE_HAS_THROW_SPEC


LIBS = -lm 
LIBSTDCXX = 

LIBSTLPORT = -library=no%Cstd -L../../lib -lstlport_sunpro


check: $(TEST)

$(TEST) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) ${LIBSTLPORT} $(LIBS) -o $(TEST_EXE)
	LD_LIBRARY_PATH="../../lib;${LD_LIBRARY_PATH}" ./$(TEST_EXE) -s 100

SUFFIXES: .cpp.o.out.res

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

%.i : %.cpp
	$(CXX) $(CXXFLAGS) $< -E -H > $@

%.out: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $*.o $(LIBS) ${LIBSTLPORT} -o $*
	LD_LIBRARY_PATH="../../lib;${LD_LIBRARY_PATH}" ./$* 


%.s: %.cpp
	$(CXX) $(CXXFLAGS) -O4 -S -pto $<  -o $@

clean:
	-rm -fr ${TEST_EXE} *.o *.rpo *.obj *.out core *~ Templates.DB SunWS_cache
