PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NORC = yes

TARGET_NAME = glu32

TARGET_BASE = $(TARGET_BASE_LIB_GLU32)

TARGET_CFLAGS = -D__USE_W32API -DRESOLVE_3D_TEXTURE_SUPPORT -DBUILD_GL32 -DLIBRARYBUILD -Wall -Werror -Iinclude -Ilibnurbs/internals -Ilibnurbs/interface -Ilibnurbs/nurbtess

TARGET_CPPFLAGS = $(TARGET_CFLAGS)

TARGET_GCCLIBS = opengl32 stdc++

TARGET_OBJECTS = \
	libnurbs/interface/bezierEval.o \
	libnurbs/interface/bezierPatch.o \
	libnurbs/interface/bezierPatchMesh.o \
	libnurbs/interface/glcurveval.o \
	libnurbs/interface/glinterface.o \
	libnurbs/interface/glrenderer.o \
	libnurbs/interface/glsurfeval.o \
	libnurbs/interface/incurveeval.o \
	libnurbs/interface/insurfeval.o \
	libnurbs/internals/arc.o \
	libnurbs/internals/arcsorter.o \
	libnurbs/internals/arctess.o \
	libnurbs/internals/backend.o \
	libnurbs/internals/basiccrveval.o \
	libnurbs/internals/basicsurfeval.o \
	libnurbs/internals/bin.o \
	libnurbs/internals/bufpool.o \
	libnurbs/internals/cachingeval.o \
	libnurbs/internals/ccw.o \
	libnurbs/internals/coveandtiler.o \
	libnurbs/internals/curve.o \
	libnurbs/internals/curvelist.o \
	libnurbs/internals/curvesub.o \
	libnurbs/internals/dataTransform.o \
	libnurbs/internals/displaylist.o \
	libnurbs/internals/flist.o \
	libnurbs/internals/flistsorter.o \
	libnurbs/internals/hull.o \
	libnurbs/internals/intersect.o \
	libnurbs/internals/knotvector.o \
	libnurbs/internals/mapdesc.o \
	libnurbs/internals/mapdescv.o \
	libnurbs/internals/maplist.o \
	libnurbs/internals/mesher.o \
	libnurbs/internals/monoTriangulationBackend.o \
	libnurbs/internals/monotonizer.o \
	libnurbs/internals/mycode.o \
	libnurbs/internals/nurbsinterfac.o \
	libnurbs/internals/nurbstess.o \
	libnurbs/internals/patch.o \
	libnurbs/internals/patchlist.o \
	libnurbs/internals/quilt.o \
	libnurbs/internals/reader.o \
	libnurbs/internals/renderhints.o \
	libnurbs/internals/slicer.o \
	libnurbs/internals/sorter.o \
	libnurbs/internals/splitarcs.o \
	libnurbs/internals/subdivider.o \
	libnurbs/internals/tobezier.o \
	libnurbs/internals/trimline.o \
	libnurbs/internals/trimregion.o \
	libnurbs/internals/trimvertpool.o \
	libnurbs/internals/uarray.o \
	libnurbs/internals/varray.o \
	libnurbs/nurbtess/directedLine.o \
	libnurbs/nurbtess/gridWrap.o \
	libnurbs/nurbtess/monoChain.o \
	libnurbs/nurbtess/monoPolyPart.o \
	libnurbs/nurbtess/monoTriangulation.o \
	libnurbs/nurbtess/partitionX.o \
	libnurbs/nurbtess/partitionY.o \
	libnurbs/nurbtess/polyDBG.o \
	libnurbs/nurbtess/polyUtil.o \
	libnurbs/nurbtess/primitiveStream.o \
	libnurbs/nurbtess/quicksort.o \
	libnurbs/nurbtess/rectBlock.o \
	libnurbs/nurbtess/sampleComp.o \
	libnurbs/nurbtess/sampleCompBot.o \
	libnurbs/nurbtess/sampleCompRight.o \
	libnurbs/nurbtess/sampleCompTop.o \
	libnurbs/nurbtess/sampleMonoPoly.o \
	libnurbs/nurbtess/sampledLine.o \
	libnurbs/nurbtess/searchTree.o \
	libtess/dict.o \
	libtess/geom.o \
	libtess/memalloc.o \
	libtess/mesh.o \
	libtess/normal.o \
	libtess/priorityq.o \
	libtess/render.o \
	libtess/sweep.o \
	libtess/tess.o \
	libtess/tessmono.o \
	libutil/error.o \
	libutil/glue.o \
	libutil/mipmap.o \
	libutil/project.o \
	libutil/quad.o \
	libutil/registry.o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
