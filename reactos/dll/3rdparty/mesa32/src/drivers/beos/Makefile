# src/mesa/drivers/beos/Makefile

TOP = ../../../..
include $(TOP)/configs/current

include $(TOP)/src/mesa/sources

MESA_MODULES = $(TOP)/src/mesa/mesa.a
# To switch back to old Mesa GLU implementation (but why do
# you want this!), uncomment this next line:

# GLU_DIR	= $(TOP)/src/glu/mesa
ifeq ($(GLU_DIR),)
# By default, we build the SGI GLU implementation
	GLU_DIR = $(TOP)/src/glu/sgi
endif

ifeq ($(GLU_DIR), $(TOP)/src/glu/mesa)
	GLU_MODULES = \
		$(GLU_DIR)/glu.o \
		$(GLU_DIR)/mipmap.o \
		$(GLU_DIR)/nurbs.o \
		$(GLU_DIR)/nurbscrv.o \
		$(GLU_DIR)/nurbssrf.o \
		$(GLU_DIR)/nurbsutl.o \
		$(GLU_DIR)/polytest.o \
		$(GLU_DIR)/project.o \
		$(GLU_DIR)/quadric.o \
		$(GLU_DIR)/tess.o \
		$(GLU_DIR)/tesselat.o
else
	GLU_MODULES = \
		$(GLU_DIR)/libutil/error.o \
		$(GLU_DIR)/libutil/glue.o \
		$(GLU_DIR)/libutil/mipmap.o	\
		$(GLU_DIR)/libutil/project.o \
		$(GLU_DIR)/libutil/quad.o \
		$(GLU_DIR)/libutil/registry.o \
		$(GLU_DIR)/libtess/dict.o \
		$(GLU_DIR)/libtess/geom.o \
		$(GLU_DIR)/libtess/memalloc.o \
		$(GLU_DIR)/libtess/mesh.o \
		$(GLU_DIR)/libtess/normal.o	\
		$(GLU_DIR)/libtess/priorityq.o \
		$(GLU_DIR)/libtess/render.o	\
		$(GLU_DIR)/libtess/sweep.o \
		$(GLU_DIR)/libtess/tess.o \
		$(GLU_DIR)/libtess/tessmono.o \
		$(GLU_DIR)/libnurbs/interface/bezierEval.o		\
		$(GLU_DIR)/libnurbs/interface/bezierPatch.o		\
		$(GLU_DIR)/libnurbs/interface/bezierPatchMesh.o		\
		$(GLU_DIR)/libnurbs/interface/glcurveval.o		\
		$(GLU_DIR)/libnurbs/interface/glinterface.o		\
		$(GLU_DIR)/libnurbs/interface/glrenderer.o		\
		$(GLU_DIR)/libnurbs/interface/glsurfeval.o		\
		$(GLU_DIR)/libnurbs/interface/incurveeval.o		\
		$(GLU_DIR)/libnurbs/interface/insurfeval.o		\
		$(GLU_DIR)/libnurbs/internals/arc.o			\
		$(GLU_DIR)/libnurbs/internals/arcsorter.o			\
		$(GLU_DIR)/libnurbs/internals/arctess.o			\
		$(GLU_DIR)/libnurbs/internals/backend.o			\
		$(GLU_DIR)/libnurbs/internals/basiccrveval.o		\
		$(GLU_DIR)/libnurbs/internals/basicsurfeval.o		\
		$(GLU_DIR)/libnurbs/internals/bin.o			\
		$(GLU_DIR)/libnurbs/internals/bufpool.o			\
		$(GLU_DIR)/libnurbs/internals/cachingeval.o		\
		$(GLU_DIR)/libnurbs/internals/ccw.o			\
		$(GLU_DIR)/libnurbs/internals/coveandtiler.o		\
		$(GLU_DIR)/libnurbs/internals/curve.o			\
		$(GLU_DIR)/libnurbs/internals/curvelist.o			\
		$(GLU_DIR)/libnurbs/internals/curvesub.o			\
		$(GLU_DIR)/libnurbs/internals/dataTransform.o		\
		$(GLU_DIR)/libnurbs/internals/displaylist.o		\
		$(GLU_DIR)/libnurbs/internals/flist.o			\
		$(GLU_DIR)/libnurbs/internals/flistsorter.o		\
		$(GLU_DIR)/libnurbs/internals/hull.o			\
		$(GLU_DIR)/libnurbs/internals/intersect.o			\
		$(GLU_DIR)/libnurbs/internals/knotvector.o		\
		$(GLU_DIR)/libnurbs/internals/mapdesc.o			\
		$(GLU_DIR)/libnurbs/internals/mapdescv.o			\
		$(GLU_DIR)/libnurbs/internals/maplist.o			\
		$(GLU_DIR)/libnurbs/internals/mesher.o			\
		$(GLU_DIR)/libnurbs/internals/monoTriangulationBackend.o	\
		$(GLU_DIR)/libnurbs/internals/monotonizer.o		\
		$(GLU_DIR)/libnurbs/internals/mycode.o			\
		$(GLU_DIR)/libnurbs/internals/nurbsinterfac.o		\
		$(GLU_DIR)/libnurbs/internals/nurbstess.o			\
		$(GLU_DIR)/libnurbs/internals/patch.o			\
		$(GLU_DIR)/libnurbs/internals/patchlist.o			\
		$(GLU_DIR)/libnurbs/internals/quilt.o			\
		$(GLU_DIR)/libnurbs/internals/reader.o			\
		$(GLU_DIR)/libnurbs/internals/renderhints.o		\
		$(GLU_DIR)/libnurbs/internals/slicer.o			\
		$(GLU_DIR)/libnurbs/internals/sorter.o			\
		$(GLU_DIR)/libnurbs/internals/splitarcs.o			\
		$(GLU_DIR)/libnurbs/internals/subdivider.o		\
		$(GLU_DIR)/libnurbs/internals/tobezier.o			\
		$(GLU_DIR)/libnurbs/internals/trimline.o			\
		$(GLU_DIR)/libnurbs/internals/trimregion.o		\
		$(GLU_DIR)/libnurbs/internals/trimvertpool.o		\
		$(GLU_DIR)/libnurbs/internals/uarray.o			\
		$(GLU_DIR)/libnurbs/internals/varray.o			\
		$(GLU_DIR)/libnurbs/nurbtess/directedLine.o		\
		$(GLU_DIR)/libnurbs/nurbtess/gridWrap.o			\
		$(GLU_DIR)/libnurbs/nurbtess/monoChain.o			\
		$(GLU_DIR)/libnurbs/nurbtess/monoPolyPart.o		\
		$(GLU_DIR)/libnurbs/nurbtess/monoTriangulation.o		\
		$(GLU_DIR)/libnurbs/nurbtess/partitionX.o			\
		$(GLU_DIR)/libnurbs/nurbtess/partitionY.o			\
		$(GLU_DIR)/libnurbs/nurbtess/polyDBG.o			\
		$(GLU_DIR)/libnurbs/nurbtess/polyUtil.o			\
		$(GLU_DIR)/libnurbs/nurbtess/primitiveStream.o		\
		$(GLU_DIR)/libnurbs/nurbtess/quicksort.o			\
		$(GLU_DIR)/libnurbs/nurbtess/rectBlock.o			\
		$(GLU_DIR)/libnurbs/nurbtess/sampleComp.o			\
		$(GLU_DIR)/libnurbs/nurbtess/sampleCompBot.o		\
		$(GLU_DIR)/libnurbs/nurbtess/sampleCompRight.o		\
		$(GLU_DIR)/libnurbs/nurbtess/sampleCompTop.o		\
		$(GLU_DIR)/libnurbs/nurbtess/sampleMonoPoly.o		\
		$(GLU_DIR)/libnurbs/nurbtess/sampledLine.o		\
		$(GLU_DIR)/libnurbs/nurbtess/searchTree.o
