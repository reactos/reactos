#
# Note : this makefile has been tested for como-4.3.0.1+gcc-2.96 on Redhat 7.3
#

.SUFFIXES:
.SUFFIXES: .cc .cpp .o .exe .out

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

CC = como
CXX = $(CC) 

CXXFLAGS = -DLIBCIO= --diag_suppress=68 -D__null=0L -D__GNUG__ -D_STLP_DEBUG -I${STL_INCL}  -I. ${CXX_EXTRA_FLAGS}

LIBS = -L../../lib -lstlport_como_stldebug -lpthread -lm 
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
	-rm -fr ${TEST_EXE} *.out *.o *.ii *.ti
