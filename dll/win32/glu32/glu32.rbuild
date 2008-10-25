<module name="glu32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_GLU32}" installbase="system32" installname="glu32.dll" allowwarnings="true">
	<importlibrary definition="glu32.def" />
	<include base="glu32">include</include>
	<include base="glu32">libnurbs/internals</include>
	<include base="glu32">libnurbs/interface</include>
	<include base="glu32">libnurbs/nurbtess</include>
	<include base="glu32">libtess</include>
	<include base="glu32">libutil</include>
	<define name="RESOLVE_3D_TEXTURE_SUPPORT" />
	<define name="BUILD_GL32" />
	<define name="LIBRARYBUILD" />
	<compilerflag compiler="cpp">-Wno-non-virtual-dtor</compilerflag>
	<library>ntdll</library>
	<library>opengl32</library>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>msvcrt</library>
	<directory name="libnurbs">
		<directory name="interface">
			<file>bezierEval.cc</file>
			<file>bezierPatch.cc</file>
			<file>bezierPatchMesh.cc</file>
			<file>glcurveval.cc</file>
			<file>glinterface.cc</file>
			<file>glrenderer.cc</file>
			<file>glsurfeval.cc</file>
			<file>incurveeval.cc</file>
			<file>insurfeval.cc</file>
		</directory>
		<directory name="internals">
			<file>arc.cc</file>
			<file>arcsorter.cc</file>
			<file>arctess.cc</file>
			<file>backend.cc</file>
			<file>basiccrveval.cc</file>
			<file>basicsurfeval.cc</file>
			<file>bin.cc</file>
			<file>bufpool.cc</file>
			<file>cachingeval.cc</file>
			<file>ccw.cc</file>
			<file>coveandtiler.cc</file>
			<file>curve.cc</file>
			<file>curvelist.cc</file>
			<file>curvesub.cc</file>
			<file>dataTransform.cc</file>
			<file>displaylist.cc</file>
			<file>flist.cc</file>
			<file>flistsorter.cc</file>
			<file>hull.cc</file>
			<file>intersect.cc</file>
			<file>knotvector.cc</file>
			<file>mapdesc.cc</file>
			<file>mapdescv.cc</file>
			<file>maplist.cc</file>
			<file>mesher.cc</file>
			<file>monoTriangulationBackend.cc</file>
			<file>monotonizer.cc</file>
			<file>mycode.cc</file>
			<file>nurbsinterfac.cc</file>
			<file>nurbstess.cc</file>
			<file>patch.cc</file>
			<file>patchlist.cc</file>
			<file>quilt.cc</file>
			<file>reader.cc</file>
			<file>renderhints.cc</file>
			<file>slicer.cc</file>
			<file>sorter.cc</file>
			<file>splitarcs.cc</file>
			<file>subdivider.cc</file>
			<file>tobezier.cc</file>
			<file>trimline.cc</file>
			<file>trimregion.cc</file>
			<file>trimvertpool.cc</file>
			<file>uarray.cc</file>
			<file>varray.cc</file>
		</directory>
		<directory name="nurbtess">
			<file>directedLine.cc</file>
			<file>gridWrap.cc</file>
			<file>monoChain.cc</file>
			<file>monoPolyPart.cc</file>
			<file>monoTriangulation.cc</file>
			<file>partitionX.cc</file>
			<file>partitionY.cc</file>
			<file>polyDBG.cc</file>
			<file>polyUtil.cc</file>
			<file>primitiveStream.cc</file>
			<file>quicksort.cc</file>
			<file>rectBlock.cc</file>
			<file>sampleComp.cc</file>
			<file>sampleCompBot.cc</file>
			<file>sampleCompRight.cc</file>
			<file>sampleCompTop.cc</file>
			<file>sampleMonoPoly.cc</file>
			<file>sampledLine.cc</file>
			<file>searchTree.cc</file>
		</directory>
	</directory>
	<directory name="libtess">
		<file>dict.c</file>
		<file>geom.c</file>
		<file>memalloc.c</file>
		<file>mesh.c</file>
		<file>normal.c</file>
		<file>priorityq.c</file>
		<file>render.c</file>
		<file>sweep.c</file>
		<file>tess.c</file>
		<file>tessmono.c</file>
	</directory>
	<directory name="libutil">
		<file>error.c</file>
		<file>glue.c</file>
		<file>mipmap.c</file>
		<file>project.c</file>
		<file>quad.c</file>
		<file>registry.c</file>
	</directory>
</module>
