/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green 
 * Copyright 2006-2007 Henri Verbeet
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * D3D shader asm has swizzles on source parameters, and write masks for
 * destination parameters. GLSL uses swizzles for both. The result of this is
 * that for example "mov dst.xw, src.zyxw" becomes "dst.xw = src.zw" in GLSL.
 * Ie, to generate a proper GLSL source swizzle, we need to take the D3D write
 * mask for the destination parameter into account.
 */

#include "config.h"
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);

#define GLINFO_LOCATION      (*gl_info)

typedef struct {
    char reg_name[50];
    char mask_str[6];
} glsl_dst_param_t;

typedef struct {
    char reg_name[50];
    char param_str[100];
} glsl_src_param_t;

typedef struct {
    const char *name;
    DWORD coord_mask;
} glsl_sample_function_t;

/** Prints the GLSL info log which will contain error messages if they exist */
void print_glsl_info_log(WineD3D_GL_Info *gl_info, GLhandleARB obj) {
    
    int infologLength = 0;
    char *infoLog;

    GL_EXTCALL(glGetObjectParameterivARB(obj,
               GL_OBJECT_INFO_LOG_LENGTH_ARB,
               &infologLength));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (infologLength > 1)
    {
        infoLog = HeapAlloc(GetProcessHeap(), 0, infologLength);
        GL_EXTCALL(glGetInfoLogARB(obj, infologLength, NULL, infoLog));
        FIXME("Error received from GLSL shader #%u: %s\n", obj, debugstr_a(infoLog));
        HeapFree(GetProcessHeap(), 0, infoLog);
    }
}

/**
 * Loads (pixel shader) samplers
 */
static void shader_glsl_load_psamplers(
    WineD3D_GL_Info *gl_info,
    IWineD3DStateBlock* iface) {

    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    GLhandleARB programId = stateBlock->glsl_program->programId;
    GLhandleARB name_loc;
    int i;
    char sampler_name[20];

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
        _snprintf(sampler_name, sizeof(sampler_name), "Psampler%d", i);
        name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
        if (name_loc != -1) {
            int mapped_unit = stateBlock->wineD3DDevice->texUnitMap[i];
            if (mapped_unit != -1 && mapped_unit < GL_LIMITS(fragment_samplers)) {
                TRACE("Loading %s for texture %d\n", sampler_name, mapped_unit);
                GL_EXTCALL(glUniform1iARB(name_loc, mapped_unit));
                checkGLcall("glUniform1iARB");
            } else {
                ERR("Trying to load sampler %s on unsupported unit %d\n", sampler_name, mapped_unit);
            }
        }
    }
}

static void shader_glsl_load_vsamplers(WineD3D_GL_Info *gl_info, IWineD3DStateBlock* iface) {
    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    GLhandleARB programId = stateBlock->glsl_program->programId;
    GLhandleARB name_loc;
    char sampler_name[20];
    int i;

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        _snprintf(sampler_name, sizeof(sampler_name), "Vsampler%d", i);
        name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
        if (name_loc != -1) {
            int mapped_unit = stateBlock->wineD3DDevice->texUnitMap[MAX_FRAGMENT_SAMPLERS + i];
            if (mapped_unit != -1 && mapped_unit < GL_LIMITS(combined_samplers)) {
                TRACE("Loading %s for texture %d\n", sampler_name, mapped_unit);
                GL_EXTCALL(glUniform1iARB(name_loc, mapped_unit));
                checkGLcall("glUniform1iARB");
            } else {
                ERR("Trying to load sampler %s on unsupported unit %d\n", sampler_name, mapped_unit);
            }
        }
    }
}

/** 
 * Loads floating point constants (aka uniforms) into the currently set GLSL program.
 * When constant_list == NULL, it will load all the constants.
 */
static void shader_glsl_load_constantsF(IWineD3DBaseShaderImpl* This, WineD3D_GL_Info *gl_info,
        unsigned int max_constants, float* constants, GLhandleARB *constant_locations,
        struct list *constant_list) {
    constants_entry *constant;
    local_constant* lconst;
    GLhandleARB tmp_loc;
    DWORD i, j;
    DWORD *idx;

    if (TRACE_ON(d3d_shader)) {
        LIST_FOR_EACH_ENTRY(constant, constant_list, constants_entry, entry) {
            idx = constant->idx;
            j = constant->count;
            while (j--) {
                i = *idx++;
                tmp_loc = constant_locations[i];
                if (tmp_loc != -1) {
                    TRACE_(d3d_constants)("Loading constants %i: %f, %f, %f, %f\n", i,
                            constants[i * 4 + 0], constants[i * 4 + 1],
                            constants[i * 4 + 2], constants[i * 4 + 3]);
                }
            }
        }
    }
    LIST_FOR_EACH_ENTRY(constant, constant_list, constants_entry, entry) {
        idx = constant->idx;
        j = constant->count;
        while (j--) {
            i = *idx++;
            tmp_loc = constant_locations[i];
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, constants + (i * 4)));
            }
        }
    }
    checkGLcall("glUniform4fvARB()");

    /* Load immediate constants */
    if (TRACE_ON(d3d_shader)) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            tmp_loc = constant_locations[lconst->idx];
            if (tmp_loc != -1) {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE_(d3d_constants)("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
    }
    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
        tmp_loc = constant_locations[lconst->idx];
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, (GLfloat*)lconst->value));
        }
    }
    checkGLcall("glUniform4fvARB()");
}

/** 
 * Loads integer constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
static void shader_glsl_load_constantsI(
    IWineD3DBaseShaderImpl* This,
    WineD3D_GL_Info *gl_info,
    GLhandleARB programId,
    unsigned max_constants,
    int* constants,
    BOOL* constants_set) {
    
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[8];
    char is_pshader = shader_is_pshader_version(This->baseShader.hex_version);
    const char* prefix = is_pshader? "PI":"VI";
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE_(d3d_constants)("Loading constants %i: %i, %i, %i, %i\n",
                  i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            _snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4ivARB(tmp_loc, 1, &constants[i*4]));
                checkGLcall("glUniform4ivARB");
            }
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsI);
    while (ptr) {
        local_constant* lconst = LIST_ENTRY(ptr, struct local_constant, entry);
        unsigned int idx = lconst->idx;
        GLint* values = (GLint*) lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i, %i, %i, %i\n", idx,
            values[0], values[1], values[2], values[3]);

        _snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform4ivARB");
        }
        ptr = list_next(&This->baseShader.constantsI, ptr);
    }
}

/** 
 * Loads boolean constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
static void shader_glsl_load_constantsB(
    IWineD3DBaseShaderImpl* This,
    WineD3D_GL_Info *gl_info,
    GLhandleARB programId,
    unsigned max_constants,
    BOOL* constants,
    BOOL* constants_set) {
    
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[8];
    char is_pshader = shader_is_pshader_version(This->baseShader.hex_version);
    const char* prefix = is_pshader? "PB":"VB";
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE_(d3d_constants)("Loading constants %i: %i;\n", i, constants[i*4]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            _snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, &constants[i*4]));
                checkGLcall("glUniform1ivARB");
            }
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsB);
    while (ptr) {
        local_constant* lconst = LIST_ENTRY(ptr, struct local_constant, entry);
        unsigned int idx = lconst->idx;
        GLint* values = (GLint*) lconst->value;

        TRACE_(d3d_constants)("Loading local constants %i: %i\n", idx, values[0]);

        _snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform1ivARB");
        }
        ptr = list_next(&This->baseShader.constantsB, ptr);
    }
}



/**
 * Loads the app-supplied constants into the currently set GLSL program.
 */
