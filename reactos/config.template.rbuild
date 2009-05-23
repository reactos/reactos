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
	Generate instructions for this CPU type. Specify one of:
		native, i386, i486, pentium, pentium-mmx, pentiumpro, i686,
		pentium2, pentium3, pentium-m, pentium4, prescott, nocona,
		core2, k6, k6-2, athlon, athlon-xp, opteron, opteron-sse3,
		barcelona, winchip-c6, winchip2, c3, c3-2, geode

	See GCC manual for more CPU names.
-->
<property name="OARCH" value="pentium" />


<!--
	Which CPU ReactOS should be optimized for. Specify one of the above
	CPUs or generic. When this option is not used, GCC will optimize for
	the processor specified by OARCH.
-->
<property name="TUNE" value="i686" />


<!--
	What level of optimisation to use.
		0 = off
		1 = Default option, optimize for size (-Os) with some additional options
		2 = -Os
		3 = -O1
		4 = -O2
		5 = -O3
-->
<property name="OPTIMIZE" value="1" />


<!--
	Whether to compile in the integrated kernel debugger.
-->
<property name="KDBG" value="1" />


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

<!--
	Whether to compile support for ELF files. Do not enable unless you know what
	you're doing.
-->
<property name="_ELF_" value="0" />

<!--
	Whether to compile the multi processor versions for ntoskrnl and hal.
-->
<property name="BUILD_MP" value="1" />

</group>
