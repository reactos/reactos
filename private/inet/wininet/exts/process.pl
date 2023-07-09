#####################################################################
#
#
#
# Perl script to process structures/classes and output
# a debugger extension function
#
#	Usage:
#		perl process.pl < <header-file>
#
#	Output:
#		Code to dump structures/classes contained in header files (dumped to stdout)
#
#		Function prototypes for the created functions in "prototypes.txt"
#
#	Gotchas:
#		Works good when the header file does not contain embedded structure 
#		definitions inside class/struct declarations. All bets are off if this 
#		occurs
#
#	Change History
#
#	01-26-97:
#		Feroze Daud ( ferozed ) created
#
#
#
#
#####################################################################
$|=1;
$debug = 0;
$full_code = 1;

%typemap = (
		DWORD, "%d", 
		VOID, "", 
		LPVOID, "0X%X", 
		int, "%d", 
		char, "%c", 
		LPSTR, "%s", 
		LPTSTR, "%s", 
		HINTERNET, "0X%X", 
		BOOL, "%d", 
		ICSTRING, "STRUCT",
		XSTRING, "STRUCT",
		PROXY_INFO, "STRUCT",
		RESOURCE_LOCK, "STRUCT",
		LONG, "%d", 
		CSOCKET, "STRUCT",
		ICSocket, "STRUCT",
		INTERNET_PORT, "%d", 
		INTERNET_SCHEME,"%d", 
		CServerInfo, "STRUCT",
		HEADER_STRING, "STRUCT",
		CHUNK_TRANSFER, "STRUCT",
		HTTPREQ_STATE, "%d", 
		HTTP_HEADERS, "0X%X", 
		HTTP_METHOD_TYPE, "%d", 
		LPBYTE, "0X%X", 
		FILETIME, "%d", 
		LPCACHE_ENTRY_INFO, "STRUCT",
		AUTHCTX, "STRUCT",
		SECURITY_CACHE_LIST_ENTRY, "STRUCT",
		RESOURCE_LOCK, "STRUCT",
		RESOURCE_INFO,"STRUCT",
		LIST_ENTRY, "STRUCT",
		SERIALIZED_LIST, "STRUCT",
		PROXY_SERVER_LIST,"STRUCT",
		PROXY_SERVER_LIST_ENTRY,"STRUCT",
		PROXY_BYPASS_LIST, "STRUCT",
		PROXY_BYPASS_LIST_ENTRY,"STRUCT",
		BAD_PROXY_LIST,"STRUCT",
		BAD_PROXY_LIST_ENTRY,"STRUCT",
		PROXY_INFO, "STRUCT"	
);



open( PROTOTYPES, ">prototypes.txt" ) || die ("Couldnt open prototypes file\n");