void shader_glsl_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {
   
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) device;
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;

    GLhandleARB *constant_locations;
    struct list *constant_list;
    GLhandleARB programId;
    GLint pos;

    if (!stateBlock->glsl_program) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    programId = stateBlock->glsl_program->programId;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;
        GLint pos;

        constant_locations = stateBlock->glsl_program->vuniformF_locations;
        constant_list = &stateBlock->set_vconstantsF;

        /* Load vertex shader samplers */
        shader_glsl_load_vsamplers(gl_info, (IWineD3DStateBlock*)stateBlock);

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, GL_LIMITS(vshader_constantsF),
                stateBlock->vertexShaderConstantF, constant_locations, constant_list);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, programId, MAX_CONST_I,
                                    stateBlock->vertexShaderConstantI,
                                    stateBlock->changed.vertexShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->vertexShaderConstantB,
                                    stateBlock->changed.vertexShaderConstantsB);

        /* Upload the position fixup params */
        pos = GL_EXTCALL(glGetUniformLocationARB(programId, "posFixup"));
        checkGLcall("glGetUniformLocationARB");
        GL_EXTCALL(glUniform4fvARB(pos, 1, &deviceImpl->posFixup[0]));
        checkGLcall("glUniform4fvARB");
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        constant_locations = stateBlock->glsl_program->puniformF_locations;
        constant_list = &stateBlock->set_pconstantsF;

        /* Load pixel shader samplers */
        shader_glsl_load_psamplers(gl_info, (IWineD3DStateBlock*) stateBlock);

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, GL_LIMITS(pshader_constantsF),
                stateBlock->pixelShaderConstantF, constant_locations, constant_list);

        /* Load DirectX 9 integer constants/uniforms for pixel shader */
        shader_glsl_load_constantsI(pshader, gl_info, programId, MAX_CONST_I,
                                    stateBlock->pixelShaderConstantI, 
                                    stateBlock->changed.pixelShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for pixel shader */
        shader_glsl_load_constantsB(pshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->pixelShaderConstantB, 
                                    stateBlock->changed.pixelShaderConstantsB);

        /* Upload the environment bump map matrix if needed. The needsbumpmat member specifies the texture stage to load the matrix from.
         * It can't be 0 for a valid texbem instruction.
         */
        if(((IWineD3DPixelShaderImpl *) pshader)->needsbumpmat != -1) {
            float *data = (float *) &stateBlock->textureState[(int) ((IWineD3DPixelShaderImpl *) pshader)->needsbumpmat][WINED3DTSS_BUMPENVMAT00];
            pos = GL_EXTCALL(glGetUniformLocationARB(programId, "bumpenvmat"));
            checkGLcall("glGetUniformLocationARB");
            GL_EXTCALL(glUniformMatrix2fvARB(pos, 1, 0, data));
            checkGLcall("glUniform4fvARB");
        }
    }
}

/** Generate the variable & register declarations for the GLSL output target */
void shader_generate_glsl_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    int i;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char prefix = pshader ? 'P' : 'V';

    /* Prototype the subroutines */
    for (i = 0; i < This->baseShader.limits.label; i++) {
        if (reg_maps->labels[i])
            shader_addline(buffer, "void subroutine%lu();\n", i);
    }

    /* Declare the constants (aka uniforms) */
    if (This->baseShader.limits.constant_float > 0) {
        unsigned max_constantsF = min(This->baseShader.limits.constant_float, 
                (pshader ? GL_LIMITS(pshader_constantsF) : GL_LIMITS(vshader_constantsF)));
        shader_addline(buffer, "uniform vec4 %cC[%u];\n", prefix, max_constantsF);
    }

    if (This->baseShader.limits.constant_int > 0)
        shader_addline(buffer, "uniform ivec4 %cI[%u];\n", prefix, This->baseShader.limits.constant_int);

    if (This->baseShader.limits.constant_bool > 0)
        shader_addline(buffer, "uniform bool %cB[%u];\n", prefix, This->baseShader.limits.constant_bool);

    if(!pshader)
        shader_addline(buffer, "uniform vec4 posFixup;\n");
    else if(reg_maps->bumpmat != -1)
        shader_addline(buffer, "uniform mat2 bumpenvmat;\n");

    /* Declare texture samplers */ 
    for (i = 0; i < This->baseShader.limits.sampler; i++) {
        if (reg_maps->samplers[i]) {

            DWORD stype = reg_maps->samplers[i] & WINED3DSP_TEXTURETYPE_MASK;
            switch (stype) {

                case WINED3DSTT_1D:
                    shader_addline(buffer, "uniform sampler1D %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_2D:
                    shader_addline(buffer, "uniform sampler2D %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_CUBE:
                    shader_addline(buffer, "uniform samplerCube %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_VOLUME:
                    shader_addline(buffer, "uniform sampler3D %csampler%lu;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %csampler%lu;\n", prefix, i);
                    FIXME("Unrecognized sampler type: %#x\n", stype);
                    break;
            }
        }
    }
    
    /* Declare address variables */
    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ivec4 A%d;\n", i);
    }

    /* Declare texture coordinate temporaries and initialize them */
    for (i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i]) 
            shader_addline(buffer, "vec4 T%lu = gl_TexCoord[%lu];\n", i, i);
    }

    /* Declare input register temporaries */
    for (i=0; i < This->baseShader.limits.packed_input; i++) {
        if (reg_maps->packed_input[i])
            shader_addline(buffer, "vec4 IN%lu;\n", i);
    }

    /* Declare output register temporaries */
    for (i = 0; i < This->baseShader.limits.packed_output; i++) {
        if (reg_maps->packed_output[i])
            shader_addline(buffer, "vec4 OUT%lu;\n", i);
    }

    /* Declare temporary variables */
    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "vec4 R%lu;\n", i);
    }

    /* Declare attributes */
    for (i = 0; i < This->baseShader.limits.attributes; i++) {
        if (reg_maps->attributes[i])
            shader_addline(buffer, "attribute vec4 attrib%i;\n", i);
    }

    /* Declare loop register aL */
    if (reg_maps->loop) {
        shader_addline(buffer, "int aL;\n");
        shader_addline(buffer, "int tmpInt;\n");
    }
    
    /* Temporary variables for matrix operations */
    shader_addline(buffer, "vec4 tmp0;\n");
    shader_addline(buffer, "vec4 tmp1;\n");

    /* Start the main program */
    shader_addline(buffer, "void main() {\n");
}

/*****************************************************************************
 * Functions to generate GLSL strings from DirectX Shader bytecode begin here.
 *
 * For more information, see http://wiki.winehq.org/DirectX-Shaders
 ****************************************************************************/

/* Prototypes */
static void shader_glsl_add_src_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, DWORD mask, glsl_src_param_t *src_param);

/** Used for opcode modifiers - They multiply the result by the specified amount */
static const char * const shift_glsl_tab[] = {
    "",           /*  0 (none) */ 
    "2.0 * ",     /*  1 (x2)   */ 
    "4.0 * ",     /*  2 (x4)   */ 
    "8.0 * ",     /*  3 (x8)   */ 
    "16.0 * ",    /*  4 (x16)  */ 
    "32.0 * ",    /*  5 (x32)  */ 
    "",           /*  6 (x64)  */ 
    "",           /*  7 (x128) */ 
    "",           /*  8 (d256) */ 
    "",           /*  9 (d128) */ 
    "",           /* 10 (d64)  */ 
    "",           /* 11 (d32)  */ 
    "0.0625 * ",  /* 12 (d16)  */ 
    "0.125 * ",   /* 13 (d8)   */ 
    "0.25 * ",    /* 14 (d4)   */ 
    "0.5 * "      /* 15 (d2)   */ 
};

