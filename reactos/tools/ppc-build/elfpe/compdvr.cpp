#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <list>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "objectfile.h"
#include "header.h"
#include "fork_execvp.h"

#define TYPE_PEHEADER (SHT_LOOS + 1)

/* tool for transforming gcc -### output and using it to drive the compiler
 * elements ourselves.  Most importantly, we need to know what file gcc asks
 * the linker to input and output, and add an intermediate stage with alink.
 */

typedef enum { empty, found_q, found_bs } break_t;

template <class T>
T break_arguments( const std::string &line ) {
    break_t state = empty;
    T args;
    std::string arg;

    for( size_t i = 0; i < line.size(); i++ ) {
	switch( state ) {
	case empty:
	    if( line[i] == '\"' ) {
		state = found_q;
	    }
	    break;

	case found_q:
	    if( line[i] == '\\' ) state = found_bs;
	    else if( line[i] == '\"' ) {
		state = empty;
		args.push_back(arg);
		arg = "";
	    } else arg += line[i];
	    break;

	case found_bs:
	    state = found_q;
	    arg += line[i];
	    break;
	}
    }

    return args;
}

template <class T>
std::vector<char *> get_arg_ptrs
( const T &arg_strings ) {
    std::vector<char *> arg_out;
    for( size_t i = 0; i < arg_strings.size(); i++ ) 
	arg_out.push_back( (char *)arg_strings[i].c_str() );
    arg_out.push_back(0);
    return arg_out;
}

int execute_command( bool verbose, const std::vector<std::string> &args ) {
    std::string error;
    std::vector<char *> args_ptrs;
    const char *tag = "executable";

    if( verbose ) {
	fprintf( stderr, "<command>\n" );
	for( size_t i = 0; i < args.size(); i++ ) {
	    fprintf( stderr, "<%s>%s</%s>\n", tag, args[i].c_str(), tag );
	    tag = "argument";
	}
    }

    Process p = fork_execvp( args );
    
    while( p && p->ProcessStarted() && !p->EndOfStream() )
	fprintf( stderr, "%s", p->ReadStdError().c_str() );

    if( verbose ) {
	fprintf( stderr, "<status>%d</status>\n", p->GetStatus() );
	fprintf( stderr, "</command>\n" );
    }

    return p ? p->GetStatus() : -1;
}

std::string make_tmp_name() {
    int fd;
    std::string name;

    while( true ) {
	name = tmpnam(NULL);
	name += ".obj";
	if( (fd = creat( name.c_str(), 0644 )) != -1 ) {
	    close(fd);
	    return name;
	}
    }
}

void recognize_arg( std::vector<std::string> &args,
		    std::string &result,
		    size_t &i, 
		    const std::string &argname,
		    const std::string &short_argname = "" ) {
    if( (short_argname.size() && (args[i] == short_argname)) || 
	(args[i] == argname) ) {
	result = args[i+1];
	args.erase(args.begin()+i);
	args.erase(args.begin()+i--);
    } else if( args[i].substr(0,argname.size()+1) == argname + "=" ) {
	result = args[i].substr(argname.size()+1);
	args.erase(args.begin()+i--);
    }
}

int run_ld( bool verbose, bool nostdlib, bool nostartfiles, bool is_dll,
	    bool make_map,
	    const std::string &lib_dir,
	    const std::string &ldscript,
	    const std::vector<std::string> &arg_vect ) {
    bool use_libgcc = false;
    std::vector<std::string> args = arg_vect;
    std::string real_output,
	entry_point = "_main", 
	image_base = "0x400000", subsystem = "windows", 
	make_dll, 
	file_align = "0x1000", 
	section_align = "0x1000", base_file;
    const ElfObjectFile::Symbol *entry_sym;
    std::vector<std::string>::iterator i =
	std::find(args.begin(),args.end(),"-lgcc");

#if 0
    if( i != args.end() ) {
	args.erase( i );
	use_libgcc = true;
    }
#else
    use_libgcc = true;
#endif

    if( !nostartfiles ) {
	if( make_dll.size() ) 
	    args.push_back(lib_dir + "/dllcrt2.o");
	else
	    args.push_back(lib_dir + "/crt2.o");
    }

    if( !nostdlib ) {
	args.push_back(std::string("-L") + lib_dir);
	args.push_back("-lmsvcrt");
	args.push_back("-lcrtdll");
	args.push_back("-lmingw32");
	args.push_back("-lkernel32");
    }
    if( use_libgcc )
	args.push_back(lib_dir + "/libgcc.a");

    if( verbose ) 
      args.insert(args.begin()+1,"-v");
    
    args.insert(args.begin()+1,"-T");
    args.insert(args.begin()+2,ldscript);
    args.insert(args.begin()+1,"--emit-relocs");
    args.insert(args.begin()+1,"-dc");
    args.insert(args.begin()+1,"-r");
    
    for( size_t i = 0; i < args.size(); i++ ) {
	if( args[i] == "-o" && i < args.size()-1 ) {
	    real_output = args[++i];
	} else if( args[i].substr(0,4) == "-mdll" ) {
	    args.erase(args.begin()+i--);
	    make_dll = "-dll";
	}

	recognize_arg( args, entry_point, i, "--entry", "-e" );
	recognize_arg( args, image_base, i, "--image-base" );
	recognize_arg( args, subsystem, i, "--subsystem" );
	recognize_arg( args, file_align, i, "--file-alignment" );
	recognize_arg( args, section_align, i, "--section-alignment" );
	recognize_arg( args, base_file, i, "--base-file" );
    }

    if( execute_command( verbose, args ) )
	return 1;

    if( base_file.size() ) {
	FILE *f = fopen(base_file.c_str(), "wb");
	if( !f ) { 
	    fprintf(stderr, "<error>\n");
	    perror(base_file.c_str());
	    fprintf(stderr, "</error>\n");
	    return 1;
	}
	fclose(f);
    }

    /* Ok fixup the elf object file */
    ElfObjectFile eof(real_output);
    
    if(!eof) return 1;

    entry_sym = eof.getNamedSymbol(entry_point);

    ElfPeHeader header
	(strtoul(image_base.c_str(), 0, 0),
	 strtoul(section_align.c_str(), 0, 0),
	 strtoul(file_align.c_str(), 0, 0),
	 entry_sym,
	 0x10000,
	 0x100000,
	 0x10000,
	 0x100000,
	 atoi(subsystem.c_str()),
	 is_dll,
	 &eof);
    
    eof.addSection(".peheader", header.getData(), TYPE_PEHEADER);

    return 0;
}

