#include <stdio.h>
#include <ctype.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

/*
 * .idata$2_liba_dll           # liba import directory
 *  __dll_liba_import_directory:
 *   .rva __dll_tail
 *   .long 0
 *   .long 0
 *   .rva __liba_dll_name
 *   .rva __liba_dll_fthunk
 * .idata$2_libb_dll           # libb import directory
 *  __dll_libb_import_directory:
 *   20 bytes ...
 * .idata$3                    # Zero entry
 *  __dll_tail:
 *   20 zeroes ...
 * .idata$4_liba_dll           # Import Lookup Table
 *  __dll_liba_ilt:
 *  .idata$4_liba_fun1_ilt
 *   __dll_liba_fun1_ilt:
 *    .rva __dll_liba_fun1_hint
 *  .idata$4_liba_fun2_ilt
 *   __dll_libb_fun2_ilt:
 *    .rva __dll_liba_fun1_hint
 * .idata$4_libb_dll           # Import Lookup Table
 *  .idata$5_libb_fun1_ilt
 *  .idata$5_libb_fun2_ilt
 * .idata$5_liba_dll           # Import Address Table
 *  .idata$5_liba_fun1_iat
 *   __dll_liba_fun1_ia:
 *    .long 0
 *  .idata$5_liba_fun2_iat     
 * .idata$5_libb_dll           # Import Address Table
 *  .idata$5_libb_fun1_iat
 *  .idata$5_libb_fun2_iat
 * .idata$6_liba_dll           # Hint Table
 *  .idata$6_liba_fun1_hint
 *  .idata$6_liba_fun2_hint
 * .idata$6_libb_dll           # Hint Table
 *  .idata$6_liba_fun1_hint
 *  .idata$6_liba_fun2_hint    
 * .idata$7_liba_dll           # DLL Name
 *  __liba_dll_name:
 *   .asciz "liba.dll"
 * .idata$7_libb_dll           # DLL Name
 *  __libb_dll_name:
 *   .asciz "libb.dll"
 *
 */

std::string uppercase( const std::string &mcase ) {
    std::string out;
    for( std::string::const_iterator i = mcase.begin(); 
	 i != mcase.end(); 
	 i++ ) {
	out += toupper(*i);
    }
    return out;
}

std::string comment_strip( const std::string &line ) {
    size_t s = line.find(';');
    if( s != std::string::npos ) return line.substr(0,s);
    else return line;
}

class DefFile {
public:
    class Export {
    public:
	Export( const std::string &name, const std::string &alias = "",
		int ordinal = -1 ) : 
	    name(name), alias_of(alias), ordinal(ordinal) {}

	const std::string &getName() const { return name; }
	const std::string &getAliasOf() const { return alias_of; }
	int getOrdinal() const { return ordinal; }

    private:
	std::string name;
	std::string alias_of;
	int ordinal;
    };

    DefFile() : ordbase(1), mode(0) { }
    
    static std::string clean_for_symbol( const std::string &s ) {
	std::string out;

	for( size_t i = 0; i < s.size(); i++ ) {
	    out += isalpha(s[i]) ? s[i] : '_';
	}

	return out;
    }

    static std::string section( const std::string &name ) {
	return std::string("\t.section\t") + name + "\n";
    }

    static std::string symbol( const std::string &name ) {
	return name + ":\n";
    }

    static std::string global( const std::string &name ) {
	return std::string("\t.global\t") + name + "\n";
    }

    static std::string global_sym( const std::string &name ) {
	return global(name) + symbol(name);
    }

    static std::string impsym( const std::string &name, size_t i ) {
	std::ostringstream oss;
	oss << name << "_" << i;
	return oss.str();
    }

    std::string archive_deco( const std::string &sym ) {
	return std::string("__dll_archive_") + libsym + "_" + sym;
    }

    static std::string rva( const std::string &symbol ) {
	return std::string("\t.rva\t") + symbol + "\n";
    }
    
    std::string common_name( size_t i, const std::string &_name = "" ) const {
	size_t at;
	std::string name = _name;

	if( !name.size() ) name = exports[i].getName();
	if( !name.size() ) {
	    std::ostringstream oss;
	    oss << libsym << "_ordinal_" << i; 
	    return oss.str(); 
	}
	
	if( name[0] == '@' ) name = name.substr(1);
	at = name.find('@');
	if( at != std::string::npos ) 
	    name = name.substr(0,at);
	
	return name;
    }

