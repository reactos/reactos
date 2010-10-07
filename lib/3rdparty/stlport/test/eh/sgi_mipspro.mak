# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

SHELL=/bin/sh

# srcdir = .
# VPATH = .


STL_INCL=-I${PWD}/../../stlport/

# STL_INCL= -DEH_NO_SGI_STL

AUX_LIST=TestClass.o main.o nc_alloc.o random_number.o

TEST_LIST=test_algo.o  \
test_algobase.o     test_list.o test_slist.o \
test_bit_vector.o   test_vector.o \
test_deque.o test_set.o test_map.o \
test_hash_map.o  test_hash_set.o test_rope.o \
test_string.o test_bitset.o test_valarray.o

LIST=${AUX_LIST} ${TEST_LIST}

OBJECTS = $(LIST)
EXECS = $(LIST:%.o=%)
TESTS = $(LIST:%.o=%.out)
TEST_EXE  = eh_test
TEST  = eh_test.out

CC = CC
CXX = $(CC)

# CXXFLAGS = -J 4 -ansi -LANG:std -I. ${STL_INCL} ${DEBUG_FLAGS} -I. -D_STLP_NO_OWN_IOSTREAMS -D_STLP_NO_NEW_IOSTREAMS
CXXFLAGS = -J 4 -ansi -LANG:std -I. ${STL_INCL} ${DEBUG_FLAGS} -I.

LIBS = -L../../lib -lstlport_mipspro -lm 
LIBSTDCXX = 

.SUFFIXES: .cpp .i .o .out .res

check: $(TEST)

$(TEST) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $(TEST_EXE)
	LD_LIBRARY_PATH="../../lib:$(LD_LIBRARY_PATH)" ./$(TEST_EXE) -s 100

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -c -o $@

.cpp.i:
	$(CXX) $(CXXFLAGS) $< -E > $@

%.out: %.cpp
	$(CXX) $(CXXFLAGS) $*.cpp -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $*.o $(LIBS) -o $*
	./$* -q
	-rm -f $*

clean:
	-rm -fr ${TEST_EXE} *.o *.rpo *.obj *.out core *~ Templates.DB SunWS_cache cxx_repository
