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
		??
-->
<property name="SARCH" value="" />


<!--
	Generate instructions for this CPU type. Specify one of:
		??

	See GCC manual for more CPU names.
-->
<property name="OARCH" value="" />

<!--
	Which CPU ReactOS should be optimized for. See GCC manual for CPU names.
-->
<property name="TUNE" value="" />


<!--
	Whether to compile for an uniprocessor or multiprocessor machine.
-->
<property name="MP" value="0" />

<!--
        New style kernel debugger
-->
<property name="_WINKD_" value="0" />

<!--
	Whether to compile in the integrated kernel debugger.
-->
<property name="KDBG" value="0" />


<!--
	Whether to compile for debugging.
-->
<property name="DBG" value="1" />


<!--
	Whether to compile apps/libs with features covered software patents or not.
	If you live in a country where software patents are valid/apply, don't
	enable this (except they/you purchased a license from the patent owner).
	This settings is disabled (0) by default.
-->
<property name="NSWPAT" value="0" />

<!--
	Whether to compile support for ELF files. Do not enable unless you know what
	you're doing.
-->
<property name="_ELF_" value="0" />

</group>
