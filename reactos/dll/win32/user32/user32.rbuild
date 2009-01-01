<module name="user32" type="win32dll" baseaddress="${BASEADDRESS_USER32}" installbase="system32" installname="user32.dll" unicode="yes" crt="dll" allowwarnings="true">
	<importlibrary definition="user32.pspec" />
	<include base="user32">.</include>
	<include base="user32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<library>wine</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>imm32</library>
	<library>win32ksys</library>
	<library>pseh</library>
	<library>ntdll</library>

	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38269
	<directory name="include">
		<pch>user32.h</pch>
	</directory>
	-->
	<directory name="controls">
		<file>button.c</file>
		<file>combo.c</file>
		<file>edit.c</file>
		<file>icontitle.c</file>
		<file>listbox.c</file>
		<file>regcontrol.c</file>
		<file>scrollbar.c</file>
		<file>static.c</file>
	</directory>
	<directory name="misc">
		<file>dde.c</file>
		<file>ddeclient.c</file>
		<file>ddeserver.c</file>
		<file>desktop.c</file>
		<file>display.c</file>
		<file>dllmain.c</file>
		<file>exit.c</file>
		<file>exticon.c</file>
		<file>misc.c</file>
		<file>object.c</file>
		<file>resources.c</file>
		<file>stubs.c</file>
		<file>timer.c</file>
		<file>winhelp.c</file>
		<file>winsta.c</file>
		<file>wsprintf.c</file>
	</directory>
	<directory name="windows">
		<file>accel.c</file>
		<file>bitmap.c</file>
		<file>caret.c</file>
		<file>class.c</file>
		<file>clipboard.c</file>
		<file>cursor.c</file>
		<file>dc.c</file>
		<file>defwnd.c</file>
		<file>dialog.c</file>
		<file>draw.c</file>
		<file>font.c</file>
		<file>hook.c</file>
		<file>icon.c</file>
		<file>input.c</file>
		<file>mdi.c</file>
		<file>menu.c</file>
		<file>message.c</file>
		<file>messagebox.c</file>
		<file>nonclient.c</file>
		<file>paint.c</file>
		<file>prop.c</file>
		<file>rect.c</file>
		<file>spy.c</file>
		<file>text.c</file>
		<file>window.c</file>
		<file>winpos.c</file>
	</directory>
	<file>user32.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag>-fno-unit-at-a-time</compilerflag>
</module>
