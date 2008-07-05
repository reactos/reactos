<module name="mc" type="win32cui" installbase="system32" installname="mc.exe" allowwarnings="true">
	<include base="mc">src</include>
	<include base="mc">pc</include>
	<include base="mc">slang</include>
	<include base="mc">edit</include>
	<define name="__USE_W32API" />
	<define name="DMC_NT" />
	<define name="_OS_NT" />
	<define name="HAVE_CONFIG_H" />
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>

	<directory name="src">
		<file>terms.c</file>
		<file>user.c</file>
		<file>file.c</file>
		<file>listmode.c</file>
		<file>cmd.c</file>
		<file>command.c</file>
		<file>help.c</file>
		<file>menu.c</file>
		<file>view.c</file>
		<file>dir.c</file>
		<file>info.c</file>
		<file>widget.c</file>
		<file>option.c</file>
		<file>dlg.c</file>
		<file>panelize.c</file>
		<file>profile.c</file>
		<file>util.c</file>
		<file>dialog.c</file>
		<file>ext.c</file>
		<file>color.c</file>
		<file>layout.c</file>
		<file>setup.c</file>
		<file>regex.c</file>
		<file>hotlist.c</file>
		<file>tree.c</file>
		<file>win.c</file>
		<file>complete.c</file>
		<file>find.c</file>
		<file>wtools.c</file>
		<file>boxes.c</file>
		<file>background.c</file>
		<file>main.c</file>
		<file>popt.c</file>
		<file>text.c</file>
		<file>screen.c</file>
	</directory>

	<directory name="pc">
		<file>slint_pc.c</file>
		<file>chmod.c</file>
		<file>drive.c</file>
		<file>cons_nt.c</file>
		<file>dirent_nt.c</file>
		<file>key_nt.c</file>
		<file>util_win32.c</file>
		<file>util_winnt.c</file>
		<file>util_nt.c</file>
	</directory>

	<directory name="slang">
		<file>slerr.c</file>
		<file>slgetkey.c</file>
		<file>slsmg.c</file>
		<file>slvideo.c</file>
		<file>slw32tty.c</file>
	</directory>

	<directory name="edit">
		<file>edit.c</file>
		<file>editcmd.c</file>
		<file>editdraw.c</file>
		<file>editmenu.c</file>
		<file>editoptions.c</file>
		<file>editwidget.c</file>
		<file>syntax.c</file>
		<file>wordproc.c</file>
	</directory>

	<file>mc.rc</file>
</module>
