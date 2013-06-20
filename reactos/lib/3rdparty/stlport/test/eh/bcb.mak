# ---------------------------------------------------------------------------
BCC32=bcc32
CPP32=cpp32

!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif

# ---------------------------------------------------------------------------
# IDE SECTION
# ---------------------------------------------------------------------------
# The following section of the project makefile is managed by the BCB IDE.
# It is recommended to use the IDE to change any of the values in this
# section.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
PROJECT = eh_test.exe
OBJFILES = TestClass.obj \
  nc_alloc.obj \
  random_number.obj \
  test_algo.obj \
  test_algobase.obj \
  test_bit_vector.obj \
  test_bitset.obj \
  test_deque.obj \
  test_hash_map.obj \
  test_hash_set.obj \
  test_list.obj \
  test_map.obj \
  test_rope.obj \
  test_set.obj \
  test_slist.obj \
  test_string.obj \
  test_valarray.obj \
  test_vector.obj main.obj

# ---------------------------------------------------------------------------
PATHCPP = .;
PATHPAS = .;
PATHASM = .;
PATHRC = .;

# USERDEFINES = _STLP_NO_OWN_IOSTREAMS

USERDEFINES = _DEBUG

SYSDEFINES = _RTLDLL;NO_STRICT;USEPACKAGES
# SYSDEFINES = NO_STRICT;USEPACKAGES
 # ---------------------------------------------------------------------------
CFLAG1 = -w- -jb -j1  -I.;..\..\stlport;$(BCB)\include; -Od -v -N -x -xp -tWC -D$(SYSDEFINES);$(USERDEFINES)

LDFLAGS = -L..\..\lib;$(BCB)\..\lib cw32i.lib stlp.4.5.lib

.autodepend
# ---------------------------------------------------------------------------

all : $(PROJECT)
        cd ..\..\lib
	..\test\eh\eh_test.exe -s 100

$(PROJECT) : $(OBJFILES)
	$(BCC32) -e$(PROJECT) $(CFLAG1) $(LDFLAGS) $(OBJFILES)

clean:
	del *.obj *.exe *.core *.tds

# ---------------------------------------------------------------------------
.cpp.obj:
    $(BCC32) $(CFLAG1) -n$(@D) -c $<

.cpp.exe:
    $(BCC32) $(CFLAG1) $(LDFLAGS) -n$(@D) $<

.cpp.i:
    $(CPP32) $(CFLAG1) -n. -Sr -Ss -Sd {$< }
# ---------------------------------------------------------------------------

