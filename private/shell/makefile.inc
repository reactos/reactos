#
#  Common makefile.inc shared across components in the shell project.
#
#  You may ask yourself: what's the difference b/t common.inc and 
#  makefile.inc??  Well, the diff is common.inc follows the semantics
#  of a 'sources' file -- that is, it never contains rules or dependencies,
#  only macro definitions.  It keeps it cleaner.
#  
#  The makefile.inc is always used to list any special dependencies 
#  and rules.
#


!ifndef SELFREGNAME
SELFREGNAME = selfreg.inf
!endif

#
# We need to delete -DNT from the command line definitions because that
# causes our "Shell.shell32.NT" strings to go south.
#
..\$(SELFREGNAME) : ..\selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln

$(SELFREGNAME) : ..\selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln


!ifdef BUILD_PRIVLIB

# The following is done so we can build a private import lib which
# contains all exports, and a public lib which is shipped with the
# SDK and contains only public exports.  The private import lib
# is a super-set of the public lib.
#
# Your sources file must define the following macros to use this:
#
#   TARGETNAME    = foo
#   DLLDEF        = foo.def             <--- shown here for context
#
#   BUILD_PRIVLIB = 1
#
#   PRIVDEF       = foop.def
#   PRIVDEFSRC    = foo.src
#   PRIVLIB       = foop.lib
#
# The foop.def will automatically be built from PRIVDEFSRC.  TARGETNAME
# is listed above simply for name context.
#

PRIV_LIB = $(TARGETPATHLIB)\*\$(PRIVLIB)

!if "$(TARGET_DIRECTORY)" == "i386"
PRIV_LIB = $(PRIV_LIB:*=i386)
!elseif "$(TARGET_DIRECTORY)" == "alpha"
PRIV_LIB = $(PRIV_LIB:*=alpha)
!elseif "$(TARGET_DIRECTORY)" == "axp64"
PRIV_LIB = $(PRIV_LIB:*=axp64)
!elseif "$(TARGET_DIRECTORY)" == "ppc"
PRIV_LIB = $(PRIV_LIB:*=ppc)
!elseif "$(TARGET_DIRECTORY)" == "ia64"
PRIV_LIB = $(PRIV_LIB:*=ia64)
!else
PRIV_LIB = $(PRIV_LIB:*=mips)
!endif

# Make the public import lib dependent on the private import lib

$(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(TARGETNAME).lib : $(PRIV_LIB)


# Rule for making the private import lib.  Makefile.def already has
# a rule to make the public import lib.

$(PRIV_LIB) $(PRIV_LIB:.lib=.exp): $(PRIVDEF) $(LIBRARY_OBJS)
   -lib -out:$(PRIV_LIB) @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(PRIVDEF)
$(LIBRARY_OBJS: =
)
<<NOKEEP

# Makefile.def already has a rule that builds a .def file from a .src
# file.  However, we need to add some exclusive defines which we don't
# want defined for anything else.

$(DLLDEF): $(PRIVDEFSRC)
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $** > $@

# In a def file, the word 'PRIVATE' is a keyword telling the linker
# to not include the export in the import library.  We don't want
# this to happen for the private import library.  However, we
# always want 'ULTRAPRIVATE' to map to 'PRIVATE' regardless of the
# import library.

$(PRIVDEF): $(PRIVDEFSRC)
    $(C_PREPROCESSOR_NAME) -DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $*.dfp
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $*.dfp > $@

!endif
