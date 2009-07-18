<module name="winex11" type="win32dll" baseaddress="${BASEADDRESS_SMDLL}" installbase="system32" installname="winex11.drv" allowwarnings="true">
	<importlibrary definition="winex11.drv.spec" />
	<include base="winex11">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__WINESRC__" />
	<file>bitblt.c</file>
	<file>bitmap.c</file>
	<file>brush.c</file>
	<file>clipboard.c</file>
	<file>codepage.c</file>
	<file>desktop.c</file>
	<file>dib.c</file>
	<file>dib_convert.c</file>
	<file>dib_dst_swap.c</file>
	<file>dib_src_swap.c</file>
	<file>event.c</file>
	<file>graphics.c</file>
	<file>ime.c</file>
	<file>init.c</file>
	<file>keyboard.c</file>
	<file>mouse.c</file>
	<file>opengl.c</file>
	<file>palette.c</file>
	<file>poll.c</file>
	<file>pen.c</file>
	<file>rosglue.c</file>
	<file>scroll.c</file>
	<file>settings.c</file>
	<file>systray.c</file>
	<file>text.c</file>
	<file>window.c</file>
	<file>x11drv_main.c</file>
	<file>xdnd.c</file>
	<file>xim.c</file>
	<file>xfont.c</file>
	<file>xinerama.c</file>
	<file>xrender.c</file>
	<file>xvidmode.c</file>

	<file>winex11.rc</file>

	<library>libX11</library>
	<library>pseh</library>
	<library>advapi32</library>
	<library>msvcrt</library>
	<library>wine</library>
	<library>comctl32</library>
	<library>imm32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>ntdll</library>
	<library>win32ksys</library>
</module>
