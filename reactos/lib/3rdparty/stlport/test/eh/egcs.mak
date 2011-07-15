# ;;; -*- Mode:makefile;-*- 
# Generated automatically from Makefile.in by configure.
# This requires GNU make.

srcdir = .
VPATH = .

# point this to proper location
STL_INCL=../../stlport

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
TEST_EXE  = ./eh_test
TEST  = ./eh_test.out

CC = egcs-c++
CXX = $(CC)

# for egcs
REPO_FLAGS =

# CXXFLAGS = -g -Wall -I${STL_INCL}  -I. -D_STLP_USE_NEWALLOC -D_STLP_DEBUG_ALLOC ${REPO_FLAGS} -DEH_NEW_HEADERS 

# CXXFLAGS = -Wall -ansi -I${STL_INCL}  -I. -D_STLP_DEBUG ${REPO_FLAGS} ${CXX_EXTRA_FLAGS}
CXXFLAGS = -Wall -g  -D_STLP_USE_NEWALLOC -DNO_FAST_ALLOCATOR -ansi -I${STL_INCL}  -I. ${REPO_FLAGS} ${CXX_EXTRA_FLAGS} -DEH_VECTOR_OPERATOR_NEW -D_STLP_DEBUG -D_STLP_NO_DEBUG_EXCEPTIONS

# CXXFLAGS = -Wall -I${STL_INCL}  -I. -D_STLP_USE_NEWALLOC ${REPO_FLAGS} ${CXX_EXTRA_FLAGS}


LIBS = -lm 
LIBSTDCXX = 

check: $(TEST)

$(TEST_EXE) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $(TEST_EXE)


$(TEST) : $(TEST_EXE)
	$(TEST_EXE)

SUFFIXES: .cpp.o.exe.out.res

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

%.i : %.cpp
	$(CXX) $(CXXFLAGS) $< -E -H -o $@

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
	-rm -fr ${TEST_EXE} $(OBJDIR) $(D_OBJDIR) $(NOSGI_OBJDIR) *.o *.rpo *.obj *.out core *~ Templates.DB