endif

INCLUDES = \
	-I../common \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/array_cache \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \
	-I$(TOP)/src/mesa/tnl \
	-I$(TOP)/src/mesa/tnl_dd \
	-I$(TOP)/src/mesa/x86 \
	-I. \
	-I- \
	-I$(TOP)/include

DRIVER_SOURCES = \
	GLView.cpp \
	../common/driverfuncs.c \
	$(addprefix ../../, $(GLAPI_SOURCES))

ifeq ($(CPU), x86)
	DRIVER_SOURCES += $(addprefix ../../, $(X86_API))
else
# No GL API PPC optimization yet
endif 

DRIVER_OBJECTS = $(DRIVER_SOURCES:.c=.o)
DRIVER_OBJECTS := $(DRIVER_OBJECTS:.S=.o)

OBJECTS := $(DRIVER_OBJECTS:.cpp=.o)


# Rules

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.S.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@


default: depend $(TOP)/$(LIB_DIR) $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

$(MESA_MODULES):
	cd $(TOP)/src/mesa; $(MAKE) mesa.a ;

$(GLU_MODULES):
	cd $(GLU_DIR); $(MAKE) $(subst $(GLU_DIR)/,,$(GLU_MODULES)) ; 

$(TOP)/$(LIB_DIR):
	mkdir $(TOP)/$(LIB_DIR)

$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(OBJECTS) $(MESA_MODULES) $(GLU_MODULES)
	@$(TOP)/bin/mklib -o $(GL_LIB) -ldflags '$(LDFLAGS)' -install $(TOP)/$(LIB_DIR) \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		$(MKLIB_OPTIONS) $(GL_LIB_DEPS) $(OBJECTS) $(MESA_MODULES) $(GLU_MODULES)

# $(GLU_OBJECTS):
#	cd $(GLU_DIR); $(MAKE) $< ;

depend: $(DRIVER_SOURCES) $(GLU_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(DRIVER_SOURCES) $(GLU_SOURCES) > /dev/null 

clean:
	rm -f depend $(OBJECTS)

include depend
