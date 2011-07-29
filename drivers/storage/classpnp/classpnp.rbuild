<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="classpnp" type="kernelmodedriver" installbase="system32/drivers" installname="classpnp.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<importlibrary definition="class.spec" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<library>libcntpr</library>
	<include base="classpnp">../inc</include>
	<define name="CLASS_GLOBAL_BREAK_ON_LOST_IRPS">0</define>
	<define name="CLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB">100</define>
	<define name="CLASS_GLOBAL_USE_DELAYED_RETRY">1</define>
	<define name="CLASS_GLOBAL_BUFFERED_DEBUG_PRINT">0</define>
	<define name="CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE">512</define>
	<define name="CLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS">512</define>
	<group compilerset="gcc">
        <compilerflag>-mrtd</compilerflag>
        <compilerflag>-fno-builtin</compilerflag>
        <compilerflag>-w</compilerflag>
    </group>
	<file>autorun.c</file>
	<file>class.c</file>
	<file>classwmi.c</file>
	<file>create.c</file>
	<file>data.c</file>
	<file>dictlib.c</file>
	<file>lock.c</file>
	<file>power.c</file>
	<file>xferpkt.c</file>
	<file>clntirp.c</file>
	<file>retry.c</file>
	<file>utils.c</file>
	<file>obsolete.c</file>
	<file>debug.c</file>
 	<file>class.rc</file>
</module>