    size_t count_names() const {
	size_t res = 0;
	for( size_t i = 0; i < exports.size(); i++ ) 
	    if( exports[i].getName().size() ) ++res;
	return res;
    }

    bool parse( int ln, const std::string &line ) {
	std::string command;
	std::string stripped_line = comment_strip(line);

	std::istringstream iss(stripped_line);

	bool got_word = iss >> command;
       
	if( uppercase(command) == "LIBRARY" ) {
	    if( mode ) {
		fprintf( stderr, "Got library out of turn on line %d\n", ln );
		return false;
	    }
	    mode++;
	    iss >> libname;
	    libsym = clean_for_symbol(libname);
	    import_directory_entry = archive_deco("import_directory_entry");
	    import_directory_term  = archive_deco("import_directory_term");
	    dll_name_text = archive_deco("dll_name");
	    original_first_thunk = archive_deco("ofirst_thk");
	    original_first_thunk_term = original_first_thunk + "_term";
	    first_thunk = archive_deco("first_thk");
	    first_thunk_term = first_thunk + "_term";
	    import_desc_table = archive_deco("import_desc");
	} else if( uppercase(command) == "EXPORTS" ) {
	    if( !mode ) {
		fprintf( stderr, "Got exports out of turn on line %d\n", ln );
		return false;
	    }
	    mode++;
	} else if( got_word ) {
	    size_t equal, at;
	    std::string name, alias, name_alias, ordinal_maybe;
	    int ordinal = -1;

	    if( mode != 2 ) {
		fprintf( stderr, "Got extraneous input on line %d\n", ln );
		return false;
	    }

	    name_alias = command;
	    if( iss >> ordinal_maybe )
		ordinal = atoi(ordinal_maybe.substr(1).c_str());

	    equal = name_alias.find('=');
	    if( equal == std::string::npos ) {
		name = name_alias;
	    } else {
		name = name_alias.substr(0,equal);
		equal++;
		alias = name_alias.substr(equal);
	    }

	    if( kill_at ) {
		at = name.find('@');
		if( at != std::string::npos )
		    name = name.substr(0,at);
		at = alias.find('@');
		if( at != std::string::npos )
		    alias = alias.substr(0,at);
	    }

	    Export ex(name,alias,ordinal);

	    export_byname.insert( std::make_pair(name, exports.size()) );
	    exports.push_back(ex);
	}

	return true;
    }

    std::string makeExportData() const {
	std::ostringstream oss;

	oss << section(".edata")
	    << longdata(0)
	    << longdata(time(NULL))
	    << longdata(0)
	    << rva(libsym)
	    << longdata(ordbase)
	    << longdata(exports.size())
	    << longdata(count_names())
	    << rva("exported_functions")
	    << rva("exported_names")
	    << rva("exported_ordinals")
	    << global_sym(libsym)
	    << asciz(libname)
	    << align(4)
	    << symbol("exported_functions");

	for( size_t i = 0; i < exports.size(); i++ ) {
	    oss << "\t.rva\t" << common_name(i) << "\n";
	}
	
	oss << "exported_ordinals:\n";
	int lastord = ordbase;
	for( size_t i = 0; i < exports.size(); i++ ) {
	    if( exports[i].getOrdinal() != -1 ) 
		lastord = exports[i].getOrdinal();
	    else
		lastord++;
	    oss << "\t.short\t" << lastord << "\n";
	}

	oss << "exported_names:\n";
	
	for( size_t i = 0; i < exports.size(); i++ ) {
	    if( exports[i].getName().size() ) {
		oss << "\t.rva\texport_name_" << (int)i << "\n";
	    }
	}

	for( size_t i = 0; i < exports.size(); i++ ) {
	    if( exports[i].getName().size() ) {
		oss << "export_name_" << (int)i 
		    << ":\t.asciz\t\"" 
		    << exports[i].getName() << "\"\n";
	    }
	}

	oss << "# End of exports\n";

	return oss.str();
    }

    static std::string align(int n) {
	std::ostringstream oss;
	oss << "\t.align\t" << n << "\n";
	return oss.str();
    }

    static std::string longdata(long l) {
	std::ostringstream oss;
	oss << "\t.long\t" << l << "\n";
	return oss.str();
    }

    static std::string shortdata(int i) {
	std::ostringstream oss;
	oss << "\t.short\t" << i << "\n";
	return oss.str();
    }

