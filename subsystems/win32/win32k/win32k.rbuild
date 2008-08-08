<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="win32k" type="kernelmodedriver" installbase="system32" installname="win32k.sys">
	<importlibrary definition="win32k.def" />
	<define name="_WIN32K_" />
	<include base="win32k">include</include>
	<include base="win32k" root="intermediate"></include>
	<include base="win32k" root="intermediate">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>freetype</library>
	<library>libcntpr</library>
	<library>pseh</library>
	<directory name="include">
		<pch>win32k.h</pch>
	</directory>
	<directory name="eng">
		<file>engblt.c</file>
		<file>engbrush.c</file>
		<file>engclip.c</file>
		<file>engdev.c</file>
		<file>engdrv.c</file>
		<file>engevent.c</file>
		<file>engerror.c</file>
		<file>engfile.c</file>
		<file>engfloat.c</file>
		<file>engfont.c</file>
		<file>engmem.c</file>
		<file>engmisc.c</file>
		<file>engpaint.c</file>
		<file>engpal.c</file>
		<file>engpath.c</file>
		<file>engpoint.c</file>
		<file>engprint.c</file>
		<file>engquery.c</file>
		<file>engrtl.c</file>
		<file>engsem.c</file>
		<file>engsurf.c</file>
		<file>engtext.c</file>
		<file>engwnd.c</file>
		<file>engxform.c</file>
		<file>engxlate.c</file>
	</directory>
	<directory name="gre">
		<file>init.c</file>
	</directory>
	<directory name="ntddraw">
		<file>d3d.c</file>
		<file>dd.c</file>
		<file>ddeng.c</file>
		<file>ddsurf.c</file>
		<file>dvp.c</file>
		<file>mocomp.c</file>
	</directory>
	<directory name="ntgdi">
		<file>gdiblt.c</file>
		<file>gdibmp.c</file>
		<file>gdibrush.c</file>
		<file>gdiclip.c</file>
		<file>gdicolor.c</file>
		<file>gdicon.c</file>
		<file>gdicoord.c</file>
		<file>gdidc.c</file>
		<file>gdieng.c</file>
		<file>gdifill.c</file>
		<file>gdifont.c</file>
		<file>gdiinit.c</file>
		<file>gdiline.c</file>
		<file>gdimeta.c</file>
		<file>gdipal.c</file>
		<file>gdipath.c</file>
		<file>gdipen.c</file>
		<file>gdiprint.c</file>
		<file>gdirect.c</file>
		<file>gdirgn.c</file>
		<file>gdistock.c</file>
		<file>gdistrm.c</file>
		<file>gditext.c</file>
		<file>gdiwingl.c</file>
		<file>gdixfer.c</file>
	</directory>
	<directory name="ntuser">
		<file>accel.c</file>
		<file>caret.c</file>
		<file>class.c</file>
		<file>clipboard.c</file>
		<file>colors.c</file>
		<file>cursor.c</file>
		<file>dde.c</file>
		<file>desktop.c</file>
		<file>display.c</file>
		<file>event.c</file>
		<file>hooks.c</file>
		<file>icon.c</file>
		<file>ime.c</file>
		<file>input.c</file>
		<file>keyboard.c</file>
		<file>menu.c</file>
		<file>message.c</file>
		<file>misc.c</file>
		<file>mouse.c</file>
		<file>objects.c</file>
		<file>painting.c</file>
		<file>process.c</file>
		<file>remote.c</file>
		<file>scroll.c</file>
		<file>simplecall.c</file>
		<file>thread.c</file>
		<file>timer.c</file>
		<file>userinit.c</file>
		<file>window.c</file>
		<file>winsta.c</file>
	</directory>
	<file>win32k.rc</file>
</module>
</group>