/* Generate a GLSL parameter that does the input modifier computation and return the input register/mask to use */
static void shader_glsl_gen_modifier (
    const DWORD instr,
    const char *in_reg,
    const char *in_regswizzle,
    char *out_str) {

    out_str[0] = 0;
    
    if (instr == WINED3DSIO_TEXKILL)
        return;

    switch (instr & WINED3DSP_SRCMOD_MASK) {
    case WINED3DSPSM_DZ: /* Need to handle this in the instructions itself (texld & texcrd). */
    case WINED3DSPSM_DW:
    case WINED3DSPSM_NONE:
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NEG:
        sprintf(out_str, "-%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NOT:
        sprintf(out_str, "!%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_BIAS:
        sprintf(out_str, "(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_BIASNEG:
        sprintf(out_str, "-(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_SIGN:
        sprintf(out_str, "(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_SIGNNEG:
        sprintf(out_str, "-(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_COMP:
        sprintf(out_str, "(1.0 - %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2:
        sprintf(out_str, "(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2NEG:
        sprintf(out_str, "-(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_ABS:
        sprintf(out_str, "abs(%s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_ABSNEG:
        sprintf(out_str, "-abs(%s%s)", in_reg, in_regswizzle);
        break;
    default:
        FIXME("Unhandled modifier %u\n", (instr & WINED3DSP_SRCMOD_MASK));
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
    }
}

/** Writes the GLSL variable name that corresponds to the register that the
 * DX opcode parameter is trying to access */
static void shader_glsl_get_register_name(
    const DWORD param,
    const DWORD addr_token,
    char* regstr,
    BOOL* is_color,
    SHADER_OPCODE_ARG* arg) {

    /* oPos, oFog and oPts in D3D */
    static const char * const hwrastout_reg_names[] = { "gl_Position", "gl_FogFragCoord", "gl_PointSize" };

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    WineD3D_GL_Info* gl_info = &deviceImpl->adapter->gl_info;

    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char tmpStr[50];

    *is_color = FALSE;   
 
    switch (regtype) {
    case WINED3DSPR_TEMP:
        sprintf(tmpStr, "R%u", reg);
    break;
    case WINED3DSPR_INPUT:
        if (pshader) {
            /* Pixel shaders >= 3.0 */
            if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
                sprintf(tmpStr, "IN%u", reg);
             else {
                if (reg==0)
                    strcpy(tmpStr, "gl_Color");
                else
                    strcpy(tmpStr, "gl_SecondaryColor");
            }
        } else {
            if (vshader_input_is_color((IWineD3DVertexShader*) This, reg))
               *is_color = TRUE;
            sprintf(tmpStr, "attrib%u", reg);
        } 
        break;
    case WINED3DSPR_CONST:
    {
        const char* prefix = pshader? "PC":"VC";

        /* Relative addressing */
        if (param & WINED3DSHADER_ADDRMODE_RELATIVE) {

           /* Relative addressing on shaders 2.0+ have a relative address token, 
            * prior to that, it was hard-coded as "A0.x" because there's only 1 register */
           if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2)  {
               glsl_src_param_t rel_param;
               shader_glsl_add_src_param(arg, addr_token, 0, WINED3DSP_WRITEMASK_0, &rel_param);
               sprintf(tmpStr, "%s[%s + %u]", prefix, rel_param.param_str, reg);
           } else
               sprintf(tmpStr, "%s[A0.x + %u]", prefix, reg);

        } else
             sprintf(tmpStr, "%s[%u]", prefix, reg);

        break;
    }
    case WINED3DSPR_CONSTINT:
        if (pshader)
            sprintf(tmpStr, "PI[%u]", reg);
        else
            sprintf(tmpStr, "VI[%u]", reg);
        break;
    case WINED3DSPR_CONSTBOOL:
        if (pshader)
            sprintf(tmpStr, "PB[%u]", reg);
        else
            sprintf(tmpStr, "VB[%u]", reg);
        break;
    case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
        if (pshader) {
            sprintf(tmpStr, "T%u", reg);
        } else {
            sprintf(tmpStr, "A%u", reg);
        }
    break;
    case WINED3DSPR_LOOP:
        sprintf(tmpStr, "aL");
    break;
    case WINED3DSPR_SAMPLER:
        if (pshader)
            sprintf(tmpStr, "Psampler%u", reg);
        else
            sprintf(tmpStr, "Vsampler%u", reg);
    break;
    case WINED3DSPR_COLOROUT:
        if (reg >= GL_LIMITS(buffers)) {
            WARN("Write to render target %u, only %d supported\n", reg, 4);
        }
        if (GL_SUPPORT(ARB_DRAW_BUFFERS)) {
            sprintf(tmpStr, "gl_FragData[%u]", reg);
        } else { /* On older cards with GLSL support like the GeforceFX there's only one buffer. */
            sprintf(tmpStr, "gl_FragColor");
        }
    break;
    case WINED3DSPR_RASTOUT:
        sprintf(tmpStr, "%s", hwrastout_reg_names[reg]);
    break;
    case WINED3DSPR_DEPTHOUT:
        sprintf(tmpStr, "gl_FragDepth");
    break;
    case WINED3DSPR_ATTROUT:
        if (reg == 0) {
            sprintf(tmpStr, "gl_FrontColor");
        } else {
            sprintf(tmpStr, "gl_FrontSecondaryColor");
        }
    break;
    case WINED3DSPR_TEXCRDOUT:
        /* Vertex shaders >= 3.0: WINED3DSPR_OUTPUT */
        if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
            sprintf(tmpStr, "OUT%u", reg);
        else
            sprintf(tmpStr, "gl_TexCoord[%u]", reg);
    break;
    case WINED3DSPR_MISCTYPE:
        if (reg == 0) {
            /* vPos */
            sprintf(tmpStr, "gl_FragCoord");
        } else {
            /* gl_FrontFacing could be used for vFace, but note that
             * gl_FrontFacing is a bool, while vFace is a float for
             * which the sign determines front/back */
            FIXME("Unhandled misctype register %d\n", reg);
            sprintf(tmpStr, "unrecognized_register");
        }
        break;
    default:
        FIXME("Unhandled register name Type(%d)\n", regtype);
        sprintf(tmpStr, "unrecognized_register");
    break;
    }

    strcat(regstr, tmpStr);
}

/* Get the GLSL write mask for the destination register */
static DWORD shader_glsl_get_write_mask(const DWORD param, char *write_mask) {
    char *ptr = write_mask;
    DWORD mask = param & WINED3DSP_WRITEMASK_ALL;

    if (shader_is_scalar(param)) {
        mask = WINED3DSP_WRITEMASK_0;
    } else {
        *ptr++ = '.';
        if (param & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
        if (param & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
        if (param & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
        if (param & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
    }

    *ptr = '\0';

    return mask;
}

static size_t shader_glsl_get_write_mask_size(DWORD write_mask) {
    size_t size = 0;

    if (write_mask & WINED3DSP_WRITEMASK_0) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_1) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_2) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_3) ++size;

    return size;
}

static void shader_glsl_get_swizzle(const DWORD param, BOOL fixup, DWORD mask, char *swizzle_str) {
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    DWORD swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";
    char *ptr = swizzle_str;

    if (!shader_is_scalar(param)) {
        *ptr++ = '.';
        /* swizzle bits fields: wwzzyyxx */
        if (mask & WINED3DSP_WRITEMASK_0) *ptr++ = swizzle_chars[swizzle & 0x03];
        if (mask & WINED3DSP_WRITEMASK_1) *ptr++ = swizzle_chars[(swizzle >> 2) & 0x03];
        if (mask & WINED3DSP_WRITEMASK_2) *ptr++ = swizzle_chars[(swizzle >> 4) & 0x03];
        if (mask & WINED3DSP_WRITEMASK_3) *ptr++ = swizzle_chars[(swizzle >> 6) & 0x03];
    }

    *ptr = '\0';
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static void shader_glsl_add_src_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, DWORD mask, glsl_src_param_t *src_param) {
    BOOL is_color = FALSE;
    char swizzle_str[6];

    src_param->reg_name[0] = '\0';
    src_param->param_str[0] = '\0';
    swizzle_str[0] = '\0';

    shader_glsl_get_register_name(param, addr_token, src_param->reg_name, &is_color, arg);

    shader_glsl_get_swizzle(param, is_color, mask, swizzle_str);
    shader_glsl_gen_modifier(param, src_param->reg_name, swizzle_str, src_param->param_str);
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static DWORD shader_glsl_add_dst_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, glsl_dst_param_t *dst_param) {
    BOOL is_color = FALSE;

    dst_param->mask_str[0] = '\0';
    dst_param->reg_name[0] = '\0';

    shader_glsl_get_register_name(param, addr_token, dst_param->reg_name, &is_color, arg);
    return shader_glsl_get_write_mask(param, dst_param->mask_str);
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst_ext(SHADER_BUFFER *buffer, SHADER_OPCODE_ARG *arg, const DWORD param) {
    glsl_dst_param_t dst_param;
    DWORD mask;
    int shift;

    mask = shader_glsl_add_dst_param(arg, param, arg->dst_addr, &dst_param);

    if(mask) {
        shift = (param & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
        shader_addline(buffer, "%s%s = %s(", dst_param.reg_name, dst_param.mask_str, shift_glsl_tab[shift]);
    }

    return mask;
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst(SHADER_BUFFER *buffer, SHADER_OPCODE_ARG *arg) {
    return shader_glsl_append_dst_ext(buffer, arg, arg->dst);
}

/** Process GLSL instruction modifiers */
void shader_glsl_add_instruction_modifiers(SHADER_OPCODE_ARG* arg) {
    
    DWORD mask = arg->dst & WINED3DSP_DSTMOD_MASK;
 
    if (arg->opcode->dst_token && mask != 0) {
        glsl_dst_param_t dst_param;

        shader_glsl_add_dst_param(arg, arg->dst, 0, &dst_param);

        if (mask & WINED3DSPDM_SATURATE) {
            /* _SAT means to clamp the value of the register to between 0 and 1 */
            shader_addline(arg->buffer, "%s%s = clamp(%s%s, 0.0, 1.0);\n", dst_param.reg_name,
                    dst_param.mask_str, dst_param.reg_name, dst_param.mask_str);
        }
        if (mask & WINED3DSPDM_MSAMPCENTROID) {
            FIXME("_centroid modifier not handled\n");
        }
        if (mask & WINED3DSPDM_PARTIALPRECISION) {
            /* MSDN says this modifier can be safely ignored, so that's what we'll do. */
        }
    }
}

static inline const char* shader_get_comp_op(
    const DWORD opcode) {

    DWORD op = (opcode & INST_CONTROLS_MASK) >> INST_CONTROLS_SHIFT;
    switch (op) {
        case COMPARISON_GT: return ">";
        case COMPARISON_EQ: return "==";
        case COMPARISON_GE: return ">=";
        case COMPARISON_LT: return "<";
        case COMPARISON_NE: return "!=";
        case COMPARISON_LE: return "<=";
        default:
            FIXME("Unrecognized comparison value: %u\n", op);
            return "(\?\?)";
    }
}

static void shader_glsl_get_sample_function(DWORD sampler_type, BOOL projected, glsl_sample_function_t *sample_function) {
    /* Note that there's no such thing as a projected cube texture. */
    switch(sampler_type) {
        case WINED3DSTT_1D:
            sample_function->name = projected ? "texture1DProj" : "texture1D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0;
            break;
        case WINED3DSTT_2D:
            sample_function->name = projected ? "texture2DProj" : "texture2D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;
            break;
        case WINED3DSTT_CUBE:
            sample_function->name = "textureCube";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            break;
        case WINED3DSTT_VOLUME:
            sample_function->name = projected ? "texture3DProj" : "texture3D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            break;
        default:
            sample_function->name = "";
            FIXME("Unrecognized sampler type: %#x;\n", sampler_type);
            break;
    }
}


/*****************************************************************************
 * 
 * Begin processing individual instruction opcodes
 * 
 ****************************************************************************/

/* Generate GLSL arithmetic functions (dst = src1 + src2) */
void shader_glsl_arith(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD write_mask;
    char op;

    /* Determine the GLSL operator to use based on the opcode */
    switch (curOpcode->opcode) {
        case WINED3DSIO_MUL: op = '*'; break;
        case WINED3DSIO_ADD: op = '+'; break;
        case WINED3DSIO_SUB: op = '-'; break;
        default:
            op = ' ';
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
    shader_addline(buffer, "%s %c %s);\n", src0_param.param_str, op, src1_param.param_str);
}

/* Process the WINED3DSIO_MOV opcode using GLSL (dst = src) */
void shader_glsl_mov(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src0_param);

    /* In vs_1_1 WINED3DSIO_MOV can write to the address register. In later
     * shader versions WINED3DSIO_MOVA is used for this. */
    if ((WINED3DSHADER_VERSION_MAJOR(shader->baseShader.hex_version) == 1 &&
            !shader_is_pshader_version(shader->baseShader.hex_version) &&
            shader_get_regtype(arg->dst) == WINED3DSPR_ADDR) ||
            arg->opcode->opcode == WINED3DSIO_MOVA) {
        /* We need to *round* to the nearest int here. */
        size_t mask_size = shader_glsl_get_write_mask_size(write_mask);
        if (mask_size > 1) {
            shader_addline(buffer, "ivec%d(floor(abs(%s) + vec%d(0.5)) * sign(%s)));\n", mask_size, src0_param.param_str, mask_size, src0_param.param_str);
        } else {
            shader_addline(buffer, "int(floor(abs(%s) + 0.5) * sign(%s)));\n", src0_param.param_str, src0_param.param_str);
        }
    } else {
        shader_addline(buffer, "%s);\n", src0_param.param_str);
    }
}

/* Process the dot product operators DP3 and DP4 in GLSL (dst = dot(src0, src1)) */
void shader_glsl_dot(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD dst_write_mask, src_write_mask;
    size_t dst_size = 0;

    dst_write_mask = shader_glsl_append_dst(buffer, arg);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    /* dp3 works on vec3, dp4 on vec4 */
    if (curOpcode->opcode == WINED3DSIO_DP4) {
        src_write_mask = WINED3DSP_WRITEMASK_ALL;
    } else {
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    }

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_write_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_write_mask, &src1_param);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(dot(%s, %s)));\n", dst_size, src0_param.param_str, src1_param.param_str);
    } else {
        shader_addline(buffer, "dot(%s, %s));\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Note that this instruction has some restrictions. The destination write mask
 * can't contain the w component, and the source swizzles have to be .xyzw */
void shader_glsl_cross(SHADER_OPCODE_ARG *arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    char dst_mask[6];

    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_mask, &src1_param);
    shader_addline(arg->buffer, "cross(%s, %s).%s);\n", src0_param.param_str, src1_param.param_str, dst_mask);
}

/* Process the WINED3DSIO_POW instruction in GLSL (dst = |src0|^src1)
 * Src0 and src1 are scalars. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
void shader_glsl_pow(SHADER_OPCODE_ARG *arg) {
    SHADER_BUFFER *buffer = arg->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD dst_write_mask;
    size_t dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, arg);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(pow(abs(%s), %s)));\n", dst_size, src0_param.param_str, src1_param.param_str);
    } else {
        shader_addline(buffer, "pow(abs(%s), %s));\n", src0_param.param_str, src1_param.param_str);
    }
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
void shader_glsl_map2gl(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src_param;
    const char *instruction;
    char arguments[256];
    DWORD write_mask;
    unsigned i;

    /* Determine the GLSL function to use based on the opcode */
    /* TODO: Possibly make this a table for faster lookups */
    switch (curOpcode->opcode) {
        case WINED3DSIO_MIN: instruction = "min"; break;
        case WINED3DSIO_MAX: instruction = "max"; break;
        case WINED3DSIO_ABS: instruction = "abs"; break;
        case WINED3DSIO_FRC: instruction = "fract"; break;
        case WINED3DSIO_NRM: instruction = "normalize"; break;
        case WINED3DSIO_LOGP:
        case WINED3DSIO_LOG: instruction = "log2"; break;
        case WINED3DSIO_EXP: instruction = "exp2"; break;
        case WINED3DSIO_SGN: instruction = "sign"; break;
        case WINED3DSIO_DSX: instruction = "dFdx"; break;
        case WINED3DSIO_DSY: instruction = "dFdy"; break;
        default: instruction = "";
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, arg);

    arguments[0] = '\0';
    if (curOpcode->num_params > 0) {
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src_param);
        strcat(arguments, src_param.param_str);
        for (i = 2; i < curOpcode->num_params; ++i) {
            strcat(arguments, ", ");
            shader_glsl_add_src_param(arg, arg->src[i-1], arg->src_addr[i-1], write_mask, &src_param);
            strcat(arguments, src_param.param_str);
        }
    }

    shader_addline(buffer, "%s(%s));\n", instruction, arguments);
}

/** Process the WINED3DSIO_EXPP instruction in GLSL:
 * For shader model 1.x, do the following (and honor the writemask, so use a temporary variable):
 *   dst.x = 2^(floor(src))
 *   dst.y = src - floor(src)
 *   dst.z = 2^src   (partial precision is allowed, but optional)
 *   dst.w = 1.0;
 * For 2.0 shaders, just do this (honoring writemask and swizzle):
 *   dst = 2^src;    (partial precision is allowed, but optional)
 */
void shader_glsl_expp(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)arg->shader;
    glsl_src_param_t src_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src_param);

    if (shader->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
        char dst_mask[6];

        shader_addline(arg->buffer, "tmp0.x = exp2(floor(%s));\n", src_param.param_str);
        shader_addline(arg->buffer, "tmp0.y = %s - floor(%s);\n", src_param.param_str, src_param.param_str);
        shader_addline(arg->buffer, "tmp0.z = exp2(%s);\n", src_param.param_str);
        shader_addline(arg->buffer, "tmp0.w = 1.0;\n");

        shader_glsl_append_dst(arg->buffer, arg);
        shader_glsl_get_write_mask(arg->dst, dst_mask);
        shader_addline(arg->buffer, "tmp0%s);\n", dst_mask);
    } else {
        DWORD write_mask;
        size_t mask_size;

        write_mask = shader_glsl_append_dst(arg->buffer, arg);
        mask_size = shader_glsl_get_write_mask_size(write_mask);

        if (mask_size > 1) {
            shader_addline(arg->buffer, "vec%d(exp2(%s)));\n", mask_size, src_param.param_str);
        } else {
            shader_addline(arg->buffer, "exp2(%s));\n", src_param.param_str);
        }
    }
}

/** Process the RCP (reciprocal or inverse) opcode in GLSL (dst = 1 / src) */
void shader_glsl_rcp(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src_param;
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(1.0 / %s));\n", mask_size, src_param.param_str);
    } else {
        shader_addline(arg->buffer, "1.0 / %s);\n", src_param.param_str);
    }
}

void shader_glsl_rsq(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src_param;
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1) {
        shader_addline(buffer, "vec%d(inversesqrt(%s)));\n", mask_size, src_param.param_str);
    } else {
        shader_addline(buffer, "inversesqrt(%s));\n", src_param.param_str);
    }
}

/** Process signed comparison opcodes in GLSL. */
void shader_glsl_compare(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);

    if (mask_size > 1) {
        const char *compare;

        switch(arg->opcode->opcode) {
            case WINED3DSIO_SLT: compare = "lessThan"; break;
            case WINED3DSIO_SGE: compare = "greaterThanEqual"; break;
            default: compare = "";
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }

        shader_addline(arg->buffer, "vec%d(%s(%s, %s)));\n", mask_size, compare,
                src0_param.param_str, src1_param.param_str);
    } else {
        const char *compare;

        switch(arg->opcode->opcode) {
            case WINED3DSIO_SLT: compare = "<"; break;
            case WINED3DSIO_SGE: compare = ">="; break;
            default: compare = "";
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }

        shader_addline(arg->buffer, "(%s %s %s) ? 1.0 : 0.0);\n",
                src0_param.param_str, compare, src1_param.param_str);
    }
}

/** Process CMP instruction in GLSL (dst = src0 >= 0.0 ? src1 : src2), per channel */
void shader_glsl_cmp(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask, cmp_channel = 0;
    unsigned int i, j;

    /* Cycle through all source0 channels */
    for (i=0; i<4; i++) {
        write_mask = 0;
        /* Find the destination channels which use the current source0 channel */
        for (j=0; j<4; j++) {
            if ( ((arg->src[0] >> (WINED3DSP_SWIZZLE_SHIFT + 2*j)) & 0x3) == i ) {
                write_mask |= WINED3DSP_WRITEMASK_0 << j;
                cmp_channel = WINED3DSP_WRITEMASK_0 << j;
            }
        }
        write_mask = shader_glsl_append_dst_ext(arg->buffer, arg, arg->dst & (~WINED3DSP_SWIZZLE_MASK | write_mask));
        if (!write_mask) continue;

        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], cmp_channel, &src0_param);
        shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
        shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);

        shader_addline(arg->buffer, "%s >= 0.0 ? %s : %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
    }
}

/** Process the CND opcode in GLSL (dst = (src0 > 0.5) ? src1 : src2) */
/* For ps 1.1-1.3, only a single component of src0 is used. For ps 1.4
 * the compare is done per component of src0. */
void shader_glsl_cnd(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask, cmp_channel = 0;
    unsigned int i, j;

    if (shader->baseShader.hex_version < WINED3DPS_VERSION(1, 4)) {
        write_mask = shader_glsl_append_dst(arg->buffer, arg);
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
        shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
        shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);
        shader_addline(arg->buffer, "%s > 0.5 ? %s : %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
        return;
    }
    /* Cycle through all source0 channels */
    for (i=0; i<4; i++) {
        write_mask = 0;
        /* Find the destination channels which use the current source0 channel */
        for (j=0; j<4; j++) {
            if ( ((arg->src[0] >> (WINED3DSP_SWIZZLE_SHIFT + 2*j)) & 0x3) == i ) {
                write_mask |= WINED3DSP_WRITEMASK_0 << j;
                cmp_channel = WINED3DSP_WRITEMASK_0 << j;
            }
        }
        write_mask = shader_glsl_append_dst_ext(arg->buffer, arg, arg->dst & (~WINED3DSP_SWIZZLE_MASK | write_mask));
        if (!write_mask) continue;

        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], cmp_channel, &src0_param);
        shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
        shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);

        shader_addline(arg->buffer, "%s > 0.5 ? %s : %s);\n",
                src0_param.param_str, src1_param.param_str, src2_param.param_str);
    }
}

