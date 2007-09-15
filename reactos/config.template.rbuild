<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "tools/rbuild/project.dtd">
<group>

<!--
	This file is a template used as a starting point for compile-time
	configuration of ReactOS. Make a copy of this file and name it config.rbuild.
	Then change the options in config.rbuild. If you don't have a config.rbuild file,
	then the defaults in this file, config.template.rbuild, will be used instead.

	Boolean options can obtain the values 0 (disabled) or 1 (enabled). String
	options can obtain any value specified in the comment before it.
-->


<!--
	Sub-architecture to build for. Specify one of:
		xbox
-->
<property name="SARCH" value="" />


<!--
	Which CPU ReactOS should be optimized for. Specify one of:
		i486, i586, pentium, pentium2, pentium3, pentium4, athlon-xp, athlon-mp,
		k6-2

	See GCC manual for more CPU names and which CPUs GCC can optimize for.
-->
<property name="OARCH" value="pentium" />


<!--
	OPTIMIZE what level do you want ReactOS to be optimized at 
	this setting does not work if GDB is set
		0 = off
		1 = Normal compiling recommended, is default setting in official build and debug build
	 
		warning : 2,3,4,5  is not tested on ReactOS. Change at own risk. 

		2 = gcc -Oz with -mpreferred-stack-boundary=2
		3 = gcc -O1 with -mpreferred-stack-boundary=2
		4 = gcc -O2 with -mpreferred-stack-boundary=2
		5 = gcc -O3 with -mpreferred-stack-boundary=2
-->
<property name="OPTIMIZE" value="1" />


<!--
	Whether to compile for an uniprocessor or multiprocessor machine.
-->
<property name="MP" value="0" />


<!--
	Whether to compile in the integrated kernel debugger.
-->
<property name="KDBG" value="0" />


<!--
	Whether to compile for debugging. No compiler optimizations will be
	performed.
-->
<property name="DBG" value="1" />


<!--
	Whether to compile for debugging with GDB. If you don't use GDB, don't
	enable this.
-->
<property name="GDB" value="0" />


<!--
	Whether to compile apps/libs with features covered software patents or not.
	If you live in a country where software patents are valid/apply, don't
	enable this (except they/you purchased a license from the patent owner).
	This settings is disabled (0) by default.
-->
<property name="NSWPAT" value="0" />

<!--
	Whether to compile with the KD protocol. This will disable support for KDBG
	as well as rossym and symbol lookups, and allow WinDBG to connect to ReactOS.
	This is currently not fully working, and requires kdcom from Windows 2003 or
	TinyKRNL. Booting into debug mode with this flag enabled will result in a
	failure to enter GUI mode. Do not enable unless you know what you're doing.
-->
<property name="_WINKD_" value="0" />

</group>