while(<STDIN>) {

#////////////////////////
#	Skip comments
#////////////////////////

	if(/^\s*\/\/.*/) {
		next;
	}


#////////////////////////
#	Skip friends and forward class declarations
#////////////////////////

	if(
			/class\s+\w+\s*;/
		||	/friend/
	) {
		next;
	}

	
#////////////////////////
#	Seeing a class or struct typedef
#////////////////////////

	if( 
		/(class)\s+(\w+)\s+\{/ 
	||	/\s+(struct)\s+(\w+)\{/
	||	/(typedef\s+struct)\s+(\w+)\{/
	) {
#		print "<<$2>>\n";
#		print;
		$class = 1;
		$type = $1;
		$name = $2;

		if( $full_code == 1 ) {
			&print_code_header($1,$2);

			print PROTOTYPES "DECLARE_API($name);\n";
			print PROTOTYPES "void do_$name( DWORD addr ); \n";
		}
		next;
	}

#////////////////////////
#	Print all #ifdef/#endif
#////////////////////////
	if(
		/#ifdef/
	||	/#endif/
	||	/#if/
	) {
		print;
		next;
	}

#	If not inside a class or a struct, skip to next line
	if( $class == 0 ) {
		next;
	}

#	Seeing end of a class/struct
	if( /\s*\}.*;/ ) {
		&end_code($name);
		print "\n\n";
#		print //;
#		print;
#		print "//$name";
#		print "//------------------------------------\n";
		$class = 0;
		next;
	}

#	Skip all member/non-member functions
	if( /\(.*\)/ ) {
		next;
	}

#	print data types
	if( 
		/DWORD/
	||	/VOID/
	||	/LPVOID/
	||	/int/
	||	/char/
	||	/LPSTR/
	||	/LPTSTR/
	||	/HINTERNET/
	||	/BOOL/
	||	/ICSTRING/
	||	/XSTRING/
	||	/PROXY_INFO/
	||	/RESOURCE_LOCK/
	||	/LONG/
	||	/CSOCKET/
	||	/ICSocket/
	||	/INTERNET_PORT/
	||	/INTERNET_SCHEME/
	||	/CServerInfo/
	||	/HEADER_STRING/
	||	/CHUNK_TRANSFER/
	||	/HTTPREQ_STATE/
	||	/HTTP_HEADERS/
	||	/HTTP_METHOD_TYPE/
	||	/LPBYTE/
	||	/FILETIME/
	||	/LPCACHE_ENTRY_INFO/
	||	/AUTHCTX/
	||	/SECURITY_CACHE_LIST_ENTRY/
	||	/RESOURCE_INFO/
	||	/LIST_ENTRY/
	||	/SERIALIZED_LIST/
	||	/PROXY_SERVER_LIST/
	||	/PROXY_SERVER_LIST_ENTRY/
	||	/PROXY_BYPASS_LIST/
	||	/PROXY_BYPASS_LIST_ENTRY/
	||	/BAD_PROXY_LIST/
	||	/BAD_PROXY_LIST_ENTRY/
	||	/PROXY_INFO/	
	) {
		/(\w+)\s*(\*)?\s*(\w+)\s*;/;

		if( $debug == 1) {
			print "$1 - $2 - $3\n";
		}

		if ( 
			( $1 eq "" )
		||	( $3 eq "" )
		) {
			next;
		}

		$fmt = $typemap{$1};
		if( $fmt eq "" ) {
			$fmt = "%d"
		}

		print "\t//$1 $2 $3\n";

		if( $2 eq "*" ) {
			print "\td_printf(\"\\t$1 $2 $3 0x%x\\n\", obj -> $3);\n";
		} else {

			if( $fmt eq "STRUCT" ) {
				print "\td_printf(\"\\t$1 (*) $3 0x%x\\n\", OFFSET( $name, $3) );\n";
			} else {
				print "\td_printf(\"\\t$1 $3 $fmt\\n\", obj -> $3);\n";
			}
		}
		print "\n";
	}

}

close( PROTOTYPES );

exit;


sub print_code_header {

#	
#	$type = "class" | "struct"
#
	$type = shift;

#
#	The name of the structure being dumped
#
	$name = shift;

	print <<EOL;

/////////////////////////////////////////////////
//
//	$name structure
//
/////////////////////////////////////////////////

DECLARE_API( $name )
{
    DWORD dwAddr;

    INIT_DPRINTF();

    dwAddr = d_GetExpression(lpArgumentString);
    if ( !dwAddr )
        {
        return;
        }
   do_$name(dwAddr);
}

VOID
do_$name(
	   DWORD addr
    )
{
	BOOL b;
	char block[ sizeof( $name ) ];

	PROXY_INFO * obj  = ($name *) &block;

    b = GetData(
			addr, 
			block, 
			sizeof( $name ), 
			NULL);

	if ( !b ) {
		d_printf("couldn't read $name at 0x%x; sorry.\\n", addr);
		return;
	}


	d_printf("$name @ 0x%x \\n\\n", addr);

EOL

}

sub end_code {

	$name = shift;

	print <<'EOL';


	d_printf("\n");

EOL
	print "}  // $name\n";
}

