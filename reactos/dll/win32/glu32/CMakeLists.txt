
if(NOT MSVC)
    set_cpp()
endif()

add_definitions(
    -DRESOLVE_3D_TEXTURE_SUPPORT
    -DBUILD_GL32
    -DLIBRARYBUILD)

include_directories(
    include
    libnurbs/internals
    libnurbs/interface
    libnurbs/nurbtess
    libtess
    libutil)

spec2def(glu32.dll glu32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    libnurbs/interface/bezierEval.cc
    libnurbs/interface/bezierPatch.cc
    libnurbs/interface/bezierPatchMesh.cc
    libnurbs/interface/glcurveval.cc
    libnurbs/interface/glinterface.cc
    libnurbs/interface/glrenderer.cc
    libnurbs/interface/glsurfeval.cc
    libnurbs/interface/incurveeval.cc
    libnurbs/interface/insurfeval.cc
    libnurbs/internals/arc.cc
    libnurbs/internals/arcsorter.cc
    libnurbs/internals/arctess.cc
    libnurbs/internals/backend.cc
    libnurbs/internals/basiccrveval.cc
    libnurbs/internals/basicsurfeval.cc
    libnurbs/internals/bin.cc
    libnurbs/internals/bufpool.cc
    libnurbs/internals/cachingeval.cc
    libnurbs/internals/ccw.cc
    libnurbs/internals/coveandtiler.cc
    libnurbs/internals/curve.cc
    libnurbs/internals/curvelist.cc
    libnurbs/internals/curvesub.cc
    libnurbs/internals/dataTransform.cc
    libnurbs/internals/displaylist.cc
    libnurbs/internals/flist.cc
    libnurbs/internals/flistsorter.cc
    libnurbs/internals/hull.cc
    libnurbs/internals/intersect.cc
    libnurbs/internals/knotvector.cc
    libnurbs/internals/mapdesc.cc
    libnurbs/internals/mapdescv.cc
    libnurbs/internals/maplist.cc
    libnurbs/internals/mesher.cc
    libnurbs/internals/monoTriangulationBackend.cc
    libnurbs/internals/monotonizer.cc
    libnurbs/internals/mycode.cc
    libnurbs/internals/nurbsinterfac.cc
    libnurbs/internals/nurbstess.cc
    libnurbs/internals/patch.cc
    libnurbs/internals/patchlist.cc
    libnurbs/internals/quilt.cc
    libnurbs/internals/reader.cc
    libnurbs/internals/renderhints.cc
    libnurbs/internals/slicer.cc
    libnurbs/internals/sorter.cc
    libnurbs/internals/splitarcs.cc
    libnurbs/internals/subdivider.cc
    libnurbs/internals/tobezier.cc
    libnurbs/internals/trimline.cc
    libnurbs/internals/trimregion.cc
    libnurbs/internals/trimvertpool.cc
    libnurbs/internals/uarray.cc
    libnurbs/internals/varray.cc
    libnurbs/nurbtess/directedLine.cc
    libnurbs/nurbtess/gridWrap.cc
    libnurbs/nurbtess/monoChain.cc
    libnurbs/nurbtess/monoPolyPart.cc
    libnurbs/nurbtess/monoTriangulation.cc
    libnurbs/nurbtess/partitionX.cc
    libnurbs/nurbtess/partitionY.cc
    libnurbs/nurbtess/polyDBG.cc
    libnurbs/nurbtess/polyUtil.cc
    libnurbs/nurbtess/primitiveStream.cc
    libnurbs/nurbtess/quicksort.cc
    libnurbs/nurbtess/rectBlock.cc
    libnurbs/nurbtess/sampleComp.cc
    libnurbs/nurbtess/sampleCompBot.cc
    libnurbs/nurbtess/sampleCompRight.cc
    libnurbs/nurbtess/sampleCompTop.cc
    libnurbs/nurbtess/sampleMonoPoly.cc
    libnurbs/nurbtess/sampledLine.cc
    libnurbs/nurbtess/searchTree.cc
    libtess/dict.c
    libtess/geom.c
    libtess/memalloc.c
    libtess/mesh.c
    libtess/normal.c
    libtess/priorityq.c
    libtess/render.c
    libtess/sweep.c
    libtess/tess.c
    libtess/tessmono.c
    libutil/error.c
    libutil/glue.c
    libutil/mipmap.c
    libutil/project.c
    libutil/quad.c
    libutil/registry.c
    ${CMAKE_CURRENT_BINARY_DIR}/glu32.def)

add_library(glu32 SHARED ${SOURCE})
allow_warnings(glu32)
set_module_type(glu32 win32dll ENTRYPOINT 0 )

add_importlibs(glu32
    opengl32
    gdi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET glu32 DESTINATION reactos/system32 FOR all)