    static std::string space(int s) {
	std::ostringstream oss;
	oss << "\t.space\t" << s << "\n";
	return oss.str();
    }

    static std::string asciz(const std::string &s) {
	std::string quote = "\"";
	if(s.size() && s[0] == '\"')
	    quote = "";
	return std::string("\t.asciz\t") + quote + s + quote + "\n";
    }

    std::string makeArchiveHeader() const {
	std::ostringstream oss;
	
	oss << section(".idata$2") 
	    << global_sym(import_directory_entry)
	    << rva(original_first_thunk)
	    << longdata(0)
	    << longdata(0)
	    << rva(dll_name_text)
	    << section(".idata$4")
	    << global_sym(original_first_thunk)
	    << section(".idata$5")
	    << global_sym(first_thunk)
	    << section(".idata$6")
	    << global_sym(import_desc_table);
	
	return oss.str();
    }

    std::string makeArchiveFooter() const {
	std::ostringstream oss;

	oss << section(".idata$3")
	    << global_sym(import_directory_term)
	    << space(20)
	    << global_sym(original_first_thunk_term)
	    << longdata(0)
	    << global_sym(first_thunk_term)
	    << longdata(0)
	    << section(".idata$7")
	    << global_sym(dll_name_text)
	    << asciz(libname);

	return oss.str();
    }

    std::string makePerImportData( size_t i ) const {
	std::ostringstream oss;

	oss << section(".text")
	    << global_sym(common_name(i))
	    << "\taddi\t1,1,16\n"
	    << "\tmflr\t0\n"
	    << "\tstw\t0,0(1)\n"
	    << "\tlis\t0," << impsym(first_thunk,i) << "@ha\n"
	    << "\tori\t0,0," << impsym(first_thunk,i) << "@l\n"
	    << "\tlwz\t0," << (4 * i) << "(0)\n"
	    << "\tmtlr\t0\n"
	    << "\tblrl\n"
	    << "\tlwz\t0,0(1)\n"
	    << "\taddi\t1,1,-16\n"
	    << "\tblr\n"
	    << rva(impsym(original_first_thunk,i))
	    << rva(impsym(import_desc_table,i))
	    << rva(impsym(first_thunk,i))
	    << rva(import_directory_entry)
	    << rva(import_directory_term)
	    << section(".idata$4")
	    << symbol(impsym(original_first_thunk,i))
	    << rva(impsym(import_desc_table,i))
	    << section(".idata$5")
	    << symbol(impsym(import_desc_table,i))
	    << shortdata(exports[i].getOrdinal()) 
	    << asciz(exports[i].getName())
	    << section(".idata$6")
	    << symbol(impsym(first_thunk,i))
	    << rva(impsym(import_desc_table,i));
	
	return oss.str();
    }

    size_t numFunctions() const { return exports.size(); }

    void setKillAt( bool kill ) { kill_at = kill; }

private:
    bool kill_at;
    std::string libname, libsym;
    std::string import_directory_entry, import_directory_term;
    std::string original_first_thunk, first_thunk, dll_name_text;
    std::string original_first_thunk_term, first_thunk_term;
    std::string import_desc_table;
    std::vector<Export> exports;
    std::map<std::string,size_t> export_byname;
    int ordbase, mode;
};

std::string maketemp
( const std::string &prefix,
  const std::string &suffix,
  const std::string &payload ) {
    std::string template_accum = 
	std::string("/tmp/") + prefix + "XXXXXX";
    std::vector<char> storage, duplicate;

    for( size_t i = 0; i < template_accum.size(); i++ ) 
	storage.push_back(template_accum[i]);
    storage.push_back(0);

    duplicate = storage;
    std::string result;
    mktemp(&duplicate[0]);
    result = &duplicate[0];
    result += suffix;
    std::ofstream of;
    of.open(result.c_str());
    of << payload;
    of.close();

    return result;
}

void _check( const std::string &varname, const std::string &varval ) {
    if( !varval.size() ) {
	fprintf( stderr, "You must specify %s\n", varname.c_str() );
	exit(1);
    }
}

#define check(x) _check(#x,x)

