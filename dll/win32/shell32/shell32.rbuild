<group>
<module name="shell32" type="win32dll" baseaddress="${BASEADDRESS_SHELL32}" installbase="system32" installname="shell32.dll" allowwarnings="true" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="shell32.spec" />
	<include base="recyclebin">.</include>
	<include base="ReactOS">include/reactos/wine</include>
  <include base="atlnew">.</include>
  <define name="_SHELL32_" />
	<define name="COM_NO_WINDOWS_H" />
	<define name="_WINE" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>wine</library>
	<library>uuid</library>
	<library>recyclebin</library>
	<library>ntdll</library>
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
	<library>msvcrt</library>
	<library>atlnew</library>
	<pch>precomp.h</pch>
	<file>authors.cpp</file>
	<file>autocomplete.cpp</file>
	<file>brsfolder.cpp</file>
	<file>changenotify.cpp</file>
	<file>classes.cpp</file>
	<file>clipboard.cpp</file>
	<file>control.cpp</file>
	<file>dataobject.cpp</file>
	<file>dde.cpp</file>
	<file>debughlp.cpp</file>
	<file>desktop.cpp</file>
	<file>dialogs.cpp</file>
	<file>dragdrophelper.cpp</file>
	<file>enumidlist.cpp</file>
	<file>extracticon.cpp</file>
	<file>folders.cpp</file>
	<file>iconcache.cpp</file>
	<file>pidl.cpp</file>
	<file>shell32_main.cpp</file>
	<file>shellitem.cpp</file>
	<file>shelllink.cpp</file>
	<file>shellole.cpp</file>
	<file>shellord.cpp</file>
	<file>shellpath.cpp</file>
	<file>shellreg.cpp</file>
	<file>shellstring.cpp</file>
	<file>shfldr_desktop.cpp</file>
	<file>shfldr_fs.cpp</file>
	<file>shfldr_mycomp.cpp</file>
	<file>shfldr_mydocuments.cpp</file>
	<file>shfldr_printers.cpp</file>
	<file>shfldr_admintools.cpp</file>
	<file>shfldr_netplaces.cpp</file>
	<file>shfldr_fonts.cpp</file>
	<file>shfldr_cpanel.cpp</file>
	<file>shfldr_recyclebin.cpp</file>
	<file>shlexec.cpp</file>
	<file>shlfileop.cpp</file>
	<file>shlfolder.cpp</file>
	<file>shlfsbind.cpp</file>
	<file>shlmenu.cpp</file>
	<file>shlview.cpp</file>
	<file>shpolicy.cpp</file>
	<file>shv_def_cmenu.cpp</file>
	<file>startmenu.cpp</file>
	<file>stubs.cpp</file>
	<file>ros-systray.cpp</file>
	<file>fprop.cpp</file>
	<file>drive.cpp</file>
	<file>she_ocmenu.cpp</file>
	<file>shv_item_new.cpp</file>
	<file>folder_options.cpp</file>
	<file>shell32.rc</file>
</module>
<module name="shobjidl_local_interface" type="idlinterface">
	<file>shobjidl_local.idl</file>
</module>
</group>