/** GLSL code generation for WINED3DSIO_MAD: Multiply the first 2 opcodes, then add the last */
void shader_glsl_mad(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);
    shader_addline(arg->buffer, "(%s * %s) + %s);\n",
            src0_param.param_str, src1_param.param_str, src2_param.param_str);
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for 
    Vertex shaders to GLSL codes */
void shader_glsl_mnxn(SHADER_OPCODE_ARG* arg) {
    int i;
    int nComponents = 0;
    SHADER_OPCODE_ARG tmpArg;
   
    memset(&tmpArg, 0, sizeof(SHADER_OPCODE_ARG));

    /* Set constants for the temporary argument */
    tmpArg.shader      = arg->shader;
    tmpArg.buffer      = arg->buffer;
    tmpArg.src[0]      = arg->src[0];
    tmpArg.src_addr[0] = arg->src_addr[0];
    tmpArg.src_addr[1] = arg->src_addr[1];
    tmpArg.reg_maps = arg->reg_maps; 
    
    switch(arg->opcode->opcode) {
        case WINED3DSIO_M4x4:
            nComponents = 4;
            tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
            break;
        case WINED3DSIO_M4x3:
            nComponents = 3;
            tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
            break;
        case WINED3DSIO_M3x4:
            nComponents = 4;
            tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
            break;
        case WINED3DSIO_M3x3:
            nComponents = 3;
            tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
            break;
        case WINED3DSIO_M3x2:
            nComponents = 2;
            tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
            break;
        default:
            break;
    }

    for (i = 0; i < nComponents; i++) {
        tmpArg.dst = ((arg->dst) & ~WINED3DSP_WRITEMASK_ALL)|(WINED3DSP_WRITEMASK_0<<i);
        tmpArg.src[1]      = arg->src[1]+i;
        shader_glsl_dot(&tmpArg);
    }
}

