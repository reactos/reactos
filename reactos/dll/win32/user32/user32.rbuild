<module name="user32" type="win32dll" baseaddress="${BASEADDRESS_USER32}" installbase="system32" installname="user32.dll" unicode="yes" allowwarnings="true">
	<importlibrary definition="user32.spec" />
	<include base="user32">.</include>
	<include base="user32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>ntdll</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<!--library>imm32</library-->
	<library>win32ksys</library>
	<library>pseh</library>

	<file>button.c</file>
	<file>caret.c</file>
	<file>class.c</file>
	<file>clipboard.c</file>
	<file>combo.c</file>
	<file>cursoricon.c</file>
	<file>dde_client.c</file>
	<file>dde_misc.c</file>
	<file>dde_server.c</file>
	<file>defdlg.c</file>
	<file>defwnd.c</file>
	<file>desktop.c</file>
	<file>dialog.c</file>
	<file>driver.c</file>
	<file>edit.c</file>
	<file>exticon.c</file>
	<file>focus.c</file>
	<file>hook.c</file>
	<file>icontitle.c</file>
	<file>input.c</file>
	<file>legacy.c</file>
	<file>listbox.c</file>
	<file>lstr.c</file>
	<file>mdi.c</file>
	<file>menu.c</file>
	<file>message.c</file>
	<file>misc.c</file>
	<file>msgbox.c</file>
	<file>nonclient.c</file>
	<file>painting.c</file>
	<file>property.c</file>
	<file>resource.c</file>
	<file>scroll.c</file>
	<file>spy.c</file>
	<file>static.c</file>
	<file>sysparams.c</file>
	<file>text.c</file>
	<file>uitools.c</file>
	<file>user_main.c</file>
	<file>win.c</file>
	<file>winhelp.c</file>
	<file>winpos.c</file>
	<file>winproc.c</file>
	<file>winstation.c</file>
	<file>wsprintf.c</file>

	<directory name="resources">
		<file>user32.rc</file>
	</directory>
</module>
