<module name="string" type="staticlibrary">
	<define name="__NO_CTYPE_INLINES" />
	<define name="_CTYPE_DISABLE_MACROS" />
	<define name="_NO_INLINING" />
	<define name="_NTSYSTEM_" />
	<define name="_NTDLLBUILD_" />

	<!--	__MINGW_IMPORT needs to be defined differently because it's defined
		as dllimport by default, which is invalid from GCC 4.1.0 on!	-->
	<define name="__MINGW_IMPORT">"extern __attribute__ ((dllexport))"</define>

	<if property="ARCH" value="i386">
		<directory name="i386">
			<file>memchr_asm.s</file>
			<file>memcpy_asm.s</file>
			<file>memmove_asm.s</file>
			<file>memset_asm.s</file>
			<file>strcat_asm.s</file>
			<file>strchr_asm.s</file>
			<file>strcmp_asm.s</file>
			<file>strcpy_asm.s</file>
			<file>strlen_asm.s</file>
			<file>strncat_asm.s</file>
			<file>strncmp_asm.s</file>
			<file>strncpy_asm.s</file>
			<file>strnlen_asm.s</file>
			<file>strrchr_asm.s</file>
			<file>wcscat_asm.s</file>
			<file>wcschr_asm.s</file>
			<file>wcscmp_asm.s</file>
			<file>wcscpy_asm.s</file>
			<file>wcslen_asm.s</file>
			<file>wcsncat_asm.s</file>
			<file>wcsncmp_asm.s</file>
			<file>wcsncpy_asm.s</file>
			<file>wcsnlen_asm.s</file>
			<file>wcsrchr_asm.s</file>
		</directory>
	</if>
	<ifnot property="ARCH" value="i386">
		<file>memchr.c</file>
		<file>memcpy.c</file>
		<file>memmove.c</file>
		<file>memset.c</file>
		<file>strcat.c</file>
		<file>strchr.c</file>
		<file>strcmp.c</file>
		<file>strcpy.c</file>
		<file>strlen.c</file>
		<file>strncat.c</file>
		<file>strncmp.c</file>
		<file>strncpy.c</file>
		<file>strnlen.c</file>
		<file>strrchr.c</file>
		<file>wcscat.c</file>
		<file>wcschr.c</file>
		<file>wcscmp.c</file>
		<file>wcscpy.c</file>
		<file>wcslen.c</file>
		<file>wcsncat.c</file>
		<file>wcsncmp.c</file>
		<file>wcsncpy.c</file>
		<file>wcsnlen.c</file>
		<file>wcsrchr.c</file>
	</ifnot>
<!--
	FIXME:
	The next files should be a part of ARCH=i386 and ARCH=unknown. 
	The current implemention of rbuild generates a dependency rule 
	for each occurence of a file. 
-->
	<file>ctype.c</file>
	<file>memccpy.c</file>
	<file>memcmp.c</file>
	<file>memicmp.c</file>
	<file>strcspn.c</file>
	<file>stricmp.c</file>
	<file>strnicmp.c</file>
	<file>strlwr.c</file>
	<file>strrev.c</file>
	<file>strset.c</file>
	<file>strstr.c</file>
	<file>strupr.c</file>
	<file>strpbrk.c</file>
	<file>strspn.c</file>
	<file>wstring.c</file>
	<file>wcsrev.c</file>
	<file>wcsnset.c</file>
	<file>abs.c</file>
	<file>atoi64.c</file>
	<file>atoi.c</file>
	<file>atol.c</file>
	<file>bsearch.c</file>
	<file>itoa.c</file>
	<file>itow.c</file>
	<file>labs.c</file>
	<file>lfind.c</file>
	<file>mbstowcs.c</file>
	<file>splitp.c</file>
	<file>strtol.c</file>
	<file>strtoul.c</file>
	<file>wcstol.c</file>
	<file>wcstombs.c</file>
	<file>wcstoul.c</file>
	<file>wtoi64.c</file>
	<file>wtoi.c</file>
	<file>wtol.c</file>
	<file>rand.c</file>
	<file>sscanf.c</file>
</module>
