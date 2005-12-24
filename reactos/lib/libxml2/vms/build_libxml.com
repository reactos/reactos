$! BUILD_LIBXML.COM
$!
$! Build the LIBXML library
$!
$! Arguments:
$!
$!   	"DEBUG"  - build everything in debug
$!
$! This procedure creates an object library XML_LIBDIR:LIBXML.OLB directory.  
$! After the library is built, you can link LIBXML routines into
$! your code with the command  
$!
$!	$ LINK your_modules,XML_LIBDIR:LIBXML.OLB/LIBRARY
$! 
$! Change History
$! --------------
$! Command file author : John A Fotheringham (jaf@jafsoft.com)
$! Update history      : 13 October 2003	Craig Berry (craigberry@mac.com)
$!			 more new module additions
$!                     : 25 April 2003		Craig Berry (craigberry@mac.com)
$!			 added xmlreader.c and relaxng.c to source list
$! 		       : 28 September 2002	Craig Berry (craigberry@mac.com)
$!			 updated to work with current sources
$!			 miscellaneous enhancements to build process
$!
$!- configuration -------------------------------------------------------------
$!
$!- compile command.  If p1="nowarn" suppress the expected warning types
$!
$   cc_opts = "/NAMES=(SHORTENED)/FLOAT=IEEE/IEEE_MODE=DENORM_RESULTS"
$!
$   if p1.eqs."DEBUG" .or. p2.eqs."DEBUG"
$   then
$     debug = "Y"
$     cc_command = "CC''cc_opts'/DEBUG/NOOPTIMIZE/LIST/SHOW=ALL"
$   else
$     debug = "N"
$     cc_command = "CC''cc_opts'"
$   endif
$!
$!- list of sources to be built into the LIBXML library.  Compare this list
$!  to the definition of "libxml2_la_SOURCES" in the file MAKEFILE.IN.
$!  Currently this definition includes the list WITH_TRIO_SOURCES_TRUE
$!
$   sources = "SAX.c entities.c encoding.c error.c parserInternals.c"
$   sources = sources + " parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c"
$   sources = sources + " valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c"
$   sources = sources + " xpointer.c xinclude.c nanohttp.c nanoftp.c DOCBparser.c"
$   sources = sources + " catalog.c globals.c threads.c c14n.c xmlstring.c"
$   sources = sources + " xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c"
$   sources = sources + " triostr.c trio.c xmlreader.c relaxng.c dict.c SAX2.c"
$   sources = sources + " xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c"
$!
$!- list of main modules to compile and link.  Compare this list to the
$!  definition of bin_PROGRAMS in MAKEFILE.IN
$!
$   bin_progs = "xmllint xmlcatalog"
$!
$!- list of test modules to compile and link.  Compare this list to the
$!  definition of noinst_PROGRAMS in MAKEFILE.
$!
$   noinst_PROGRAMS = "testSchemas testRelax testSAX testHTML testXPath testURI " -
                + "testThreads testC14N testAutomata testRegexp testReader"
