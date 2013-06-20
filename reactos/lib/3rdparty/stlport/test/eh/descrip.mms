# ;;; -*- Mode:makefile;-*- 
# Generated manually for MMS

# point this to proper location
STL_INCL= /include="../../stlport"


# STL_INCL= -DEH_NO_SGI_STL

.SUFFIXES .obj .cpp

all : check

AUX_LIST=TestClass.obj,main.obj,nc_alloc.obj,random_number.obj

TEST_LIST=test_algo.obj,-
test_algobase.obj,test_list.obj,test_slist.obj,-
test_bit_vector.obj,test_vector.obj,-
test_deque.obj,test_set.obj,test_map.obj,-
test_hash_map.obj,test_hash_set.obj,test_rope.obj,-
test_string.obj,test_bitset.obj,test_valarray.obj

LIST=$(AUX_LIST),$(TEST_LIST)

OBJECTS = $(LIST)
EXECS = $(LIST:%.obj=%.exe)
TESTS = $(LIST:%.obj=%.out)
TEST_EXE  = eh_test.exe
TEST  = eh_test.out

CC = cxx
CXX = $(CC)
LINK = cxxlink

# -std strict_ansi_errors

CXXFLAGS = $(STL_INCL) /define=(__NO_USE_STD_IOSTREAM,EH_VECTOR_OPERATOR_NEW,EH_DELETE_HAS_THROW_SPEC)

# This is to test with native STL
# CXXFLAGS = +w2 -xildoff -D__STL_USE_NEWALLOC -DEH_NO_SGI_STL -DEH_NEW_HEADERS -DEH_VECTOR_OPERATOR_NEW -DEH_DELETE_HAS_THROW_SPEC


LIBS = 
LIBSTDCXX = 

check : $(TEST)

$(TEST) : $(OBJECTS)
	$(LINK)/exe=$(TEST_EXE) $(OBJECTS) $(LIBS)
	run $(TEST_EXE)

.cpp.obj :
	$(CXX) $(CXXFLAGS) /obj=$@ $< 

