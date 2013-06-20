
SHELL=/bin/sh

# srcdir = .
# VPATH = .

STL_INCL=-I${PWD}/../../stlport/

AUX_LIST=TestClass.o main.o nc_alloc.o random_number.o

TEST_LIST=test_algo.o  \
test_algobase.o     test_list.o test_slist.o \
test_bit_vector.o   test_vector.o \
test_deque_cray.o test_set.o test_map.o \
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

#CXXFLAGS = -hexceptions -DEH_DELETE_HAS_THROW_SPEC -I. ${STL_INCL} ${DEBUG_FLAGS}
CXXFLAGS = -D_STLP_HAS_NO_EXCEPTIONS -I. ${STL_INCL} ${DEBUG_FLAGS}

#LIBS = -L../../lib -lstlportx -lpthread
LIBS = -L../../lib -lstlport -lpthread

.SUFFIXES: .cpp .i .o .out

check: $(TEST)

$(TEST) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LIBS) $(OBJECTS) -o $(TEST_EXE)
	./$(TEST_EXE) -s 100

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -c -o $@

.cpp.i:
	$(CXX) $(CXXFLAGS) $< -E > $@

%.out: %.cpp
	$(CXX) $(CXXFLAGS) $*.cpp -c -USINGLE -DMAIN -g -o $*.o
	$(CXX) $(CXXFLAGS) $(LIBS) $*.o -o $*
	./$* -q
	-rm -f $*

clean:
	-rm -fr ${TEST_EXE} *.o *.ii *.out core