$!
$!- set up build logicals -----------------------------------------------------\
$!
$!
$!- start from where the procedure is in case it's submitted in batch ----------\
$!
$   whoami = f$parse(f$environment("PROCEDURE"),,,,"NO_CONCEAL")
$   procdir = f$parse(whoami,,,"DEVICE") + f$parse(whoami,,,"DIRECTORY")
$   set default 'procdir'
$!
$   if f$trnlnm("XML_LIBDIR").eqs.""
$   then
$     if f$search("[-]lib.dir") .eqs. ""
$     then
$       create/directory/log [-.lib]
$     endif
$     xml_libdir = f$parse("[-.lib]",,,"DEVICE") + f$parse("[-.lib]",,,"DIRECTORY")
$     define/process XML_LIBDIR 'xml_libdir'
$     write sys$output "Defining XML_LIBDIR as """ + f$trnlnm("XML_LIBDIR") + """
$   endif
$!
$   if f$trnlnm("XML_SRCDIR").eqs.""
$   then
$     globfile = f$search("[-...]globals.c")
$     if globfile.eqs.""
$     then
$	write sys$output "Can't locate globals.c.  You need to manually define a XML_SRCDIR logical"
$	exit
$     else
$	srcdir = f$parse(globfile,,,"DEVICE") + f$parse(globfile,,,"DIRECTORY")
$	define/process XML_SRCDIR "''srcdir'"
$       write sys$output "Defining XML_SRCDIR as ""''srcdir'"""
$     endif
$   endif
$!
$   copy/log config.vms xml_srcdir:config.h
$!
$   if f$trnlnm("libxml").eqs."" 
$   then 
$     globfile = f$search("[-...]globals.h")
$     if globfile.eqs.""
$     then
$	write sys$output "Can't locate globals.h.  You need to manually define a LIBXML logical"
$	exit
$     else
$	includedir = f$parse(globfile,,,"DEVICE") + f$parse(globfile,,,"DIRECTORY")
$	define/process libxml "''includedir'"
$       write sys$output "Defining libxml as ""''includedir'"""
$     endif
$   endif
$!
$!- set up error handling (such as it is) -------------------------------------
$!
$ exit_status = 1
$ saved_default = f$environment("default")
$ on error then goto ERROR_OUT 
$ on control_y then goto ERROR_OUT 
$!
$!- move to the source directory and create any necessary subdirs and the 
$!  object library
$!
$ set default xml_srcdir
$ if f$search("DEBUG.DIR").eqs."" then create/dir [.DEBUG]
$ if f$search("XML_LIBDIR:LIBXML.OLB").eqs."" 
$ then 
$   write sys$output "Creating new object library XML_LIBDIR:LIBXML.OLB"
$   library/create XML_LIBDIR:LIBXML.OLB
$ endif
$!
$ goto start_here
$ start_here:	  ! move this line to debug/rerun parts of this command file
$!
$!- compile modules into the library ------------------------------------------
$!
$ lib_command   = "LIBRARY/REPLACE/LOG XML_LIBDIR:LIBXML.OLB"
$ link_command	= ""
$!
$ write sys$output ""
$ write sys$output "Building modules into the LIBXML object library"
$ write sys$output ""
$!
$ s_no = 0
$ sources = f$edit(sources,"COMPRESS")
$!
$ source_loop:
$!
$   next_source = f$element (S_no," ",sources)
$   if next_source.nes."" .and. next_source.nes." "
$   then
$!
$     on error then goto ERROR_OUT 
$     on control_y then goto ERROR_OUT 
$     call build 'next_source'
$     s_no = s_no + 1
$     goto source_loop
$!
$   endif
$!
$!- now build self-test programs ----------------------------------------------
$!
$! these programs are built as ordinary modules into XML_LIBDIR:LIBXML.OLB.  Here they
$! are built a second time with /DEFINE=(STANDALONE) in which case a main()
$! is also compiled into the module
$ 
$ lib_command	= ""
$ link_command	= "LINK"
$!
$ library/compress XML_LIBDIR:LIBXML.OLB
$ purge XML_LIBDIR:LIBXML.OLB
$!
$ write sys$output ""
$ write sys$output "Building STANDALONE self-test programs"
$ write sys$output ""
$!
$ call build NANOFTP.C	/DEFINE=(STANDALONE)
$ call build NANOHTTP.C	/DEFINE=(STANDALONE)
$ call build TRIONAN.C	/DEFINE=(STANDALONE)
$!
$!- now build main and test programs ------------------------------------------
$!
$!
$ lib_command	= ""
$ link_command	= "LINK"
$!
$ write sys$output ""
$ write sys$output "Building main programs and test programs"
$ write sys$output ""
$!
$ p_no = 0
$ all_progs = bin_progs + " " + noinst_PROGRAMS
$ all_progs = f$edit(all_progs,"COMPRESS")
$!
$ prog_loop:
$!
$   next_prog = f$element (p_no," ",all_progs)
$   if next_prog.nes."" .and. next_prog.nes." "
$   then
$!
$     on error then goto ERROR_OUT 
$     on control_y then goto ERROR_OUT 
$     call build 'next_prog'.c
$     p_no = p_no + 1
$     goto prog_loop
$!
$   endif
$!
$!- Th-th-th-th-th-that's all folks! ------------------------------------------
$!
$ goto exit_here ! move this line to avoid parts of this command file
$ exit_here:	  
$!
$ exit       
$ goto exit_out
$!
$!
$EXIT_OUT:
$!
$ purge/nolog [.debug]
$ set default 'saved_default
$ exit 'exit_status
$!
$
$ERROR_OUT:
$ exit_status = $status
$ write sys$output "''f$message(exit_status)'"
$ goto EXIT_OUT
$!
$!- the BUILD subroutine.  Compile then insert into library or link as required
$!
$BUILD: subroutine
$   on warning then goto EXIT_BUILD
$   source_file = p1
$   name = f$parse(source_file,,,"NAME")
$   object_file = f$parse("[.debug].OBJ",name,,,"SYNTAX_ONLY")
$!
$!- compile
$!
$   write sys$output "''cc_command'''p2'/object=''object_file' ''source_file'"
$   cc_command'p2' /object='object_file 'source_file'
$!
$!- insert into library if command defined
$!
$   if lib_command.nes.""  then lib_command 'object_file'
$!
$!- link module if command defined
$   if link_command.nes."" 
$   then
$	opts = ""
$	if debug then opts = "/DEBUG"
$	write sys$output "''link_command'''opts' ''object_file',XML_LIBDIR:libxml.olb/library"
$	link_command'opts' 'object_file',-
      		XML_LIBDIR:libxml.olb/library
$   endif
$!
$EXIT_BUILD:
$   exit $status
$!
$endsubroutine