/**
    The LRP instruction performs a component-wise linear interpolation 
    between the second and third operands using the first operand as the
    blend factor.  Equation:  (dst = src2 + src0 * (src1 - src2))
    This is equivalent to mix(src2, src1, src0);
*/
void shader_glsl_lrp(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);

    shader_addline(arg->buffer, "mix(%s, %s, %s));\n",
            src2_param.param_str, src1_param.param_str, src0_param.param_str);
}

/** Process the WINED3DSIO_LIT instruction in GLSL:
 * dst.x = dst.w = 1.0
 * dst.y = (src0.x > 0) ? src0.x
 * dst.z = (src0.x > 0) ? ((src0.y > 0) ? pow(src0.y, src.w) : 0) : 0
 *                                        where src.w is clamped at +- 128
 */
void shader_glsl_lit(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src3_param;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &src3_param);

    shader_addline(arg->buffer, "vec4(1.0, (%s > 0.0 ? %s : 0.0), (%s > 0.0 ? ((%s > 0.0) ? pow(%s, clamp(%s, -128.0, 128.0)) : 0.0) : 0.0), 1.0)%s);\n",
        src0_param.param_str, src0_param.param_str, src0_param.param_str, src1_param.param_str, src1_param.param_str, src3_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
void shader_glsl_dst(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0y_param;
    glsl_src_param_t src0z_param;
    glsl_src_param_t src1y_param;
    glsl_src_param_t src1w_param;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_1, &src0y_param);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_2, &src0z_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_1, &src1y_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_3, &src1w_param);

    shader_addline(arg->buffer, "vec4(1.0, %s * %s, %s, %s))%s;\n",
            src0y_param.param_str, src1y_param.param_str, src0z_param.param_str, src1w_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_SINCOS instruction in GLSL:
 * VS 2.0 requires that specific cosine and sine constants be passed to this instruction so the hardware
 * can handle it.  But, these functions are built-in for GLSL, so we can just ignore the last 2 params.
 * 
 * dst.x = cos(src0.?)
 * dst.y = sin(src0.?)
 * dst.z = dst.z
 * dst.w = dst.w
 */
void shader_glsl_sincos(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);

    switch (write_mask) {
        case WINED3DSP_WRITEMASK_0:
            shader_addline(arg->buffer, "cos(%s));\n", src0_param.param_str);
            break;

        case WINED3DSP_WRITEMASK_1:
            shader_addline(arg->buffer, "sin(%s));\n", src0_param.param_str);
            break;

        case (WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1):
            shader_addline(arg->buffer, "vec2(cos(%s), sin(%s)));\n", src0_param.param_str, src0_param.param_str);
            break;

        default:
            ERR("Write mask should be .x, .y or .xy\n");
            break;
    }
}

