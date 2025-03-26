
include_directories(BEFORE
    include
    src/include
    src/libnurbs/internals
    src/libnurbs/interface
    src/libnurbs/nurbtess)

add_definitions(
    -DBUILD_GLU32
    -DNDEBUG=
    -DLIBRARYBUILD
    -DRESOLVE_3D_TEXTURE_SUPPORT)

# we must use our own spec file
spec2def(glu32.dll glu32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    src/libutil/error.c
    src/libutil/glue.c
    src/libutil/mipmap.c
    src/libutil/project.c
    src/libutil/quad.c
    src/libutil/registry.c
    src/libtess/dict.c
    src/libtess/geom.c
    src/libtess/memalloc.c
    src/libtess/mesh.c
    src/libtess/normal.c
    src/libtess/priorityq.c
    src/libtess/render.c
    src/libtess/sweep.c
    src/libtess/tess.c
    src/libtess/tessmono.c)

list(APPEND LIBNURBS_SOURCE
    src/libnurbs/interface/bezierEval.cc
    src/libnurbs/interface/bezierPatch.cc
    src/libnurbs/interface/bezierPatchMesh.cc
    src/libnurbs/interface/glcurveval.cc
    src/libnurbs/interface/glinterface.cc
    src/libnurbs/interface/glrenderer.cc
    src/libnurbs/interface/glsurfeval.cc
    src/libnurbs/interface/incurveeval.cc
    src/libnurbs/interface/insurfeval.cc
    src/libnurbs/internals/arc.cc
    src/libnurbs/internals/arcsorter.cc
    src/libnurbs/internals/arctess.cc
    src/libnurbs/internals/backend.cc
    src/libnurbs/internals/basiccrveval.cc
    src/libnurbs/internals/basicsurfeval.cc
    src/libnurbs/internals/bin.cc
    src/libnurbs/internals/bufpool.cc
    src/libnurbs/internals/cachingeval.cc
    src/libnurbs/internals/ccw.cc
    src/libnurbs/internals/coveandtiler.cc
    src/libnurbs/internals/curve.cc
    src/libnurbs/internals/curvelist.cc
    src/libnurbs/internals/curvesub.cc
    src/libnurbs/internals/dataTransform.cc
    src/libnurbs/internals/displaylist.cc
    src/libnurbs/internals/flist.cc
    src/libnurbs/internals/flistsorter.cc
    src/libnurbs/internals/hull.cc
    src/libnurbs/internals/intersect.cc
    src/libnurbs/internals/knotvector.cc
    src/libnurbs/internals/mapdesc.cc
    src/libnurbs/internals/mapdescv.cc
    src/libnurbs/internals/maplist.cc
    src/libnurbs/internals/mesher.cc
    src/libnurbs/internals/monoTriangulationBackend.cc
    src/libnurbs/internals/monotonizer.cc
    src/libnurbs/internals/mycode.cc
    src/libnurbs/internals/nurbsinterfac.cc
    src/libnurbs/internals/nurbstess.cc
    src/libnurbs/internals/patch.cc
    src/libnurbs/internals/patchlist.cc
    src/libnurbs/internals/quilt.cc
    src/libnurbs/internals/reader.cc
    src/libnurbs/internals/renderhints.cc
    src/libnurbs/internals/slicer.cc
    src/libnurbs/internals/sorter.cc
    src/libnurbs/internals/splitarcs.cc
    src/libnurbs/internals/subdivider.cc
    src/libnurbs/internals/tobezier.cc
    src/libnurbs/internals/trimline.cc
    src/libnurbs/internals/trimregion.cc
    src/libnurbs/internals/trimvertpool.cc
    src/libnurbs/internals/uarray.cc
    src/libnurbs/internals/varray.cc
    src/libnurbs/nurbtess/directedLine.cc
    src/libnurbs/nurbtess/gridWrap.cc
    src/libnurbs/nurbtess/monoChain.cc
    src/libnurbs/nurbtess/monoPolyPart.cc
    src/libnurbs/nurbtess/monoTriangulation.cc
    src/libnurbs/nurbtess/partitionX.cc
    src/libnurbs/nurbtess/partitionY.cc
    src/libnurbs/nurbtess/polyUtil.cc
    src/libnurbs/nurbtess/primitiveStream.cc
    src/libnurbs/nurbtess/quicksort.cc
    src/libnurbs/nurbtess/rectBlock.cc
    src/libnurbs/nurbtess/sampleComp.cc
    src/libnurbs/nurbtess/sampleCompBot.cc
    src/libnurbs/nurbtess/sampleCompRight.cc
    src/libnurbs/nurbtess/sampleCompTop.cc
    src/libnurbs/nurbtess/sampleMonoPoly.cc
    src/libnurbs/nurbtess/sampledLine.cc
    src/libnurbs/nurbtess/searchTree.cc)

list(APPEND PCH_SKIP_SOURCE
    src/libnurbs/nurbtess/polyDBG.cc
    ${SOURCE})

add_library(glu32 MODULE
    ${LIBNURBS_SOURCE}
    ${PCH_SKIP_SOURCE}
    glu32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/glu32.def)
target_link_libraries(glu32 cpprt)

set_module_type(glu32 win32dll)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(glu32 PRIVATE -Wno-write-strings)
    target_compile_options(glu32 PRIVATE -Wno-unused-but-set-variable)
    # Prevent a warning when comparing 'this' against 0
    set_source_files_properties(src/libnurbs/internals/arc.cc PROPERTIES COMPILE_OPTIONS "-Wno-nonnull-compare")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(glu32 PRIVATE -Wno-write-strings)
    target_compile_options(glu32 PRIVATE -Wno-self-assign -Wno-unused-function -Wno-microsoft-include)
    target_compile_options(glu32 PRIVATE -Wno-deprecated-register -Wno-tautological-undefined-compare)
endif()

add_importlibs(glu32 opengl32 gdi32 msvcrt kernel32 ntdll)
add_pch(glu32 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET glu32 DESTINATION reactos/system32 FOR all)
