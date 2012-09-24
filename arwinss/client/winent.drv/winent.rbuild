<module name="winent" type="win32dll" baseaddress="${BASEADDRESS_WINENT}" installbase="system32" installname="winent.drv">
	<importlibrary definition="winent.drv.spec" />
	<include base="winent">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="WINVER">0x0600</redefine>
	<redefine name="_WIN32_WINNT">0x0501</redefine>
	<define name="__WINESRC__" />
	<define name="_WINE" />
	<file>bitblt.c</file>
	<file>clipboard.c</file>
	<file>dib.c</file>
	<file>event.c</file>
	<file>font.c</file>
	<file>gdidrv.c</file>
	<file>gdiobj.c</file>
	<file>graphics.c</file>
	<file>main.c</file>
	<file>userdrv.c</file>
	<file>mouse.c</file>
	<file>window.c</file>
	<file>ogldrv.c</file>

	<file>winent.rc</file>

	<library>wine</library>
	<library>imm32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>ntdll</library>
	<library>pseh</library>
	<library>win32ksys</library>
</module>
