
list(APPEND SOURCE
    ../../../generated/mesa/program/lex.yy.c
    ../../../generated/mesa/program/program_parse.tab.c
    arbprogparse.c
    hash_table.c
    ir_to_mesa.cpp
    # nvfragparse.c
    # nvvertparse.c
    program.c
    program_parse_extra.c
    prog_cache.c
    prog_execute.c
    prog_instruction.c
    prog_noise.c
    prog_optimize.c
    prog_opt_constant_fold.c
    prog_parameter.c
    prog_parameter_layout.c
    prog_print.c
    prog_statevars.c
    programopt.c
    sampler.cpp
    symbol_table.c
    string_to_uint_map.cpp
)

if(NOT MSVC)
    add_compile_flags("-Wno-format")
endif()

add_library(mesa_program STATIC ${SOURCE})