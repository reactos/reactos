# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

# SHELL=/bin/sh
# srcdir = .
# VPATH = .

SHELL=/bin/sh

# point this to proper location
STL_INCL= -I../../stlport

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

CXXFLAGS = $(ARCHF) +w2 -mt -features=rtti ${STL_INCL}
# CXXFLAGS = +w2 ${STL_INCL}



LIBS = -lm 

LIBSTLPORT = -L../../lib -lstlport_sunpro42

check: $(TEST)

$(TEST) : $(OBJECTS)
	echo 'Info: For CC 4.x, warnings from ld in the form "symbol `XXX' has differing sizes" are normal.'
	$(CXX) $(CXXFLAGS) $(OBJECTS) ${LIBSTLPORT} $(LIBS) -o $(TEST_EXE)
	LD_LIBRARY_PATH="../../lib:$(LD_LIBRARY_PATH)" ./$(TEST_EXE) -s 100

SUFFIXES: .cpp.o.out.res

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

%.i : %.cpp
	$(CXX) $(CXXFLAGS) $< -E -H > $@

%.out: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $*.o $(LIBSTLPORT) $(LIBS) -o $*
	./$* -q
	-rm -f $*

%.s: %.cpp
	$(CXX) $(CXXFLAGS) -O4 -S -pto $<  -o $@

clean:
	-rm -fr ${TEST_EXE} *.o *.rpo *.obj *.out core *~ Templates.DB SunWS_cache






