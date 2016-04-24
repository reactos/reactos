#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class ofw_wrappers {
public:
    int base, ctindex, method_ctindex;
    std::string functions;
    std::string names;
    std::string calltable;
    std::string le_stubs;
    std::string of_call;
};

class vartype {
public:
    vartype( const std::string &typestr ) {
    size_t amp = typestr.find('&');
    size_t col = typestr.find(':');
    if( amp != std::string::npos ) {
        if( col > amp && col != std::string::npos )
             lit_value = typestr.substr(amp+1,col-amp+1);
        else lit_value = typestr.substr(amp+1);
    }
    if( col != std::string::npos ) {
        if( amp > col && amp != std::string::npos )
             len = typestr.substr(col+1,amp-col+1);
        else len = typestr.substr(col+1);
    }

    if( amp != std::string::npos && amp < col ) col = amp;
    if( col == std::string::npos ) col = typestr.size();
    c_type = typestr.substr(0,col);
    }

    vartype( const vartype &other )
    : c_type(other.c_type),
      len(other.len),
      lit_value(other.lit_value) {
    }

    vartype &operator = ( const vartype &other ) {
        c_type = other.c_type;
        len = other.len;
        lit_value = other.lit_value;
        return *this;
    }

    std::string c_type;
    std::string len;
    std::string lit_value;
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

void populate_definition( ofw_wrappers &wrapper, const std::string &line ) {
    std::istringstream iss(line);
    bool make_function = true, method_call = false, make_stub = true, comma;
    std::string name, nametext, argtype, rettype;
    std::vector<vartype> argtypes;
    int args, rets, i, local_offset, total_stack, userarg_start = 0;
    size_t f;
    std::ostringstream function, ct_csource, le_stub, of_call;

    iss >> name >> args >> rets;

    if( !name.size() ) return;

    if( (f = name.find('!')) != std::string::npos ) {
        nametext = name.substr(f+1);
        name = name.substr(0,f);
    }
    if( name[0] == '-' ) {
        name = name.substr(1);
        make_function = false;
    }
    if( name[0] == '+' ) {
        name = name.substr(1);
        make_stub = false;
    }
    if( name[0] == '@' ) {
        name = name.substr(1);
        method_call = true;
        make_function = false;
    }

    if( !nametext.size() ) nametext = name;

    for( i = 1; i < (int)name.size(); i++ )
        if( name[i] == '-' ) name[i] = '_';

    if( nametext == "call-method" )
        wrapper.method_ctindex = wrapper.ctindex;

    for( i = 0; i < args; i++ ) {
        iss >> argtype;
        argtypes.push_back(vartype(argtype));
    }

    if( method_call ) {
        userarg_start = 1;
        args += 2;
    }

    iss >> rettype;
    if( !rettype.size() ) rettype = "void";

    local_offset = (3 + rets + args) * 4;
    total_stack = round_up(12 + local_offset, 16);

    function << "asm_ofw_" << name << ":\n"
             << "\t/* Reserve stack space */\n"
             << "\tsubi %r1,%r1," << total_stack << "\n"
             << "\t/* Store r8, r9, lr */\n"
             << "\tstw %r8," << (local_offset + 0) << "(%r1)\n"
             << "\tstw %r9," << (local_offset + 4) << "(%r1)\n"
             << "\tmflr %r8\n"
             << "\tstw %r8," << (local_offset + 8) << "(%r1)\n"
             << "\t/* Get read name */\n"
             << "\tlis %r8," << name << "_ofw_name@ha\n"
             << "\taddi %r9,%r8," << name << "_ofw_name@l\n"
             << "\tstw %r9,0(%r1)\n"
             << "\t/* " << args << " arguments and " << rets << " return */\n"
             << "\tli %r9," << args << "\n"
             << "\tstw %r9,4(%r1)\n"
             << "\tli %r9," << rets << "\n"
             << "\tstw %r9,8(%r1)\n";

    for( int i = 0; i < args; i++ )
    function << "\tstw %r" << (i+3) << "," << (4 * (i + 3)) << "(%r1)\n";

    function << "\t/* Load up the call address */\n"
             << "\tlis %r10,ofw_call_addr@ha\n"
             << "\tlwz %r9,ofw_call_addr@l(%r10)\n"
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

    if( method_call ) {
        argtypes.insert(argtypes.begin(),vartype("int"));
        argtypes.insert(argtypes.begin(),vartype("char*"));
    }

    le_stub << rettype << " ofw_" << name << "(";

    comma = false;
    for( i = userarg_start; i < args; i++ ) {
        if( !argtypes[i].lit_value.size() ) {
            if( !comma ) comma = true; else le_stub << ",";
            le_stub << argtypes[i].c_type << " arg" << i;
        }
    }
    le_stub << ")";
    of_call << le_stub.str() << ";\n";

    le_stub << " {\n";
    if( rettype != "void" )
        le_stub << "\t" << rettype << " ret;\n";

    if( method_call ) {
        le_stub << "\tchar arg0["
                << round_up(nametext.size()+1,8)
                << "] = \"" << nametext << "\";\n";
    }

    for( i = 0; i < args; i++ ) {
        if( argtypes[i].lit_value.size() ) {
            le_stub << "\t" << argtypes[i].c_type << " arg" << i << " = "
                    << argtypes[i].lit_value << ";\n";
        }
    }

    le_stub << "\t";
    if( rettype != "void" ) le_stub << "ret = (" << rettype << ")";

    le_stub << "ofproxy(" <<
      (method_call ? (wrapper.method_ctindex * 4) : (wrapper.ctindex * 4));

    for( i = 0; i < 6; i++ ) {
        if( i < args ) le_stub << ",(void *)arg" << i;
        else le_stub << ",NULL";
    }

    le_stub << ");\n";

    if( rettype != "void" )
    le_stub << "\treturn ret;\n";

    le_stub << "}\n";

    if( make_function ) wrapper.functions += function.str();
    if( make_stub ) {
        wrapper.le_stubs += le_stub.str();
        wrapper.of_call += of_call.str();
    }
    if( !method_call ) {
        wrapper.names += name + "_ofw_name:\n\t.asciz \"" + nametext + "\"\n";
        wrapper.calltable += std::string("\t.long asm_ofw_") + name + "\n";
        wrapper.ctindex++;
    }
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
        << "\t.align 4\n"
        << "\t.globl _start\n"
        << "\t.globl ofw_functions\n"
        << "\t.globl ofw_call_addr\n"
        << "ofw_call_addr:\n"
        << "\t.long 0\n"
        << "\n/* Function Wrappers */\n\n"
        << wrappers.functions
        << "\n/* Function Names */\n\n"
        << wrappers.names
        << "\n/* Function Call Table for Freeldr */\n\n"
        << "\t.align 4\n"
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
