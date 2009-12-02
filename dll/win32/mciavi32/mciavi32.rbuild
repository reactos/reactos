<module name="mciavi32" type="win32dll" baseaddress="${BASEADDRESS_MCIAVI32}" installbase="system32" installname="mciavi32.dll" allowwarnings="true">
	<importlibrary definition="mciavi32.spec" />
	<include base="mciavi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>info.c</file>
	<file>mciavi.c</file>
	<file>mmoutput.c</file>
	<file>wnd.c</file>
	<file>mciavi_res.rc</file>
	<library>wine</library>
	<library>msvfw32</library>
	<library>winmm</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
