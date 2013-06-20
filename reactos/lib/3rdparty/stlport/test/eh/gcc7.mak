# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

srcdir = .
VPATH = .

# point this to proper location
STL_INCL=-I../../stlport

AUX_LIST=TestClass.cpp main.cpp nc_alloc.cpp random_number.cpp

TEST_LIST=test_algo.cpp  \
test_algobase.cpp     test_list.cpp test_slist.cpp \
test_bit_vector.cpp   test_vector.cpp \
test_deque.cpp test_set.cpp test_map.cpp \
test_hash_map.cpp  test_hash_set.cpp test_rope.cpp \
test_string.cpp test_bitset.cpp test_valarray.cpp

LIST=${AUX_LIST} ${TEST_LIST}

OBJECTS = $(LIST:%.cpp=obj/%.o) $(STAT_MODULE)
D_OBJECTS = $(LIST:%.cpp=d_obj/%.o) $(STAT_MODULE)
NOSGI_OBJECTS = $(LIST:%.cpp=nosgi_obj/%.o) $(STAT_MODULE)

EXECS = $(LIST:%.cpp=%)
TESTS = $(LIST:%.cpp=%.out)
TEST_EXE  = ./eh_test
D_TEST_EXE = ./eh_test_d
NOSGI_TEST_EXE = ./eh_test_nosgi

TEST  = ./eh_test.out
D_TEST = ./eh_test_d.out
NOSGI_TEST = ./eh_test_nosgi.out

CC = c++
CXX = $(CC)

CXXFLAGS = -Wall -fhandle-exceptions -g ${STL_INCL} -I. ${CXX_EXTRA_FLAGS} -D_STLP_NO_OWN_IOSTREAMS -D_STLP_DEBUG_UNINITIALIZED

check: $(TEST)

LIBS = -lm 
D_LIBSTLPORT = -L../../lib -lstlport_gcc_debug
LIBSTLPORT = -L../../lib -lstlport_gcc

all: $(TEST_EXE) $(D_TEST_EXE) $(NOSGI_TEST_EXE)

check_nosgi: $(NOSGI_TEST)
check_d: $(D_TEST)


$(TEST_EXE) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $(TEST_EXE)


$(TEST) : $(TEST_EXE)
	$(TEST_EXE)

$(D_TEST) : $(D_TEST_EXE)
	$(D_TEST_EXE)

$(NOSGI_TEST) : $(NOSGI_TEST_EXE)
	$(NOSGI_TEST_EXE)

SUFFIXES: .cpp.o.exe.out.res

nosgi_obj/%.o : %.cpp
	$(CXX) $(NOSGI_CXXFLAGS) $< -c -o $@

d_obj/%.o : %.cpp
	$(CXX) $(D_CXXFLAGS) $< -c -o $@

obj/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

obj/%.i : %.cpp
	$(CXX) $(CXXFLAGS) $< -E -H > $@

%.out: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $*.o $(LIBS) -o $*
	./$* > $@
	-rm -f $*

%.s: %.cpp
	$(CXX) $(CXXFLAGS) -O4 -S -pto $<  -o $@

%.E: %.cpp
	$(CXX) $(CXXFLAGS) -E $<  -o $@

clean:
	-rm -fr ${TEST_EXE} *.o */*.o *.rpo *.obj *.out core *~ Templates.DB
