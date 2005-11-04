#ifndef __R300_FIXED_PIPELINES_H__
#define __R300_FIXED_PIPELINES_H__

/******** Flat color pipeline **********/
static struct r300_vertex_shader_state FLAT_COLOR_VERTEX_SHADER={
		program: {
			length: 20,

			body:{ d: {
			        EASY_VSF_OP(MUL, 0, ALL, TMP),
			        VSF_PARAM(3),
			        VSF_ATTR_W(0),
			        EASY_VSF_SOURCE(0, W, W, W, W, NONE, NONE),

			        EASY_VSF_OP(MUL, 1, ALL, RESULT),
			        VSF_REG(1),
				VSF_ATTR_UNITY(1),
				VSF_UNITY(1),

				EASY_VSF_OP(MAD, 0, ALL, TMP),
				VSF_PARAM(2),
				VSF_ATTR_Z(0),
				VSF_TMP(0),

				EASY_VSF_OP(MAD, 0, ALL, TMP),
				VSF_PARAM(1),
				VSF_ATTR_Y(0),
				VSF_TMP(0),

				EASY_VSF_OP(MAD, 0, ALL, RESULT),
				VSF_PARAM(0),
				VSF_ATTR_X(0),
				VSF_TMP(0),
				} }
			},

		matrix:{
			 {
			length: 16,
			body: { f: {
				2.0,
				0,
				0.0,
				0.0,
				0,
				2.5,
				0,
				0,
				0.0,
				0,
				-1.00,
				-1.0,
				-3.0,
				0,
				6.0,
				6.0
				} }
			 },
			{
			length: 0,
			},
			{
			length: 0,
			}
			},

		vector: {
			{
			length: 0,
			},
			{
			length: 0,
			}
			},

		unknown1: {
			length: 0
			},

		unknown2: {
			length: 0
			},

		program_start: 0,
		unknown_ptr1: 4,
		program_end: 4,

		param_offset: 0,
		param_count: 4,

		unknown_ptr2: 0,
		unknown_ptr3: 4
		};

static struct r300_pixel_shader_state FLAT_COLOR_PIXEL_SHADER={
		program: {
			tex: {
				length: 0
				},
			alu: {
				length: 1,
					/* My understanding is that we need at least 1 instructions for pixel shader,
					   in particular because alu_end==0 means there is one instruction */
				inst: {
					PFS_NOP
					}
				},
			node: {
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0}
				},
			active_nodes: 1,
			first_node_has_tex: 0,
			temp_register_count: 0,

			tex_offset: 0,
			tex_end: 0,
			alu_offset: 0,
			alu_end: 0
			},

		param_length: 0
		};


/******** Single texture pipeline ***********/
static struct r300_vertex_shader_state SINGLE_TEXTURE_VERTEX_SHADER={
		program: {
			length: 24,

			body: { d: {
				EASY_VSF_OP(MUL, 0, ALL, TMP),
				VSF_PARAM(3),
				VSF_ATTR_W(0),
				EASY_VSF_SOURCE(0, W, W, W, W, NONE, NONE),

				EASY_VSF_OP(MUL, 2, ALL, RESULT),
				VSF_REG(2),
				VSF_ATTR_UNITY(2),
				VSF_UNITY(2),

				EASY_VSF_OP(MAD, 0, ALL, TMP),
				VSF_PARAM(2),
				VSF_ATTR_Z(0),
				VSF_TMP(0),

				EASY_VSF_OP(MUL, 1, ALL, RESULT),
				VSF_REG(1),
				VSF_ATTR_UNITY(1),
				VSF_UNITY(1),

				EASY_VSF_OP(MAD, 0, ALL, TMP),
				VSF_PARAM(1),
				VSF_ATTR_Y(0),
				VSF_TMP(0),

				EASY_VSF_OP(MAD, 0, ALL, RESULT),
				VSF_PARAM(0),
				VSF_ATTR_X(0),
				VSF_TMP(0),
				} }
			},

		matrix:{
			 {
			length: 16,
			body: { f: {
				2.0,
				0,
				0.0,
				0.0,
				0,
				2.5,
				0,
				0,
				0.0,
				0,
				-1.00,
				-1.0,
				-3.0,
				0,
				6.0,
				6.0
				} }
			 },
			{
			length: 0,
			},
			{
			length: 0,
			}
			},

		vector: {
			{
			length: 0,
			},
			{
			length: 0,
			}
			},

		unknown1: {
			length: 0
			},

		unknown2: {
			length: 4,
			body: { f: {
				0.0,
				0.0,
				1.0,
				0.0
				} }
			},

		program_start: 0,
		unknown_ptr1: 5,
		program_end: 5,

		param_offset: 0,
		param_count: 4,

		unknown_ptr2: 0,
		unknown_ptr3: 5
		};

static struct r300_pixel_shader_state SINGLE_TEXTURE_PIXEL_SHADER={
		program: {
			tex: {
				length: 1,
				inst: { 0x00018000 }
				},
			alu: {
				length: 2,
				inst:
					{
/* I get misc problems without this after doing cold-reboot.
   This would imply that alu programming is buggy. --aet */
#if 1
					PFS_NOP,
#endif

					/* What are 0's ORed with flags ? They are register numbers that
					   just happen to be 0 */
					{
					EASY_PFS_INSTR0(MAD, SRC0C_XYZ, SRC1C_XYZ, ZERO),
					EASY_PFS_INSTR1(0, 0, 1, 0 | PFS_FLAG_CONST, NONE, ALL),

#if 0
					/* no alpha in textures */
					EASY_PFS_INSTR2(MAD, SRC0A, ONE, ZERO),
					EASY_PFS_INSTR3(0, 1, 0 | PFS_FLAG_CONST, 0 | PFS_FLAG_CONST, OUTPUT)
#endif

					/* alpha in textures */
					EASY_PFS_INSTR2(MAD, SRC0A, SRC1A, ZERO),
					EASY_PFS_INSTR3(0, 0, 1, 0 | PFS_FLAG_CONST, OUTPUT)
					}
					}
				},
			node: {
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0},
				{ 0, 0, 0, 0}
				},

			active_nodes: 1,
			first_node_has_tex: 1,
			temp_register_count: 1,

			tex_offset: 0,
			tex_end: 0,
			alu_offset: 0,
			alu_end: 0
			},

		param_length: 8,
		param: {
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0},
			{ 0.0, 0.0, 0.0, 0.0}
			}
		};

#endif