int main( int argc, char **argv ) {
    bool verbose = false, ld_mode = false, nostdlib = false, 
	nostartfiles = false, is_dll = false, make_map = false;
    int err_fd[2], read_len, child_pid_gcc, child_pid_command, status = 0;
    std::string gcc_name, gcc_hash_output, gcc_line, linker_name = "ld",
	    mingw_lib_dir, ldscript;
    std::vector<std::string> gcc_args_str,
	subcmd_args;
    std::vector<char *> arguments_for_gcc;
    char buf[1024];

    for( int i = 1; i < argc; i++ ) {
	if( std::string("-gcc-name") == argv[i] && i < argc-1 ) {
	    gcc_name = argv[++i];
	} else if( std::string("-ldscript") == argv[i] && i < argc-1 ) {
	    ldscript = argv[++i];
	} else if( std::string("-link-stage-name") == argv[i] && i < argc-1 ) {
	    linker_name = argv[++i];
	} else if( std::string("-mingw-lib-dir") == argv[i] && i < argc-1 ) {
	    mingw_lib_dir = argv[++i];
	} else if( std::string("-v") == argv[i] ) {
	    verbose = true;
	} else if( std::string("-pipe") == argv[i] ) {
	    /* ignore */
	} else if( std::string("-T") == argv[i] ) {
	    /* ignore */
	    i++;
	} else if( std::string("-nostdlib") == argv[i] ) {
	    nostdlib = true;
	} else if( std::string("-nostartfiles") == argv[i] ) {
	    nostartfiles = true;
	} else if( std::string("-shared") == argv[i] ) {
	    is_dll = true;
	} else if( std::string("-map") == argv[i] ) {
	    make_map = true;
	} else {
	    gcc_args_str.push_back(argv[i]);
	}
    }

    /* We never use the system start files or standard libs */
    gcc_args_str.insert(gcc_args_str.begin()+1,"-nostdlib");
    gcc_args_str.insert(gcc_args_str.begin()+1,"-nostartfiles");

    if( std::string(argv[0]).find("ld") != std::string::npos ) {
	gcc_args_str.insert
	    ( gcc_args_str.begin(), linker_name );
	return run_ld
	    ( verbose, nostdlib, nostartfiles, is_dll, make_map, mingw_lib_dir, 
	      ldscript, gcc_args_str );
    }
    if( verbose ) fprintf( stderr, "<compiler-driver>\n" );

    // Stack on driver name and dump commands flag
    gcc_args_str.insert(gcc_args_str.begin(),std::string("-###"));
    gcc_args_str.insert(gcc_args_str.begin(),gcc_name);

    /* Redirect stderr to our pipe */
    if( verbose ) {
	const char *tag = "executable";
	fprintf( stderr, "<gcc>\n" );
	for( size_t i = 0; i < gcc_args_str.size(); i++ ) {
	    fprintf( stderr, " <%s>%s</%s>\n",
		     tag, gcc_args_str[i].c_str(),
		     tag );
	    tag = "arg";
	}
	fprintf( stderr, "</gcc>\n" );
    }

    Process p = fork_execvp( gcc_args_str );
    
    while( p && p->ProcessStarted() && !p->EndOfStream() )
	gcc_hash_output += p->ReadStdError();

    std::istringstream iss( gcc_hash_output );
    
    if( p->GetStatus() ) goto final;

    while( std::getline( iss, gcc_line, '\n' ) ) {
	// command line
	if( gcc_line.size() > 2 && gcc_line[0] == ' ' ) {
	    subcmd_args = 
		break_arguments<std::vector<std::string> >( gcc_line );
	    
	    if( subcmd_args.size() < 1 ) continue;
	    
	    if( subcmd_args[0].find("collect2") != std::string::npos ||
		subcmd_args[0].find("ld") != std::string::npos ) {
		if( status = 
		    run_ld( verbose, nostdlib, nostartfiles, is_dll, make_map,
			    mingw_lib_dir, ldscript, subcmd_args ) )
		    goto final;
		else
		    continue;
	    }
	    
	    if( verbose ) 
		subcmd_args.insert(subcmd_args.begin()+1,"-v");
	    
	    if( status = execute_command
		( verbose, subcmd_args ) )
		goto final;
	} else if( verbose ) 
	    fprintf( stderr, "<error>%s</error>\n", gcc_line.c_str() );
    }
    goto theend;

 final:
 theend:
    if( verbose ) {
	fprintf( stderr, "<status>%d</status>\n", status );
	fprintf( stderr, "</compiler-driver>\n" );
    }
    return status;
}