/** Process the WINED3DSIO_LOOP instruction in GLSL:
 * Start a for() loop where src1.y is the initial value of aL,
 *  increment aL by src1.z for a total of src1.x iterations.
 *  Need to use a temporary variable for this operation.
 */
/* FIXME: I don't think nested loops will work correctly this way. */
void shader_glsl_loop(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_ALL, &src1_param);
  
    shader_addline(arg->buffer, "for (tmpInt = 0, aL = %s.y; tmpInt < %s.x; tmpInt++, aL += %s.z) {\n",
            src1_param.reg_name, src1_param.reg_name, src1_param.reg_name);
}

void shader_glsl_end(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "}\n");
}

void shader_glsl_rep(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(arg->buffer, "for (tmpInt = 0; tmpInt < %s; tmpInt++) {\n", src0_param.param_str);
}

void shader_glsl_if(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(arg->buffer, "if (%s) {\n", src0_param.param_str);
}

void shader_glsl_ifc(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(arg->buffer, "if (%s %s %s) {\n",
            src0_param.param_str, shader_get_comp_op(arg->opcode_token), src1_param.param_str);
}

void shader_glsl_else(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "} else {\n");
}

void shader_glsl_break(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "break;\n");
}

/* FIXME: According to MSDN the compare is done per component. */
void shader_glsl_breakc(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(arg->buffer, "if (%s %s %s) break;\n",
            src0_param.param_str, shader_get_comp_op(arg->opcode_token), src1_param.param_str);
}

void shader_glsl_label(SHADER_OPCODE_ARG* arg) {

    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "}\n");
    shader_addline(arg->buffer, "void subroutine%lu () {\n",  snum);
}

void shader_glsl_call(SHADER_OPCODE_ARG* arg) {
    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "subroutine%lu();\n", snum);
}

void shader_glsl_callnz(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src1_param;

    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);
    shader_addline(arg->buffer, "if (%s) subroutine%lu();\n", src1_param.param_str, snum);
}

/*********************************************
 * Pixel Shader Specific Code begins here
 ********************************************/
void pshader_glsl_tex(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD hex_version = This->baseShader.hex_version;
    char dst_swizzle[6];
    glsl_sample_function_t sample_function;
    DWORD sampler_type;
    DWORD sampler_idx;
    BOOL projected;
    DWORD mask = 0;

    /* All versions have a destination register */
    shader_glsl_append_dst(arg->buffer, arg);

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (hex_version < WINED3DPS_VERSION(1,4)) {
        IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
        DWORD flags;

        sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
        flags = deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS];

        if (flags & WINED3DTTFF_PROJECTED) {
            projected = TRUE;
            switch (flags & ~WINED3DTTFF_PROJECTED) {
                case WINED3DTTFF_COUNT1: FIXME("WINED3DTTFF_PROJECTED with WINED3DTTFF_COUNT1?\n"); break;
                case WINED3DTTFF_COUNT2: mask = WINED3DSP_WRITEMASK_1; break;
                case WINED3DTTFF_COUNT3: mask = WINED3DSP_WRITEMASK_2; break;
                case WINED3DTTFF_COUNT4:
                case WINED3DTTFF_DISABLE: mask = WINED3DSP_WRITEMASK_3; break;
            }
        } else {
            projected = FALSE;
        }
    } else if (hex_version < WINED3DPS_VERSION(2,0)) {
        DWORD src_mod = arg->src[0] & WINED3DSP_SRCMOD_MASK;
        sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;

        if (src_mod == WINED3DSPSM_DZ) {
            projected = TRUE;
            mask = WINED3DSP_WRITEMASK_2;
        } else if (src_mod == WINED3DSPSM_DW) {
            projected = TRUE;
            mask = WINED3DSP_WRITEMASK_3;
        } else {
            projected = FALSE;
        }
    } else {
        sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
        if(arg->opcode_token & WINED3DSI_TEXLD_PROJECT) {
                /* ps 2.0 texldp instruction always divides by the fourth component. */
                projected = TRUE;
                mask = WINED3DSP_WRITEMASK_3;
        } else {
            projected = FALSE;
        }
    }

    sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    shader_glsl_get_sample_function(sampler_type, projected, &sample_function);
    mask |= sample_function.coord_mask;

    if (hex_version < WINED3DPS_VERSION(2,0)) {
        shader_glsl_get_write_mask(arg->dst, dst_swizzle);
    } else {
        shader_glsl_get_swizzle(arg->src[1], FALSE, arg->dst, dst_swizzle);
    }

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
    if (hex_version < WINED3DPS_VERSION(1,4)) {
        char coord_mask[6];
        shader_glsl_get_write_mask(mask, coord_mask);
        shader_addline(arg->buffer, "%s(Psampler%u, T%u%s)%s);\n",
                sample_function.name, sampler_idx, sampler_idx, coord_mask, dst_swizzle);
    } else {
        glsl_src_param_t coord_param;
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], mask, &coord_param);
        shader_addline(arg->buffer, "%s(Psampler%u, %s)%s);\n",
                sample_function.name, sampler_idx, coord_param.param_str, dst_swizzle);
    }
}

void shader_glsl_texldl(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*)arg->shader;
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param, lod_param;
    char dst_swizzle[6];
    DWORD sampler_type;
    DWORD sampler_idx;

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_swizzle(arg->src[1], FALSE, arg->dst, dst_swizzle);

    sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
    sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], sample_function.coord_mask, &coord_param);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &lod_param);

    if (shader_is_pshader_version(This->baseShader.hex_version)) {
        /* The GLSL spec claims the Lod sampling functions are only supported in vertex shaders.
         * However, they seem to work just fine in fragment shaders as well. */
        WARN("Using %sLod in fragment shader.\n", sample_function.name);
        shader_addline(arg->buffer, "%sLod(Psampler%u, %s, %s)%s);\n",
                sample_function.name, sampler_idx, coord_param.param_str, lod_param.param_str, dst_swizzle);
    } else {
        shader_addline(arg->buffer, "%sLod(Vsampler%u, %s, %s)%s);\n",
                sample_function.name, sampler_idx, coord_param.param_str, lod_param.param_str, dst_swizzle);
    }
}

void pshader_glsl_texcoord(SHADER_OPCODE_ARG* arg) {

    /* FIXME: Make this work for more than just 2D textures */
    
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;
    DWORD write_mask;
    char dst_mask[6];

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(write_mask, dst_mask);

    if (hex_version != WINED3DPS_VERSION(1,4)) {
        DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "clamp(gl_TexCoord[%u], 0.0, 1.0)%s);\n", reg, dst_mask);
    } else {
        DWORD reg = arg->src[0] & WINED3DSP_REGNUM_MASK;
        DWORD src_mod = arg->src[0] & WINED3DSP_SRCMOD_MASK;
        char dst_swizzle[6];

        shader_glsl_get_swizzle(arg->src[0], FALSE, write_mask, dst_swizzle);

        if (src_mod == WINED3DSPSM_DZ) {
            glsl_src_param_t div_param;
            size_t mask_size = shader_glsl_get_write_mask_size(write_mask);
            shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_2, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        } else if (src_mod == WINED3DSPSM_DW) {
            glsl_src_param_t div_param;
            size_t mask_size = shader_glsl_get_write_mask_size(write_mask);
            shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        } else {
            shader_addline(buffer, "gl_TexCoord[%u]%s);\n", reg, dst_swizzle);
        }
    }
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
void pshader_glsl_texdp3tex(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_addline(arg->buffer, "texture2D(Psampler%u, vec2(dot(gl_TexCoord[%u].xyz, %s), 0.5))%s);\n",
            sampler_idx, sampler_idx, src0_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_TEXDP3 instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
void pshader_glsl_texdp3(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dst_mask;
    size_t mask_size;

    dst_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(dot(T%u.xyz, %s)));\n", mask_size, dstreg, src0_param.param_str);
    } else {
        shader_addline(arg->buffer, "dot(T%u.xyz, %s));\n", dstreg, src0_param.param_str);
    }
}

/** Process the WINED3DSIO_TEXDEPTH instruction in GLSL:
 * Calculate the depth as dst.x / dst.y   */
