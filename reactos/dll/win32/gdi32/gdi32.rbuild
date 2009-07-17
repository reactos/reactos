<module name="gdi32" type="win32dll" baseaddress="${BASEADDRESS_GDI32}" installbase="system32" installname="gdi32.dll" unicode="yes">
	<importlibrary definition="gdi32.spec" />
	<include base="gdi32">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="LANGPACK" />
	<library>ntdll</library>
	<library>user32</library>
	<library>kernel32</library>
	<!--library>advapi32</library-->
	<library>win32ksys</library>
	<library>pseh</library>
	<library>dxguid</library>

	<file>bidi.c</file>
	<file>bitblt.c</file>
	<file>bitmap.c</file>
	<file>brush.c</file>
	<file>clipping.c</file>
	<file>dc.c</file>
	<file>dib.c</file>
	<file>driver.c</file>
	<file>enhmetafile.c</file>
	<file>font.c</file>
	<file>freetype.c</file>
	<file>gdi_main.c</file>
	<file>gdiobj.c</file>
	<file>icm.c</file>
	<file>mapping.c</file>
	<file>metafile.c</file>
	<file>opengl.c</file>
	<file>painting.c</file>
	<file>palette.c</file>
	<file>path.c</file>
	<file>pen.c</file>
	<file>printdrv.c</file>
	<file>region.c</file>

	<file>gdi32.rc</file>
</module>
