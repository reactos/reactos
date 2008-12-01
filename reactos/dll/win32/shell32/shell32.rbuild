<module name="shell32" type="win32dll" baseaddress="${BASEADDRESS_SHELL32}" installbase="system32" installname="shell32.dll">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="shell32.spec" />
	<include base="shell32">.</include>
	<include base="recyclebin">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_SHELL32_" />
	<define name="COM_NO_WINDOWS_H" />
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>uuid</library>
	<library>recyclebin</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>version</library>
	<library>devmgr</library>
	<library>winspool</library>
	<library>winmm</library>
	<pch>precomp.h</pch>
	<file>authors.c</file>
	<file>autocomplete.c</file>
	<file>brsfolder.c</file>
	<file>changenotify.c</file>
	<file>classes.c</file>
	<file>clipboard.c</file>
	<file>control.c</file>
	<file>dataobject.c</file>
	<file>dde.c</file>
	<file>debughlp.c</file>
	<file>desktop.c</file>
	<file>dialogs.c</file>
	<file>dragdrophelper.c</file>
	<file>enumidlist.c</file>
	<file>extracticon.c</file>
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
	<file>shfldr_mydocuments.c</file>
	<file>shfldr_printers.c</file>
	<file>shfldr_admintools.c</file>
	<file>shfldr_netplaces.c</file>
	<file>shfldr_fonts.c</file>
	<file>shfldr_cpanel.c</file>
	<file>shfldr_recyclebin.c</file>
	<file>shlexec.c</file>
	<file>shlfileop.c</file>
	<file>shlfolder.c</file>
	<file>shlfsbind.c</file>
	<file>shlmenu.c</file>
	<file>shlview.c</file>
	<file>shpolicy.c</file>
	<file>shv_def_cmenu.c</file>
	<file>startmenu.c</file>
	<file>ros-systray.c</file>
	<file>fprop.c</file>
	<file>drive.c</file>
	<file>she_ocmenu.c</file>
	<file>shv_item_new.c</file>
	<file>folder_options.c</file>
	<file>shell32.rc</file>
</module>