void pshader_glsl_texdepth(SHADER_OPCODE_ARG* arg) {
    glsl_dst_param_t dst_param;

    shader_glsl_add_dst_param(arg, arg->dst, 0, &dst_param);

    shader_addline(arg->buffer, "gl_FragDepth = %s.x / %s.y;\n", dst_param.reg_name, dst_param.reg_name);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in GLSL:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
void pshader_glsl_texm3x2depth(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_addline(arg->buffer, "tmp0.y = dot(T%u.xyz, %s);\n", dstreg, src0_param.param_str);
    shader_addline(arg->buffer, "gl_FragDepth = (tmp0.y == 0.0) ? 1.0 : clamp(tmp0.x / tmp0.y, 0.0, 1.0);\n");
}

/** Process the WINED3DSIO_TEXM3X2PAD instruction in GLSL
 * Calculate the 1st of a 2-row matrix multiplication. */
void pshader_glsl_texm3x2pad(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
void pshader_glsl_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.%c = dot(T%u.xyz, %s);\n", 'x' + current_state->current_row, reg, src0_param.param_str);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_glsl_texm3x2tex(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;
    char dst_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.y = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    /* Sample the texture using the calculated coordinates */
    shader_addline(buffer, "texture2D(Psampler%u, tmp0.xy)%s);\n", reg, dst_mask);
}

/** Process the WINED3DSIO_TEXM3X3TEX instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply, then sample the texture using the calculated coordinates */
void pshader_glsl_texm3x3tex(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    DWORD sampler_type = arg->reg_maps->samplers[reg] & WINED3DSP_TEXTURETYPE_MASK;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_addline(arg->buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_addline(arg->buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3 instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply */
void pshader_glsl_texm3x3(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_addline(arg->buffer, "vec4(tmp.xy, dot(T%u.xyz, %s), 1.0)%s);\n", reg, src0_param.param_str, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3SPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    char dst_mask[6];
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    DWORD stype = arg->reg_maps->samplers[reg] & WINED3DSP_TEXTURETYPE_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_mask, &src1_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);

    /* Calculate reflection vector, 2*(tmp0.src1)*tmp0-src1
     * This is equivalent to reflect(-src1, tmp0); */
    shader_addline(buffer, "tmp0.xyz = reflect(-(%s), tmp0.xyz);\n", src1_param.param_str);

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_get_sample_function(stype, FALSE, &sample_function);

    /* Sample the texture */
    shader_addline(buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD sampler_type = arg->reg_maps->samplers[reg] & WINED3DSP_TEXTURETYPE_MASK;
    glsl_sample_function_t sample_function;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_param.param_str);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "tmp1.xyz = normalize(vec3(gl_TexCoord[%u].w, gl_TexCoord[%u].w, gl_TexCoord[%u].w));\n",
            current_state->texcoord_w[0], current_state->texcoord_w[1], reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(tmp0.tmp1)*tmp0-tmp1
     * This is equivalent to reflect(-tmp1, tmp0); */
    shader_addline(buffer, "tmp0.xyz = reflect(-tmp1.xyz, tmp0.xyz);\n");

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_addline(buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * texbem is pshader <= 1.3 only, this saves a few version checks
 */
void pshader_glsl_texbem(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    char dst_swizzle[6];
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param;
    DWORD sampler_type;
    DWORD sampler_idx;
    DWORD mask;
    DWORD flags;
    char coord_mask[6];

    sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    flags = deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS];

    sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);
    mask = sample_function.coord_mask;

    shader_glsl_get_write_mask(arg->dst, dst_swizzle);

    shader_glsl_get_write_mask(mask, coord_mask);

    /* with projective textures, texbem only divides the static texture coord, not the displacement,
         * so we can't let the GL handle this.
         */
    if (flags & WINED3DTTFF_PROJECTED) {
        DWORD div_mask=0;
        char coord_div_mask[3];
        switch (flags & ~WINED3DTTFF_PROJECTED) {
            case WINED3DTTFF_COUNT1: FIXME("WINED3DTTFF_PROJECTED with WINED3DTTFF_COUNT1?\n"); break;
            case WINED3DTTFF_COUNT2: div_mask = WINED3DSP_WRITEMASK_1; break;
            case WINED3DTTFF_COUNT3: div_mask = WINED3DSP_WRITEMASK_2; break;
            case WINED3DTTFF_COUNT4:
            case WINED3DTTFF_DISABLE: div_mask = WINED3DSP_WRITEMASK_3; break;
        }
        shader_glsl_get_write_mask(div_mask, coord_div_mask);
        shader_addline(arg->buffer, "T%u%s /= T%u%s;\n", sampler_idx, coord_mask, sampler_idx, coord_div_mask);
    }

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0|WINED3DSP_WRITEMASK_1, &coord_param);
    shader_addline(arg->buffer, "%s(Psampler%u, T%u%s + vec4(bumpenvmat * %s, 0.0, 0.0)%s )%s);\n",
                   sample_function.name, sampler_idx, sampler_idx, coord_mask, coord_param.param_str, coord_mask, dst_swizzle);
}

void pshader_glsl_bem(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param, src1_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0|WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0|WINED3DSP_WRITEMASK_1, &src1_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_addline(arg->buffer, "%s + bumpenvmat * %s);\n",
                   src0_param.param_str, src1_param.param_str);
}

/** Process the WINED3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
void pshader_glsl_texreg2ar(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_addline(arg->buffer, "texture2D(Psampler%u, %s.wx)%s);\n", sampler_idx, src0_param.reg_name, dst_mask);
}

/** Process the WINED3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
void pshader_glsl_texreg2gb(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_ALL, &src0_param);

    shader_addline(arg->buffer, "texture2D(Psampler%u, %s.yz)%s);\n", sampler_idx, src0_param.reg_name, dst_mask);
}

/** Process the WINED3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
void pshader_glsl_texreg2rgb(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    glsl_sample_function_t sample_function;

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], sample_function.coord_mask, &src0_param);

    shader_addline(arg->buffer, "%s(Psampler%u, %s)%s);\n", sample_function.name, sampler_idx, src0_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
void pshader_glsl_texkill(SHADER_OPCODE_ARG* arg) {
    glsl_dst_param_t dst_param;

    shader_glsl_add_dst_param(arg, arg->dst, 0, &dst_param);
    shader_addline(arg->buffer, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;\n", dst_param.reg_name);
}

/** Process the WINED3DSIO_DP2ADD instruction in GLSL.
 * dst = dot2(src0, src1) + src2 */
void pshader_glsl_dp2add(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], WINED3DSP_WRITEMASK_0, &src2_param);

    shader_addline(arg->buffer, "dot(%s, %s) + %s);\n", src0_param.param_str, src1_param.param_str, src2_param.param_str);
}

void pshader_glsl_input_pack(
   SHADER_BUFFER* buffer,
   semantic* semantics_in) {

   unsigned int i;

   for (i = 0; i < MAX_REG_INPUT; i++) {

       DWORD usage_token = semantics_in[i].usage;
       DWORD register_token = semantics_in[i].reg;
       DWORD usage, usage_idx;
       char reg_mask[6];

       /* Uninitialized */
       if (!usage_token) continue;
       usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_write_mask(register_token, reg_mask);

       switch(usage) {

           case WINED3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "IN%u%s = vec4(gl_Color)%s;\n",
                       i, reg_mask, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "IN%u%s = vec4(gl_SecondaryColor)%s;\n",
                       i, reg_mask, reg_mask);
               else
                   shader_addline(buffer, "IN%u%s = vec4(unsupported_color_input)%s;\n",
                       i, reg_mask, reg_mask);
               break;

           case WINED3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "IN%u%s = vec4(gl_TexCoord[%u])%s;\n",
                   i, reg_mask, usage_idx, reg_mask );
               break;

           case WINED3DDECLUSAGE_FOG:
               shader_addline(buffer, "IN%u%s = vec4(gl_FogFragCoord)%s;\n",
                   i, reg_mask, reg_mask);
               break;

           default:
               shader_addline(buffer, "IN%u%s = vec4(unsupported_input)%s;\n",
                   i, reg_mask, reg_mask);
        }
    }
}

/*********************************************
 * Vertex Shader Specific Code begins here
 ********************************************/

