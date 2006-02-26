
TARGET = rbuild.exe

all:	$(TARGET)


SOURCES = tools/rbuild/automaticdependency.cpp \
	tools/rbuild/bootstrap.cpp \
	tools/rbuild/cdfile.cpp \
	tools/rbuild/compilationunit.cpp \
	tools/rbuild/compilationunitsupportcode.cpp \
	tools/rbuild/compilerflag.cpp \
	tools/rbuild/configuration.cpp \
	tools/rbuild/define.cpp \
	tools/rbuild/directory.cpp \
	tools/rbuild/exception.cpp \
	tools/rbuild/filesupportcode.cpp \
	tools/rbuild/global.cpp \
	tools/rbuild/include.cpp \
	tools/rbuild/installfile.cpp \
	tools/rbuild/linkerflag.cpp \
	tools/rbuild/linkerscript.cpp \
	tools/rbuild/module.cpp \
	tools/rbuild/project.cpp \
	tools/rbuild/rbuild.cpp \
	tools/rbuild/stubbedcomponent.cpp \
	tools/rbuild/syssetupgenerator.cpp \
	tools/rbuild/testsupportcode.cpp \
	tools/rbuild/wineresource.cpp \
	tools/rbuild/backend/backend.cpp \
	tools/rbuild/backend/mingw/mingw.cpp \
	tools/rbuild/backend/mingw/proxymakefile.cpp \
	tools/rbuild/backend/mingw/modulehandler.cpp \
	tools/rbuild/backend/msvc/msvc.cpp \
	tools/rbuild/backend/msvc/msvcmaker.cpp \
	tools/rbuild/backend/msvc/vcprojmaker.cpp \
	tools/rbuild/backend/msvc/genguid.cpp \
	tools/rbuild/backend/devcpp/devcpp.cpp \
	tools/expat/xmlparse.c \
	tools/expat/xmlrole.c \
	tools/expat/xmltok.c \
	tools/xml.cpp \
	tools/ssprintf.cpp \
	tools/xmlstorage.cpp


INFLIB_BASE = lib/inflib
INFLIB_BASE_ = $(INFLIB_BASE)/

INFLIB_HOST_SOURCES = $(addprefix $(INFLIB_BASE_), \
	infcore.c \
	infget.c \
	infput.c \
	infhostgen.c \
	infhostget.c \
	infhostglue.c \
	infhostput.c \
)

INFLIB_HOST_OBJECTS = \
	$(subst $(INFLIB_BASE_), , $(INFLIB_HOST_SOURCES:.c=.o))

INFLIB_HOST_CFLAGS = -O3 -Wall -Wpointer-arith -Wconversion \
	-Wstrict-prototypes -Wmissing-prototypes -DINFLIB_HOST -D_M_IX86 \
	-I$(INFLIB_BASE) -Iinclude/reactos -DDBG \
	-Iinclude -Iinclude\ndk -Iinclude\ddk

$(INFLIB_HOST_OBJECTS):
	gcc $(INFLIB_HOST_CFLAGS) -c $(INFLIB_BASE_)$*.c -o $*.o


CFLAGS = -Itools -Itools/rbuild -Ilib/inflib \
	-DXMLNODE_LOCATION -DHAVE_EXPAT_CONFIG_H -DXMLCALL="" -DXMLIMPORT="" -D_ROS_ \
	-O0 -g

OBJECTS = \
	$(subst .c,.o,$(SOURCES:.cpp=.o)) \
	$(INFLIB_HOST_OBJECTS)


.c.o:
	gcc $(CFLAGS) -c $< -o $@

.cpp.o:
	g++ $(CFLAGS) -c $< -o $@


$(TARGET):	$(OBJECTS)
	g++ $(CFLAGS) $^ -o $@


clean:
	rm -f $(TARGET) $(OBJECTS)

