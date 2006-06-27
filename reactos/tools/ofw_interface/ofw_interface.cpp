#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class ofw_wrappers {
public:
    int base, ctindex;
    std::string functions;
    std::string names;
    std::string calltable;
    std::string le_stubs;
    std::string of_call;
};

std::string uppercase( const std::string &toucase ) {
    std::vector<char> ucase_work(toucase.size());
    for( size_t i = 0; i < toucase.size(); i++ ) {
	ucase_work[i] = toupper(toucase[i]);
    }
    return std::string(&ucase_work[0], toucase.size());
}

std::string clip_eol( std::string in, const std::string &eol_marks ) {
    size_t found;
    for( size_t i = 0; i < eol_marks.size(); i++ ) {
	found = in.find( eol_marks[i] );
	if( found != std::string::npos ) 
	    in = in.substr( 0, found );
    }
    return in;
}

int round_up( int x, int factor ) {
    return (x + (factor - 1)) & ~(factor - 1);
}

std::string c_type( const std::string &intype ) {
    size_t colon = intype.find(':');
    if( colon != std::string::npos ) return intype.substr(0,colon);
    else if( intype.size() ) return intype;
    else return "void";
}

std::string have_len( const std::string &intype ) {
    size_t colon = intype.find(':');
    if( colon != std::string::npos ) return intype.substr(colon+1);
    else return "";
}

bool need_swap( const std::string &intype ) {
    return intype.find('*') != std::string::npos;
}

void populate_definition( ofw_wrappers &wrapper, const std::string &line ) {
    std::istringstream iss(line);
    bool make_function = true;
    std::string name, argtype, rettype, c_rettype;
    std::vector<std::string> argtypes;
    int args, rets, i, local_offset, total_stack;
    std::ostringstream function, ct_csource, le_stub, of_call;

    iss >> name >> args >> rets;

    if( !name.size() ) return;
    if( name[0] == '-' ) {
	name = name.substr(1);
	make_function = false;
    }

    for( i = 0; i < args; i++ ) {
        iss >> argtype;
	argtypes.push_back(argtype);
    }

    iss >> rettype;

    local_offset = (3 + rets + args) * 4;
    total_stack = round_up(12 + local_offset, 16);

    function << "ofw_" << name << ":\n"
	     << "\t/* Reserve stack space */\n"
	     << "\tsubi %r1,%r1," << total_stack << "\n"
	     << "\t/* Store r8, r9, lr */\n"
	     << "\tstw %r8," << (local_offset + 0) << "(%r1)\n"
	     << "\tstw %r9," << (local_offset + 4) << "(%r1)\n"
	     << "\tmflr %r8\n"
	     << "\tstw %r8," << (local_offset + 8) << "(%r1)\n"
	     << "\t/* Get read name */\n"
	     << "\tlis %r8," << wrapper.base << "@ha\n"
	     << "\taddi %r9,%r8," << name << "_ofw_name - _start\n"
	     << "\tstw %r9,0(%r1)\n"
	     << "\t/* " << args << " arguments and " << rets << " return */\n"
	     << "\tli %r9," << args << "\n"
	     << "\tstw %r9,4(%r1)\n"
	     << "\tli %r9," << rets << "\n"
	     << "\tstw %r9,8(%r1)\n";

    for( int i = 0; i < args; i++ )
	function << "\tstw %r" << (i+3) << "," << (4 * (i + 3)) << "(%r1)\n";
	    
    function << "\t/* Load up the call address */\n"
	     << "\tlwz %r9,ofw_call_addr - _start(%r8)\n"
	     << "\tmtlr %r9\n"
	     << "\t/* Set argument */\n"
	     << "\tmr %r3,%r1\n"
	     << "\t/* Fire */\n"
	     << "\tblrl\n"
	     << "\tlwz %r3," << (local_offset - 4) << "(%r1)\n"
	     << "\t/* Restore registers */\n"
	     << "\tlwz %r8," << (local_offset + 8) << "(%r1)\n"
	     << "\tmtlr %r8\n"
	     << "\tlwz %r9," << (local_offset + 4) << "(%r1)\n"
	     << "\tlwz %r8," << (local_offset + 0) << "(%r1)\n"
	     << "\t/* Return */\n"
	     << "\taddi %r1,%r1," << total_stack << "\n"
	     << "\tblr\n";

    c_rettype = c_type(rettype);

    le_stub << c_rettype << " ofw_" << name << "(";
    for( i = 0; i < args; i++ ) {
	if( i ) le_stub << ",";
	le_stub << c_type(argtypes[i]) << " arg" << i;
    }
    le_stub << ")";
    of_call << le_stub.str() << ";\n";

    le_stub << " {\n";
    if( c_rettype != "void" ) 
	le_stub << "\t" << c_rettype << " ret;\n";

    for( i = 0; i < args; i++ ) {
	if( need_swap(argtypes[i]) ) {
	    std::string len;
	    if( have_len(argtypes[i]).size() ) 
		len = have_len(argtypes[i]);
	    else {
		std::ostringstream oss;
		oss << "strlen(arg" << i << ")";
		len = oss.str();
	    }
	    le_stub << "\tle_swap("
		    << "arg" << i << "," 
		    << "arg" << i << "+" << len << ","
		    << "arg" << i << ");\n";
	}
    }

    le_stub << "\t";
    if( c_rettype != "void" ) le_stub << "ret = (" << c_rettype << ")";

    le_stub << "ofproxy(" << (wrapper.ctindex * 4);
    
    for( i = 0; i < 4; i++ ) {
	if( i < args ) le_stub << ",(void *)arg" << i;
	else le_stub << ",NULL";
    }

    le_stub << ");\n";

    for( i = args-1; i >= 0; i-- ) {
	if( need_swap(argtypes[i]) ) {
	    std::string len;
	    if( have_len(argtypes[i]).size() ) 
		len = have_len(argtypes[i]);
	    else {
		std::ostringstream oss;
		oss << "strlen(arg" << i << ")";
		len = oss.str();
	    }
	    le_stub << "\tle_swap("
		    << "arg" << i << "," 
		    << "arg" << i << "+" << len << ","
		    << "arg" << i << ");\n";
	}
    }

    if( c_rettype != "void" ) 
	le_stub << "\treturn ret;\n";

    le_stub << "}\n";

    if( make_function ) wrapper.functions += function.str();
    wrapper.le_stubs += le_stub.str();
    wrapper.of_call += of_call.str();
    wrapper.names += name + "_ofw_name:\n\t.asciz \"" + name + "\"\n";
    wrapper.calltable += std::string("\t.long ofw_") + name + "\n";
    wrapper.ctindex++;
}