void vshader_glsl_output_unpack(
   SHADER_BUFFER* buffer,
   semantic* semantics_out) {

   unsigned int i;

   for (i = 0; i < MAX_REG_OUTPUT; i++) {

       DWORD usage_token = semantics_out[i].usage;
       DWORD register_token = semantics_out[i].reg;
       DWORD usage, usage_idx;
       char reg_mask[6];

       /* Uninitialized */
       if (!usage_token) continue;

       usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_write_mask(register_token, reg_mask);

       switch(usage) {

           case WINED3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "gl_FrontColor%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "gl_FrontSecondaryColor%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               else
                   shader_addline(buffer, "unsupported_color_output%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               break;

           case WINED3DDECLUSAGE_POSITION:
               shader_addline(buffer, "gl_Position%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               break;

           case WINED3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "gl_TexCoord[%u]%s = OUT%u%s;\n",
                   usage_idx, reg_mask, i, reg_mask);
               break;

           case WINED3DDECLUSAGE_PSIZE:
               shader_addline(buffer, "gl_PointSize = OUT%u.x;\n", i);
               break;

           case WINED3DDECLUSAGE_FOG:
               shader_addline(buffer, "gl_FogFragCoord = OUT%u%s;\n", i, reg_mask);
               break;

           default:
               shader_addline(buffer, "unsupported_output%s = OUT%u%s;\n", reg_mask, i, reg_mask);
       }
    }
}

static void add_glsl_program_entry(IWineD3DDeviceImpl *device, struct glsl_shader_prog_link *entry) {
    glsl_program_key_t *key;

    key = HeapAlloc(GetProcessHeap(), 0, sizeof(glsl_program_key_t));
    key->vshader = entry->vshader;
    key->pshader = entry->pshader;

    hash_table_put(device->glsl_program_lookup, key, entry);
}

static struct glsl_shader_prog_link *get_glsl_program_entry(IWineD3DDeviceImpl *device,
        GLhandleARB vshader, GLhandleARB pshader) {
    glsl_program_key_t key;

    key.vshader = vshader;
    key.pshader = pshader;

    return (struct glsl_shader_prog_link *)hash_table_get(device->glsl_program_lookup, &key);
}

void delete_glsl_program_entry(IWineD3DDevice *iface, struct glsl_shader_prog_link *entry) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    glsl_program_key_t *key;

    key = HeapAlloc(GetProcessHeap(), 0, sizeof(glsl_program_key_t));
    key->vshader = entry->vshader;
    key->pshader = entry->pshader;
    hash_table_remove(This->glsl_program_lookup, key);

    GL_EXTCALL(glDeleteObjectARB(entry->programId));
    if (entry->vshader) list_remove(&entry->vshader_entry);
    if (entry->pshader) list_remove(&entry->pshader_entry);
    HeapFree(GetProcessHeap(), 0, entry->vuniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry->puniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry);
}

/** Sets the GLSL program ID for the given pixel and vertex shader combination.
 * It sets the programId on the current StateBlock (because it should be called
 * inside of the DrawPrimitive() part of the render loop).
 *
 * If a program for the given combination does not exist, create one, and store
 * the program in the hash table.  If it creates a program, it will link the
 * given objects, too.
 */
static void set_glsl_shader_program(IWineD3DDevice *iface, BOOL use_ps, BOOL use_vs) {
    IWineD3DDeviceImpl *This               = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info               = &This->adapter->gl_info;
    IWineD3DPixelShader  *pshader          = This->stateBlock->pixelShader;
    IWineD3DVertexShader *vshader          = This->stateBlock->vertexShader;
    struct glsl_shader_prog_link *entry    = NULL;
    GLhandleARB programId                  = 0;
    int i;
    char glsl_name[8];

    GLhandleARB vshader_id = use_vs ? ((IWineD3DBaseShaderImpl*)vshader)->baseShader.prgId : 0;
    GLhandleARB pshader_id = use_ps ? ((IWineD3DBaseShaderImpl*)pshader)->baseShader.prgId : 0;
    entry = get_glsl_program_entry(This, vshader_id, pshader_id);
    if (entry) {
        This->stateBlock->glsl_program = entry;
        return;
    }

    /* If we get to this point, then no matching program exists, so we create one */
    programId = GL_EXTCALL(glCreateProgramObjectARB());
    TRACE("Created new GLSL shader program %u\n", programId);

    /* Create the entry */
    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(struct glsl_shader_prog_link));
    entry->programId = programId;
    entry->vshader = vshader_id;
    entry->pshader = pshader_id;
    /* Add the hash table entry */
    add_glsl_program_entry(This, entry);

    /* Set the current program */
    This->stateBlock->glsl_program = entry;

    /* Attach GLSL vshader */
    if (vshader_id) {
        int max_attribs = 16;   /* TODO: Will this always be the case? It is at the moment... */
        char tmp_name[10];

        TRACE("Attaching GLSL shader object %u to program %u\n", vshader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, vshader_id));
        checkGLcall("glAttachObjectARB");

        /* Bind vertex attributes to a corresponding index number to match
         * the same index numbers as ARB_vertex_programs (makes loading
         * vertex attributes simpler).  With this method, we can use the
         * exact same code to load the attributes later for both ARB and
         * GLSL shaders.
         *
         * We have to do this here because we need to know the Program ID
         * in order to make the bindings work, and it has to be done prior
         * to linking the GLSL program. */
        for (i = 0; i < max_attribs; ++i) {
             _snprintf(tmp_name, sizeof(tmp_name), "attrib%i", i);
             GL_EXTCALL(glBindAttribLocationARB(programId, i, tmp_name));
        }
        checkGLcall("glBindAttribLocationARB");

        list_add_head(&((IWineD3DBaseShaderImpl *)vshader)->baseShader.linked_programs, &entry->vshader_entry);
    }

    /* Attach GLSL pshader */
    if (pshader_id) {
        TRACE("Attaching GLSL shader object %u to program %u\n", pshader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, pshader_id));
        checkGLcall("glAttachObjectARB");

        list_add_head(&((IWineD3DBaseShaderImpl *)pshader)->baseShader.linked_programs, &entry->pshader_entry);
    }

    /* Link the program */
    TRACE("Linking GLSL shader program %u\n", programId);
    GL_EXTCALL(glLinkProgramARB(programId));
    print_glsl_info_log(&GLINFO_LOCATION, programId);

    entry->vuniformF_locations = HeapAlloc(GetProcessHeap(), 0, sizeof(GLhandleARB) * GL_LIMITS(vshader_constantsF));
    for (i = 0; i < GL_LIMITS(vshader_constantsF); ++i) {
        _snprintf(glsl_name, sizeof(glsl_name), "VC[%i]", i);
        entry->vuniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    entry->puniformF_locations = HeapAlloc(GetProcessHeap(), 0, sizeof(GLhandleARB) * GL_LIMITS(pshader_constantsF));
    for (i = 0; i < GL_LIMITS(pshader_constantsF); ++i) {
        _snprintf(glsl_name, sizeof(glsl_name), "PC[%i]", i);
        entry->puniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
}

static GLhandleARB create_glsl_blt_shader(WineD3D_GL_Info *gl_info) {
    GLhandleARB program_id;
    GLhandleARB vshader_id, pshader_id;
    const char *blt_vshader[] = {
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0].x = (gl_Vertex.x * 0.5) + 0.5;\n"
        "    gl_TexCoord[0].y = (-gl_Vertex.y * 0.5) + 0.5;\n"
        "}\n"
    };

    const char *blt_pshader[] = {
        "uniform sampler2D sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n"
    };

    vshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(vshader_id, 1, blt_vshader, NULL));
    GL_EXTCALL(glCompileShaderARB(vshader_id));

    pshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(pshader_id, 1, blt_pshader, NULL));
    GL_EXTCALL(glCompileShaderARB(pshader_id));

    program_id = GL_EXTCALL(glCreateProgramObjectARB());
    GL_EXTCALL(glAttachObjectARB(program_id, vshader_id));
    GL_EXTCALL(glAttachObjectARB(program_id, pshader_id));
    GL_EXTCALL(glLinkProgramARB(program_id));

    print_glsl_info_log(&GLINFO_LOCATION, program_id);

    return program_id;
}

static void shader_glsl_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    GLhandleARB program_id = 0;

    if (useVS || usePS) set_glsl_shader_program(iface, usePS, useVS);
    else This->stateBlock->glsl_program = NULL;

    program_id = This->stateBlock->glsl_program ? This->stateBlock->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);
    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
}

static void shader_glsl_select_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    static GLhandleARB program_id = 0;
    static GLhandleARB loc = -1;

    if (!program_id) {
        program_id = create_glsl_blt_shader(gl_info);
        loc = GL_EXTCALL(glGetUniformLocationARB(program_id, "sampler"));
    }

    GL_EXTCALL(glUseProgramObjectARB(program_id));
    GL_EXTCALL(glUniform1iARB(loc, 0));
}

static void shader_glsl_cleanup(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    GL_EXTCALL(glUseProgramObjectARB(0));
}

const shader_backend_t glsl_shader_backend = {
    &shader_glsl_select,
    &shader_glsl_select_depth_blt,
    &shader_glsl_load_constants,
    &shader_glsl_cleanup
};
