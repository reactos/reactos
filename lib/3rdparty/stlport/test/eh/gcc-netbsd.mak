# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

srcdir = .
VPATH = .
SHELL=/bin/sh

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

# dwa 12/22/99 -- had to turn off -ansi flag so we could use SGI IOSTREAMS
# also, test_slist won't compile with -O3/-O2 when targeting PPC. It fails 
# in the assembler with 'invalid relocation type'
CXXFLAGS = -Wall -g -O ${STL_INCL} -I. ${CXX_EXTRA_FLAGS} -DEH_VECTOR_OPERATOR_NEW
D_CXXFLAGS = -Wall -g -O ${STL_INCL} -I. ${CXX_EXTRA_FLAGS} -DEH_VECTOR_OPERATOR_NEW -D_STLP_DEBUG -D_STLP_USE_STATIC_LIB
NOSGI_CXXFLAGS = -Wall -g -O2 ${STL_INCL} -I. ${CXX_EXTRA_FLAGS} -D_STLP_NO_OWN_IOSTREAMS -D_STLP_DEBUG_UNINITIALIZED -DEH_VECTOR_OPERATOR_NEW

check: $(TEST)

LIBS = -lm -lpthread
D_LIBSTLPORT = -L../../lib -R../../lib -lstlport_gcc_stldebug
LIBSTLPORT = -L../../lib -R../../lib -lstlport_gcc

all: $(TEST_EXE) $(D_TEST_EXE) $(NOSGI_TEST_EXE)

check_nosgi: $(NOSGI_TEST)
check_d: $(D_TEST)

OBJDIR=obj
D_OBJDIR=d_obj
NOSGI_OBJDIR=nosgi_obj

$(OBJDIR):
	mkdir obj
$(D_OBJDIR):
	mkdir d_obj
$(NOSGI_OBJDIR):
	mkdir nosgi_obj

$(TEST_EXE) : $(OBJDIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBSTLPORT) $(LIBS) -o $(TEST_EXE)

$(D_TEST_EXE) : $(D_OBJDIR) $(D_OBJECTS)
	$(CXX) $(D_CXXFLAGS) $(D_OBJECTS) $(D_LIBSTLPORT) $(LIBS) -o $(D_TEST_EXE)

$(NOSGI_TEST_EXE) : $(NOSGI_OBJDIR) $(NOSGI_OBJECTS)
	$(CXX) $(NOSGI_CXXFLAGS) $(NOSGI_OBJECTS) $(LIBS) -o $(NOSGI_TEST_EXE)


$(TEST) : $(TEST_EXE)
	LD_LIBRARY_PATH="../../lib:$(LD_LIBRARY_PATH)" ./$(TEST_EXE) -s 100

$(D_TEST) : $(D_TEST_EXE)
	LD_LIBRARY_PATH="../../lib:$(LD_LIBRARY_PATH)" ./$(D_TEST_EXE) -s 100


$(NOSGI_TEST) : $(NOSGI_TEST_EXE)
	$(NOSGI_TEST_EXE)

SUFFIXES: .cpp.o.exe.out.res

nosgi_obj/%.o : %.cpp
	$(CXX) $(NOSGI_CXXFLAGS) $< -c -o $@

d_obj/%.o : %.cpp
	$(CXX) $(D_CXXFLAGS) $< -c -o $@

obj/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

nosgi_obj/%.i : %.cpp
	$(CXX) $(NOSGI_CXXFLAGS) $< -E -H > $@

d_obj/%.i : %.cpp
	$(CXX) $(D_CXXFLAGS) $< -E -H > $@

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
