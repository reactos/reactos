#Midl appends an extra \r an the end of a line ending with once
/^#if defined(__cplusplus) && !defined(CINTERFACE)$/d
/^#else[ 	]*\/\*[		]* C style interface[ 	]*\*\/$/,/^#endif[ 	]*\/\*[		]* C style interface[ 	]*\*\/$/d
s/once[	 ]*.$/once\
/
$a\
#endif /* !defined(__cplusplus) || defined(CINTERFACE)\ */