int main( int argc, char **argv ) {
    int status = 0;
    std::ifstream in;
    std::ofstream out, outcsource, outcheader;
    std::string line;
    const char *eol_marks = "#\r\n";
    ofw_wrappers wrappers;

    wrappers.base = 0xe00000;
    wrappers.ctindex = 0;

    if( argc < 5 ) {
	fprintf( stderr, "%s [interface.ofw] [ofw.s] [le_stub.c] [le_stub.h]\n", argv[0] );
	return 1;
    }

    in.open( argv[1] );
    if( !in ) {
	fprintf( stderr, "can't open %s\n", argv[1] );
	status = 1;
	goto error;
    }

    out.open( argv[2] );
    if( !out ) {
	fprintf( stderr, "can't open %s\n", argv[2] );
	status = 1;
	goto error;
    }

    outcsource.open( argv[3] );
    if( !outcsource ) {
	fprintf( stderr, "can't open %s\n", argv[3] );
	status = 1;
	goto error;
    }

    outcheader.open( argv[4] );
    if( !outcheader ) {
	fprintf( stderr, "can't open %s\n", argv[4] );
	status = 1;
	goto error;
    }

    while( std::getline( in, line ) ) {
	line = clip_eol( line, eol_marks );
	if( line.size() ) populate_definition( wrappers, line );
    }

    out << "/* AUTOMATICALLY GENERATED BY ofw_interface */\n"
	<< "\t.section .text\n"
	<< "\t.globl _start\n"
	<< "\t.globl ofw_call_addr\n"
	<< "ofw_call_addr:\n"
	<< "\t.long 0\n"
	<< "\n/* Function Wrappers */\n\n"
	<< wrappers.functions
	<< "\t/* Function Names */\n\n"
	<< wrappers.names
	<< "\n/* Function Call Table for Freeldr */\n\n"
	<< "ofw_functions:\n"
	<< wrappers.calltable
	<< "\n/* End */\n";

    outcsource << "/* AUTOMATICALLY GENERATED BY ofw_interface */\n"
	       << "#include \"of.h\"\n" 
	       << wrappers.le_stubs;

    outcheader << "/* AUTOMATICALLY GENERATE BY ofw_interface */\n"
	       << "#ifndef _OFW_CALLS_H\n"
	       << "#define _OFW_CALLS_H\n"
	       << wrappers.of_call
	       << "#endif/*_OFW_CALLS_H*/\n";

error:
    return status;
}
