<module name="libpng" type="staticlibrary" allowwarnings="true">
	<define name="WIN32" />
	<define name="NDEBUG" />
	<define name="_WINDOWS" />
	<define name="_WINDOWS_" />
	<define name="_USRDLL" />
	<include base="libpng">.</include>
	<include base="ReactOS">lib/3rdparty/zlib</include>
	<file>png.c</file>
	<file>pngerror.c</file>
	<file>pnggccrd.c</file>
	<file>pngget.c</file>
	<file>pngmem.c</file>
	<file>pngpread.c</file>
	<file>pngread.c</file>
	<file>pngrio.c</file>
	<file>pngrtran.c</file>
	<file>pngrutil.c</file>
	<file>pngset.c</file>
	<file>pngtest.c</file>
	<file>pngtrans.c</file>
	<file>pngvcrd.c</file>
	<file>pngwio.c</file>
	<file>pngwrite.c</file>
	<file>pngwtran.c</file>
	<file>pngwutil.c</file>
</module>