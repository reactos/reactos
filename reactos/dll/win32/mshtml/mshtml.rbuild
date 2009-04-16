<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mshtml" type="win32dll" baseaddress="${BASEADDRESS_MSHTML}" installbase="system32" installname="mshtml.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="mshtml.spec" />
	<include base="mshtml">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<file>conpoint.c</file>
	<file>dispex.c</file>
	<file>editor.c</file>
	<file>hlink.c</file>
	<file>htmlanchor.c</file>
	<file>htmlbody.c</file>
	<file>htmlcomment.c</file>
	<file>htmlcurstyle.c</file>
	<file>htmldoc.c</file>
	<file>htmldoc3.c</file>
	<file>htmldoc5.c</file>
	<file>htmlelem.c</file>
	<file>htmlelem2.c</file>
	<file>htmlelem3.c</file>
	<file>htmlelemcol.c</file>
	<file>htmlevent.c</file>
	<file>htmlgeneric.c</file>
	<file>htmliframe.c</file>
	<file>htmlimg.c</file>
	<file>htmlinput.c</file>
	<file>htmllocation.c</file>
	<file>htmlnode.c</file>
	<file>htmloption.c</file>
	<file>htmlscript.c</file>
	<file>htmlselect.c</file>
	<file>htmlstyle.c</file>
	<file>htmlstyle2.c</file>
	<file>htmlstyle3.c</file>
	<file>htmlstylesheet.c</file>
	<file>htmltable.c</file>
	<file>htmltablerow.c</file>
	<file>htmltextarea.c</file>
	<file>htmltextcont.c</file>
	<file>htmltextnode.c</file>
	<file>htmlwindow.c</file>
	<file>install.c</file>
	<file>loadopts.c</file>
	<file>main.c</file>
	<file>mutation.c</file>
	<file>navigate.c</file>
	<file>nsembed.c</file>
	<file>nsevents.c</file>
	<file>nsio.c</file>
	<file>nsservice.c</file>
	<file>olecmd.c</file>
	<file>oleobj.c</file>
	<file>olewnd.c</file>
	<file>omnavigator.c</file>
	<file>persist.c</file>
	<file>protocol.c</file>
	<file>script.c</file>
	<file>selection.c</file>
	<file>service.c</file>
	<file>task.c</file>
	<file>txtrange.c</file>
	<file>view.c</file>
	<file>rsrc.rc</file>
	<include base="mshtml" root="intermediate">.</include>
	<library>wine</library>
	<library>strmiids</library>
	<library>uuid</library>
	<library>urlmon</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<dependency>mshtml_nsiface_header</dependency>
</module>
<module name="mshtml_nsiface_header" type="idlheader" allowwarnings="true">
	<file>nsiface.idl</file>
</module>
</group>
