<module name="comctl32" type="win32dll" baseaddress="${BASEADDRESS_COMCTL32}" installbase="system32" installname="comctl32.dll" allowwarnings="true">
	<importlibrary definition="comctl32.spec.def" />
	<include base="comctl32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>winmm</library>
	<library>uxtheme</library>
	<library>ntdll</library>
	<file>animate.c</file>
	<library>ntdll</library>
	<file>comboex.c</file>
	<library>ntdll</library>
	<file>comctl32undoc.c</file>
	<library>ntdll</library>
	<file>commctrl.c</file>
	<library>ntdll</library>
	<file>datetime.c</file>
	<library>ntdll</library>
	<file>dpa.c</file>
	<library>ntdll</library>
	<file>draglist.c</file>
	<library>ntdll</library>
	<file>dsa.c</file>
	<library>ntdll</library>
	<file>flatsb.c</file>
	<library>ntdll</library>
	<file>header.c</file>
	<library>ntdll</library>
	<file>hotkey.c</file>
	<library>ntdll</library>
	<file>imagelist.c</file>
	<library>ntdll</library>
	<file>ipaddress.c</file>
	<library>ntdll</library>
	<file>listview.c</file>
	<library>ntdll</library>
	<file>monthcal.c</file>
	<library>ntdll</library>
	<file>nativefont.c</file>
	<library>ntdll</library>
	<file>pager.c</file>
	<library>ntdll</library>
	<file>progress.c</file>
	<library>ntdll</library>
	<file>propsheet.c</file>
	<library>ntdll</library>
	<file>rebar.c</file>
	<library>ntdll</library>
	<file>smoothscroll.c</file>
	<library>ntdll</library>
	<file>status.c</file>
	<library>ntdll</library>
	<file>string.c</file>
	<library>ntdll</library>
	<file>syslink.c</file>
	<library>ntdll</library>
	<file>tab.c</file>
	<library>ntdll</library>
	<file>theme_combo.c</file>
	<library>ntdll</library>
	<file>theme_dialog.c</file>
	<library>ntdll</library>
	<file>theme_edit.c</file>
	<library>ntdll</library>
	<file>theme_listbox.c</file>
	<library>ntdll</library>
	<file>theming.c</file>
	<library>ntdll</library>
	<file>toolbar.c</file>
	<library>ntdll</library>
	<file>tooltips.c</file>
	<library>ntdll</library>
	<file>trackbar.c</file>
	<library>ntdll</library>
	<file>treeview.c</file>
	<library>ntdll</library>
	<file>updown.c</file>
	<library>ntdll</library>
	<file>rsrc.rc</file>
	<file>comctl32.spec</file>
</module>
