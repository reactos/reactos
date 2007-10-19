<module name="dflat32" type="win32cui" installbase="system32" installname="edit.exe">
	<include base="ReactOS">include/wine</include>
	<include base="dflat32">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>gdi32</library>
 	<file>applicat.c</file>
 	<file>barchart.c</file>
 	<file>box.c</file>
 	<file>button.c</file>
 	<file>calendar.c</file>
 	<file>checkbox.c</file>
 	<file>clipbord.c</file>
	<file>combobox.c</file>
 	<file>config.c</file>
 	<file>console.c</file>
 	<file>decomp.c</file>
 	<file>dfalloc.c</file>
 	<file>dialbox.c</file>
 	<file>dialogs.c</file>
	<file>direct.c</file>
 	<file>edit.c</file>
 	<file>editbox.c</file>
 	<file>fileopen.c</file>
 	<file>helpbox.c</file>
 	<file>htree.c</file>
 	<file>keys.c</file>
	<file>listbox.c</file>
 	<file>lists.c</file>
 	<file>log.c</file>
 	<file>menu.c</file>
 	<file>menubar.c</file>
 	<file>menus.c</file>
 	<file>message.c</file>
 	<file>msgbox.c</file>
	<file>normal.c</file>
 	<file>pictbox.c</file>
 	<file>popdown.c</file>
 	<file>radio.c</file>
 	<file>rect.c</file>
 	<file>search.c</file>
 	<file>slidebox.c</file>
 	<file>spinbutt.c</file>
	<file>statbar.c</file>
 	<file>sysmenu.c</file>
 	<file>text.c</file>
 	<file>textbox.c</file>
 	<file>video.c</file>
 	<file>watch.c</file>
 	<file>window.c</file>
</module>
