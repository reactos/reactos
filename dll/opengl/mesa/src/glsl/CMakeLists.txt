
set_cpp()

# From Sconscript
include_directories(
    ../mapi
    ../mesa
    .
    glcpp
    ../../generated/glsl)

list(APPEND SOURCE
# generated files
    ../../generated/glsl/glcpp/glcpp-lex.c
    ../../generated/glsl/glcpp/glcpp-parse.c
    ../../generated/glsl/glsl_lexer.cpp
    ../../generated/glsl/glsl_parser.cpp
    ../../generated/glsl/builtin_function.cpp
# Regular files
    strtod.c
	ralloc.c
    glcpp/pp.c
    ast_expr.cpp
	ast_function.cpp
	ast_to_hir.cpp
	ast_type.cpp
	builtin_variables.cpp
	glsl_parser_extras.cpp
	glsl_types.cpp
	glsl_symbol_table.cpp
	hir_field_selection.cpp
	ir_basic_block.cpp
	ir_clone.cpp
	ir_constant_expression.cpp
	ir.cpp
	ir_expression_flattening.cpp
	ir_function_can_inline.cpp
	ir_function_detect_recursion.cpp
	ir_function.cpp
	ir_hierarchical_visitor.cpp
	ir_hv_accept.cpp
	ir_import_prototypes.cpp
	ir_print_visitor.cpp
	ir_reader.cpp
	ir_rvalue_visitor.cpp
	ir_set_program_inouts.cpp
	ir_validate.cpp
	ir_variable_refcount.cpp
	linker.cpp
	link_functions.cpp
	link_uniforms.cpp
	loop_analysis.cpp
	loop_controls.cpp
	loop_unroll.cpp
	lower_clip_distance.cpp
	lower_discard.cpp
	lower_if_to_cond_assign.cpp
	lower_instructions.cpp
	lower_jumps.cpp
	lower_mat_op_to_vec.cpp
	lower_noise.cpp
	lower_texture_projection.cpp
	lower_variable_index_to_cond_assign.cpp
	lower_vec_index_to_cond_assign.cpp
	lower_vec_index_to_swizzle.cpp
	lower_vector.cpp
	lower_output_reads.cpp
	opt_algebraic.cpp
	opt_constant_folding.cpp
	opt_constant_propagation.cpp
	opt_constant_variable.cpp
	opt_copy_propagation.cpp
	opt_copy_propagation_elements.cpp
	opt_dead_code.cpp
	opt_dead_code_local.cpp
	opt_dead_functions.cpp
	opt_discard_simplification.cpp
	opt_function_inlining.cpp
	opt_if_simplification.cpp
	opt_noop_swizzle.cpp
	opt_redundant_jumps.cpp
	opt_structure_splitting.cpp
	opt_swizzle_swizzle.cpp
	opt_tree_grafting.cpp
	s_expression.cpp)

#this is just a helper library, don't include it in the all target
add_library(mesa_glsl STATIC EXCLUDE_FROM_ALL ${SOURCE})
  
    