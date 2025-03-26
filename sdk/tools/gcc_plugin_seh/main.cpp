/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     BSD Zero Clause License (https://spdx.org/licenses/0BSD)
 * PURPOSE:     Helper pragma implementation for pseh library (amd64)
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou
 *              Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <gcc-plugin.h>
#include <plugin-version.h>
#include <function.h>
#include <tree.h>
#include <c-family/c-pragma.h>
#include <c-family/c-common.h>

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cstdio>

#if 0 // To enable tracing
#define trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define trace(...)
#endif

#define is_alpha(c) (((c)>64 && (c)<91) || ((c)>96 && (c)<123))

#if defined(_WIN32) || defined(WIN32)
#define VISIBLE __declspec(dllexport)
#else
#define VISIBLE __attribute__((__visibility__("default")))
#endif

#define UNUSED __attribute__((__unused__))

int
VISIBLE
plugin_is_GPL_compatible = 1;

constexpr size_t k_header_statement_max_size = 20000;

struct seh_handler
{
    bool is_except;
    unsigned int line;
};

struct seh_function
{
    bool unwind;
    bool except;
    tree asm_header_text;
    tree asm_header;
    size_t count;
    std::vector<seh_handler> handlers;

    seh_function(struct function* fun)
        : unwind(false)
        , except(false)
        , count(0)
    {
        /* Reserve space for our header statement */
#if 0 // FIXME: crashes on older GCC
        asm_header_text = build_string(k_header_statement_max_size, "");
#else
        char buf[k_header_statement_max_size];
        memset(buf, 0, sizeof(buf));
        asm_header_text = build_string(sizeof(buf), buf);
#endif
        asm_header = build_stmt(fun->function_start_locus, ASM_EXPR, asm_header_text, NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
        ASM_VOLATILE_P(asm_header) = 1;
        add_stmt(asm_header);
    }
};

static std::unordered_map<struct function*, struct seh_function*> func_seh_map = {};

static
struct seh_function*
get_seh_function()
{
    auto search = func_seh_map.find(cfun);
    if (search != func_seh_map.end())
        return search->second;

    auto seh_fun = new seh_function(cfun);
    func_seh_map.insert({cfun, seh_fun});

    return seh_fun;
}

static
void
handle_seh_pragma(cpp_reader* UNUSED parser)
{
    tree x, arg, line;
    std::stringstream label_decl;
    bool is_except;

    if (!cfun)
    {
        error("%<#pragma REACTOS seh%> is not allowed outside functions");
        return;
    }

    if ((pragma_lex(&x) != CPP_OPEN_PAREN) ||
        (pragma_lex(&arg) != CPP_NAME) || // except or finally
        (pragma_lex(&x) != CPP_COMMA) ||
        (pragma_lex(&line) != CPP_NUMBER) || // Line number
        (pragma_lex(&x) != CPP_CLOSE_PAREN) ||
        (pragma_lex(&x) != CPP_EOF)
        )
    {
        error("%<#pragma REACTOS seh%> needs two parameters%>");
        return;
    }

    trace(stderr, "Pragma: %s, %u\n", IDENTIFIER_POINTER(arg), TREE_INT_CST_LOW(line));

    const char* op = IDENTIFIER_POINTER(arg);

    seh_function* seh_fun = get_seh_function();
    if (strcmp(op, "__seh$$except") == 0)
    {
        is_except = true;
        seh_fun->except = true;
    }
    else if (strcmp(op, "__seh$$finally") == 0)
    {
        is_except = false;
        seh_fun->unwind = true;
    }
    else
    {
        error("Wrong argument for %<#pragma REACTOS seh%>. Expected \"except\" or \"finally\"");
        return;
    }
    seh_fun->count++;

    seh_fun->handlers.push_back({is_except, (unsigned int)TREE_INT_CST_LOW(line)});

    /* Make sure we use a frame pointer. REACTOS' PSEH depends on this */
    cfun->machine->accesses_prev_frame = 1;
}

static
void
finish_seh_function(void* event_data, void* UNUSED user_data)
{
    tree fndef = (tree)event_data;
    struct function* fun = DECL_STRUCT_FUNCTION(fndef);

    auto search = func_seh_map.find(fun);
    if (search == func_seh_map.end())
        return;

    /* Get our SEH details and remove us from the map */
    seh_function* seh_fun = search->second;
    func_seh_map.erase(search);

    if (DECL_FUNCTION_PERSONALITY(fndef) != nullptr)
    {
        error("Function %s has a personality. Are you mixing SEH with C++ exceptions ?",
              IDENTIFIER_POINTER(fndef));
        return;
    }

    /* Update asm statement */
    std::stringstream asm_str;
    asm_str << ".seh_handler __C_specific_handler";
    if (seh_fun->unwind)
        asm_str << ", @unwind";
    if (seh_fun->except)
        asm_str << ", @except";
    asm_str << "\n";
    asm_str << "\t.seh_handlerdata\n";
    asm_str << "\t.long " << seh_fun->count << "\n";

    for (auto& handler : seh_fun->handlers)
    {
        asm_str << "\n\t.rva " << "__seh2$$begin_try__" << handler.line; /* Begin of tried code */
        asm_str << "\n\t.rva " << "__seh2$$end_try__" << handler.line; /* End of tried code */
        asm_str << "\n\t.rva " << "__seh2$$filter__" << handler.line; /* Filter function */
        if (handler.is_except)
            asm_str << "\n\t.rva " << "__seh2$$begin_except__" << handler.line; /* Called on except */
        else
            asm_str << "\n\t.long 0"; /* No unwind handler */
    }
    asm_str << "\n\t.seh_code\n";

    strncpy(const_cast<char*>(TREE_STRING_POINTER(seh_fun->asm_header_text)),
            asm_str.str().c_str(),
            TREE_STRING_LENGTH(seh_fun->asm_header_text));

    trace(stderr, "ASM: %s\n", asm_str.str().c_str());

    delete seh_fun;
}

static
void
register_seh_pragmas(void* UNUSED event_data, void* UNUSED user_data)
{
    c_register_pragma("REACTOS", "seh", handle_seh_pragma);
}

/* Return 0 on success or error code on failure */
extern "C"
VISIBLE
int plugin_init(struct plugin_name_args   *info,  /* Argument infor */
                struct plugin_gcc_version *version)   /* Version of GCC */
{
    if (!plugin_default_version_check (version, &gcc_version))
    {
        std::cerr << "This GCC plugin is for version " << GCCPLUGIN_VERSION_MAJOR << "." << GCCPLUGIN_VERSION_MINOR << "\n";
        return 1;
    }

    register_callback(info->base_name, PLUGIN_PRAGMAS, register_seh_pragmas, NULL);
    register_callback(info->base_name, PLUGIN_FINISH_PARSE_FUNCTION, finish_seh_function, NULL);

    return 0;
}
