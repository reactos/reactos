# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

# SHELL=/bin/sh
# srcdir = .
# VPATH = .


# point this to proper location
STL_INCL= -I../../stlport

AUX_LIST=TestClass.cpp main.cpp nc_alloc.cpp random_number.cpp

TEST_LIST=test_algo.cpp  \
test_algobase.cpp     test_list.cpp test_slist.cpp \
test_bit_vector.cpp   test_vector.cpp \
test_deque.cpp test_set.cpp test_map.cpp \
test_hash_map.cpp  test_hash_set.cpp test_rope.cpp \
test_string.cpp test_bitset.cpp test_valarray.cpp

OBJECTS = test_algo.o  \
test_algobase.o     test_list.o test_slist.o \
test_bit_vector.o   test_vector.o \
test_deque.o test_set.o test_map.o \
test_hash_map.o  test_hash_set.o test_rope.o \
test_string.o test_bitset.o test_valarray.o

LIST=${AUX_LIST} ${TEST_LIST}

# OBJECTS = $(LIST:%.cpp=%.o) $(STAT_MODULE)
EXECS = $(LIST:%.cpp=%)
TESTS = $(LIST:%.cpp=%.out)
TEST_EXE  = eh_test
TEST  = eh_test.out

CC = CC
CXX = $(CC)

CXXFLAGS = -w ${STL_INCL} -D_STLP_NO_CUSTOM_IO

LIBS = -lm 

LIBSTLPORT = -L../../lib -lstlport_hp

check: $(TEST)

all: $(TEST_EXE)
	echo done.

$(TEST_EXE) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) ${LIBSTLPORT} $(LIBS) -o $(TEST_EXE)

$(TEST) : $(TEST_EXE)
	$(TEST_EXE)

SUFFIXES: .cpp .o .i .s .out .res .y

.cpp.o :
	$(CXX) $(CXXFLAGS) $< -c -o $@

.cpp.i :
	$(CXX) $(CXXFLAGS) $< -E -H > $@

.cpp.out:
	$(CXX) $(CXXFLAGS) $< -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $*.o $(LIBS) -o $*
	./$* -q
	-rm -f $*

.cpp.s:
	$(CXX) $(CXXFLAGS) -O4 -S -pto $<  -o $@

clean:
	-rm -fr ${TEST_EXE} *.o *.rpo *.obj *.out core *~ Templates.DB SunWS_cache






