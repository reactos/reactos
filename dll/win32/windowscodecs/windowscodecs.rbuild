<module name="windowscodecs" type="win32dll" baseaddress="${BASEADDRESS_WINDOWSCODECS}" installbase="system32" installname="windowscodecs.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="windowscodecs.spec" />
	<include base="windowscodecs">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS">include/reactos/libs/libjpeg</include>
	<define name="__WINESRC__" />

	<redefine name="_WIN32_WINNT">0x600</redefine>

	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>advapi32</library>

	<file>bmpdecode.c</file>
	<file>bmpencode.c</file>
	<file>clsfactory.c</file>
	<file>converter.c</file>
	<file>gifformat.c</file>
	<file>icoformat.c</file>
	<file>imgfactory.c</file>
	<file>info.c</file>
	<file>jpegformat.c</file>
	<file>main.c</file>
	<file>palette.c</file>
	<file>pngformat.c</file>
	<file>propertybag.c</file>
	<file>regsvr.c</file>
	<file>stream.c</file>
	<file>tiffformat.c</file>
	<file>ungif.c</file>

	<file>version.rc</file>
</module>