bool parseArg(int argc, char **argv, 
	      std::string &argval,
	      const char *short_n, 
	      const char *long_n = 0,
	      const char *extra_n = 0)
{
    std::string argv_i;
    bool got_val = false;

    if( !long_n ) { long_n = short_n; }

    for( int i = 0; i < argc; i++ ) {
	argv_i = argv[i];
	if( argv_i == short_n ) {
	    if( i < argc-1 )
		argval = argv[i+1];
	    got_val = true;
	} else if( argv_i == long_n ) {
	    if( i < argc-1 )
		argval = argv[i+1];
	    got_val = true;
	} else if( argv_i.substr(0,strlen(long_n)+1) == 
		   std::string(long_n) + "=" ) {
	    argval = argv_i.substr(strlen(long_n)+1);
	    got_val = true;
	}
    }

    if( !got_val && extra_n ) 
	return parseArg(argc,argv,argval,short_n,extra_n);

    return got_val;
}

int main( int argc, char **argv ) {
    int i, ln = 0;
    bool dont_delete = false, kill_at = false;
    DefFile f;
    std::string line;
    std::string def_file, exp_file, dll_name, imp_name, 
	as_name = "as", ar_name = "ar cq", foo;
    std::ifstream in_def;

    parseArg(argc,argv,def_file,"-d","--def","--input-def");
    parseArg(argc,argv,exp_file,"-e","--exp","--output-exp");
    parseArg(argc,argv,imp_name,"-l","--lib","--output-lib");
    parseArg(argc,argv,dll_name,"-D","--dll","--dll-name");
    parseArg(argc,argv,as_name, "-S","--as");
    dont_delete = parseArg(argc,argv,foo,"-n");
    kill_at     = parseArg(argc,argv,foo,"-k");

    check(def_file);
    check(imp_name);
    
    int err = 0;
    std::string export_file, archive_header_file, archive_footer_file;
    std::vector<std::string> archive_member_files;
    std::ostringstream make_exp;
    std::ostringstream make_lib, archive_it;

    f.setKillAt(kill_at);

    in_def.open(def_file.c_str());
    while( std::getline(in_def,line) ) 
	if(!f.parse(++ln,line)) {
	    fprintf( stderr, "Ungrammatic def file\n" );
	    goto cleanup;
	}
    in_def.close();
    
    unlink(imp_name.c_str());

    export_file = maketemp("exports",".s",f.makeExportData());
    archive_header_file = maketemp("arch_hdr",".s",f.makeArchiveHeader());
    archive_footer_file = maketemp("arch_end",".s",f.makeArchiveFooter());
    for( size_t i = 0; i < f.numFunctions(); i++ )
	archive_member_files.push_back
	    (maketemp("arch_mem",".s",f.makePerImportData(i)));
    
    if( exp_file.size() ) {
	make_exp << as_name << " -mlittle -o " << exp_file << " " << export_file;
	
	fprintf( stderr, "execute %s\n", make_exp.str().c_str() );

	if( (err = system(make_exp.str().c_str())) ) {
	    fprintf(stderr, "Failed to execute %s\n", make_exp.str().c_str());
	    goto cleanup;
	}
    }

    make_lib << as_name << " -mlittle -o " << archive_header_file << ".o " 
	     << archive_header_file 
	     << " && "
	     << as_name << " -mlittle -o " << archive_footer_file << ".o "
	     << archive_footer_file;

    archive_it << ar_name << " " << imp_name << " " 
	       << archive_header_file << ".o";
    for( size_t i = 0; i < f.numFunctions(); i++ ) {
	make_lib << " && " 
		 << as_name << " -mlittle -o " 
		 << archive_member_files[i] << ".o " 
		 << archive_member_files[i];
	archive_it << " " << archive_member_files[i] << ".o";
    }

    archive_it << " " << archive_footer_file << ".o";
    make_lib << " && " << archive_it.str();

    fprintf( stderr, "execute %s\n", make_lib.str().c_str() );

    if( (err = system(make_lib.str().c_str())) ) {
	fprintf(stderr, "Failed to execute %s\n", make_exp.str().c_str());
	goto cleanup;
    }
    
cleanup:
    if( !dont_delete ) {
	unlink(export_file.c_str());
	unlink(archive_header_file.c_str());
	unlink((archive_header_file + ".o").c_str());
	unlink(archive_footer_file.c_str());
	unlink((archive_footer_file + ".o").c_str());
	for( size_t i = 0; i < f.numFunctions(); i++ ) {
	    unlink(archive_member_files[i].c_str());
	    unlink((archive_member_files[i] + ".o").c_str());
	}
    }

    return err;
}
