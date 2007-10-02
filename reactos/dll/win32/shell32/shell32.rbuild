<module name="shell32" type="win32dll" baseaddress="${BASEADDRESS_SHELL32}" installbase="system32" installname="shell32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="shell32.spec.def" />
	<include base="shell32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_SHELL32_" />
	<define name="COM_NO_WINDOWS_H" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>version</library>
	<library>devmgr</library>
	<file>authors.c</file>
	<file>autocomplete.c</file>
	<file>brsfolder.c</file>
	<file>changenotify.c</file>
	<file>classes.c</file>
	<file>clipboard.c</file>
	<file>control.c</file>
	<file>cpanelfolder.c</file>
	<file>dataobject.c</file>
	<file>dde.c</file>
	<file>debughlp.c</file>
	<file>dialogs.c</file>
	<file>dragdrophelper.c</file>
	<file>enumidlist.c</file>
	<file>folders.c</file>
	<file>iconcache.c</file>
	<file>pidl.c</file>
	<file>regsvr.c</file>
	<file>shell32_main.c</file>
	<file>shelllink.c</file>
	<file>shellole.c</file>
	<file>shellord.c</file>
	<file>shellpath.c</file>
	<file>shellreg.c</file>
	<file>shellstring.c</file>
	<file>shfldr_desktop.c</file>
	<file>shfldr_fs.c</file>
	<file>shfldr_mycomp.c</file>
	<file>shlexec.c</file>
	<file>shlfileop.c</file>
	<file>shlfolder.c</file>
	<file>shlfsbind.c</file>
	<file>shlmenu.c</file>
	<file>shlview.c</file>
	<file>shpolicy.c</file>
	<file>shv_bg_cmenu.c</file>
	<file>shv_item_cmenu.c</file>
	<file>ros-systray.c</file>
	<file>shres.rc</file>
	<file>shell32.spec</file>
	<file>fprop.c</file>
	<file>drive.c</file>
</module>
