/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green 
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define GLINFO_LOCATION      (*gl_info)

typedef struct {
    char reg_name[150];
    char mask_str[6];
} glsl_dst_param_t;

typedef struct {
    char reg_name[150];
    char param_str[100];
} glsl_src_param_t;

typedef struct {
    const char *name;
    DWORD coord_mask;
} glsl_sample_function_t;

/* GLSL shader private data */
struct shader_glsl_priv {
    struct hash_table_t *glsl_program_lookup;
    struct glsl_shader_prog_link *glsl_program;
    GLhandleARB depth_blt_program[tex_type_count];
};

/* Struct to maintain data about a linked GLSL program */
struct glsl_shader_prog_link {
    struct list             vshader_entry;
    struct list             pshader_entry;
    GLhandleARB             programId;
    GLhandleARB             *vuniformF_locations;
    GLhandleARB             *puniformF_locations;
    GLhandleARB             vuniformI_locations[MAX_CONST_I];
    GLhandleARB             puniformI_locations[MAX_CONST_I];
    GLhandleARB             posFixup_location;
    GLhandleARB             bumpenvmat_location[MAX_TEXTURES];
    GLhandleARB             luminancescale_location[MAX_TEXTURES];
    GLhandleARB             luminanceoffset_location[MAX_TEXTURES];
    GLhandleARB             srgb_comparison_location;
    GLhandleARB             srgb_mul_low_location;
    GLhandleARB             ycorrection_location;
    GLenum                  vertex_color_clamp;
    GLhandleARB             vshader;
    GLhandleARB             pshader;
};

typedef struct {
    GLhandleARB vshader;
    GLhandleARB pshader;
} glsl_program_key_t;


/** Prints the GLSL info log which will contain error messages if they exist */
static void print_glsl_info_log(WineD3D_GL_Info *gl_info, GLhandleARB obj) {
    int infologLength = 0;
    char *infoLog;
    int i;
    BOOL is_spam;

    const char *spam[] = {
        "Vertex shader was successfully compiled to run on hardware.\n",    /* fglrx          */
        "Fragment shader was successfully compiled to run on hardware.\n",  /* fglrx          */
        "Fragment shader(s) linked, vertex shader(s) linked. \n ",          /* fglrx, with \n */
        "Fragment shader(s) linked, vertex shader(s) linked.",              /* fglrx, no \n   */
        "Vertex shader(s) linked, no fragment shader(s) defined. \n ",      /* fglrx, with \n */
        "Vertex shader(s) linked, no fragment shader(s) defined.",          /* fglrx, no \n   */
        "Fragment shader was successfully compiled to run on hardware.\nWARNING: 0:1: extension 'GL_ARB_draw_buffers' is not supported",
        "Fragment shader(s) linked, no vertex shader(s) defined.",          /* fglrx, no \n   */
        "Fragment shader(s) linked, no vertex shader(s) defined. \n ",      /* fglrx, with \n */
        "WARNING: 0:2: extension 'GL_ARB_draw_buffers' is not supported\n"  /* MacOS ati      */
    };

    GL_EXTCALL(glGetObjectParameterivARB(obj,
               GL_OBJECT_INFO_LOG_LENGTH_ARB,
               &infologLength));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (infologLength > 1)
    {
        /* Fglrx doesn't terminate the string properly, but it tells us the proper length.
         * So use HEAP_ZERO_MEMORY to avoid uninitialized bytes
         */
        infoLog = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, infologLength);
        GL_EXTCALL(glGetInfoLogARB(obj, infologLength, NULL, infoLog));
        is_spam = FALSE;

        for(i = 0; i < sizeof(spam) / sizeof(spam[0]); i++) {
            if(strcmp(infoLog, spam[i]) == 0) {
                is_spam = TRUE;
                break;
            }
        }
        if(is_spam) {
            TRACE("Spam received from GLSL shader #%u: %s\n", obj, debugstr_a(infoLog));
        } else {
            FIXME("Error received from GLSL shader #%u: %s\n", obj, debugstr_a(infoLog));
        }
        HeapFree(GetProcessHeap(), 0, infoLog);
    }
}

/**
 * Loads (pixel shader) samplers
 */
static void shader_glsl_load_psamplers(
    WineD3D_GL_Info *gl_info,
    IWineD3DStateBlock* iface,
    GLhandleARB programId) {

    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    GLhandleARB name_loc;
    int i;
    char sampler_name[20];

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
        snprintf(sampler_name, sizeof(sampler_name), "Psampler%d", i);
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

static void shader_glsl_load_vsamplers(WineD3D_GL_Info *gl_info, IWineD3DStateBlock* iface, GLhandleARB programId) {
    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    GLhandleARB name_loc;
    char sampler_name[20];
    int i;

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        snprintf(sampler_name, sizeof(sampler_name), "Vsampler%d", i);
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
    DWORD i, j, k;
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

    /* 1.X pshaders have the constants clamped to [-1;1] implicitly. */
    if(WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1 &&
       shader_is_pshader_version(This->baseShader.hex_version)) {
        float lcl_const[4];

        LIST_FOR_EACH_ENTRY(constant, constant_list, constants_entry, entry) {
            idx = constant->idx;
            j = constant->count;
            while (j--) {
                i = *idx++;
                tmp_loc = constant_locations[i];
                if (tmp_loc != -1) {
                    /* We found this uniform name in the program - go ahead and send the data */
                    k = i * 4;
                    if(constants[k + 0] < -1.0) lcl_const[0] = -1.0;
                    else if(constants[k + 0] > 1.0) lcl_const[0] = 1.0;
                    else lcl_const[0] = constants[k + 0];
                    if(constants[k + 1] < -1.0) lcl_const[1] = -1.0;
                    else if(constants[k + 1] > 1.0) lcl_const[1] = 1.0;
                    else lcl_const[1] = constants[k + 1];
                    if(constants[k + 2] < -1.0) lcl_const[2] = -1.0;
                    else if(constants[k + 2] > 1.0) lcl_const[2] = 1.0;
                    else lcl_const[2] = constants[k + 2];
                    if(constants[k + 3] < -1.0) lcl_const[3] = -1.0;
                    else if(constants[k + 3] > 1.0) lcl_const[3] = 1.0;
                    else lcl_const[3] = constants[k + 3];

                    GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, lcl_const));
                }
            }
        }
    } else {
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
    }
    checkGLcall("glUniform4fvARB()");

    if(!This->baseShader.load_local_constsF) {
        TRACE("No need to load local float constants for this shader\n");
        return;
    }

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
    /* Immediate constants are clamped to [-1;1] at shader creation time if needed */
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
    GLhandleARB locations[MAX_CONST_I],
    unsigned max_constants,
    int* constants,
    BOOL* constants_set) {
    
    int i;
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE_(d3d_constants)("Loading constants %i: %i, %i, %i, %i\n",
                  i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4ivARB(locations[i], 1, &constants[i*4]));
            checkGLcall("glUniform4ivARB");
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

        /* We found this uniform name in the program - go ahead and send the data */
        GL_EXTCALL(glUniform4ivARB(locations[idx], 1, values));
        checkGLcall("glUniform4ivARB");
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

            TRACE_(d3d_constants)("Loading constants %i: %i;\n", i, constants[i]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, &constants[i]));
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

        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
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
static void shader_glsl_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {
   
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) device;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *)deviceImpl->shader_priv;
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;

    GLhandleARB *constant_locations;
    struct list *constant_list;
    GLhandleARB programId;
    struct glsl_shader_prog_link *prog = priv->glsl_program;
    unsigned int i;

    if (!prog) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    programId = prog->programId;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;

        constant_locations = prog->vuniformF_locations;
        constant_list = &stateBlock->set_vconstantsF;

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, GL_LIMITS(vshader_constantsF),
                stateBlock->vertexShaderConstantF, constant_locations, constant_list);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, programId,
                                    prog->vuniformI_locations, MAX_CONST_I,
                                    stateBlock->vertexShaderConstantI,
                                    stateBlock->changed.vertexShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->vertexShaderConstantB,
                                    stateBlock->changed.vertexShaderConstantsB);

        /* Upload the position fixup params */
        GL_EXTCALL(glUniform4fvARB(prog->posFixup_location, 1, &deviceImpl->posFixup[0]));
        checkGLcall("glUniform4fvARB");
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        constant_locations = prog->puniformF_locations;
        constant_list = &stateBlock->set_pconstantsF;

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, GL_LIMITS(pshader_constantsF),
                stateBlock->pixelShaderConstantF, constant_locations, constant_list);

        /* Load DirectX 9 integer constants/uniforms for pixel shader */
        shader_glsl_load_constantsI(pshader, gl_info, programId,
                                    prog->puniformI_locations, MAX_CONST_I,
                                    stateBlock->pixelShaderConstantI, 
                                    stateBlock->changed.pixelShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for pixel shader */
        shader_glsl_load_constantsB(pshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->pixelShaderConstantB, 
                                    stateBlock->changed.pixelShaderConstantsB);

        /* Upload the environment bump map matrix if needed. The needsbumpmat member specifies the texture stage to load the matrix from.
         * It can't be 0 for a valid texbem instruction.
         */
        for(i = 0; i < ((IWineD3DPixelShaderImpl *) pshader)->numbumpenvmatconsts; i++) {
            IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) pshader;
            int stage = ps->luminanceconst[i].texunit;

            float *data = (float *) &stateBlock->textureState[(int) ps->bumpenvmatconst[i].texunit][WINED3DTSS_BUMPENVMAT00];
            GL_EXTCALL(glUniformMatrix2fvARB(prog->bumpenvmat_location[i], 1, 0, data));
            checkGLcall("glUniformMatrix2fvARB");

            /* texbeml needs the luminance scale and offset too. If texbeml is used, needsbumpmat
             * is set too, so we can check that in the needsbumpmat check
             */
            if(ps->baseShader.reg_maps.luminanceparams[stage]) {
                GLfloat *scale = (GLfloat *) &stateBlock->textureState[stage][WINED3DTSS_BUMPENVLSCALE];
                GLfloat *offset = (GLfloat *) &stateBlock->textureState[stage][WINED3DTSS_BUMPENVLOFFSET];

                GL_EXTCALL(glUniform1fvARB(prog->luminancescale_location[i], 1, scale));
                checkGLcall("glUniform1fvARB");
                GL_EXTCALL(glUniform1fvARB(prog->luminanceoffset_location[i], 1, offset));
                checkGLcall("glUniform1fvARB");
            }
        }

        if(((IWineD3DPixelShaderImpl *) pshader)->srgb_enabled &&
                  !((IWineD3DPixelShaderImpl *) pshader)->srgb_mode_hardcoded) {
            float comparison[4];
            float mul_low[4];

            if(stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE]) {
                comparison[0] = srgb_cmp; comparison[1] = srgb_cmp;
                comparison[2] = srgb_cmp; comparison[3] = srgb_cmp;

                mul_low[0] = srgb_mul_low; mul_low[1] = srgb_mul_low;
                mul_low[2] = srgb_mul_low; mul_low[3] = srgb_mul_low;
            } else {
                comparison[0] = 1.0 / 0.0; comparison[1] = 1.0 / 0.0;
                comparison[2] = 1.0 / 0.0; comparison[3] = 1.0 / 0.0;

                mul_low[0] = 1.0; mul_low[1] = 1.0;
                mul_low[2] = 1.0; mul_low[3] = 1.0;
            }

            GL_EXTCALL(glUniform4fvARB(prog->srgb_comparison_location, 1, comparison));
            GL_EXTCALL(glUniform4fvARB(prog->srgb_mul_low_location, 1, mul_low));
        }
        if(((IWineD3DPixelShaderImpl *) pshader)->vpos_uniform) {
            float correction_params[4];
            if(deviceImpl->render_offscreen) {
                correction_params[0] = 0.0;
                correction_params[1] = 1.0;
            } else {
                /* position is window relative, not viewport relative */
                correction_params[0] = ((IWineD3DSurfaceImpl *) deviceImpl->render_targets[0])->currentDesc.Height;
                correction_params[1] = -1.0;
            }
            GL_EXTCALL(glUniform4fvARB(prog->ycorrection_location, 1, correction_params));
        }
    }
}

/** Generate the variable & register declarations for the GLSL output target */
static void shader_generate_glsl_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
    int i;
    unsigned int extra_constants_needed = 0;
    local_constant* lconst;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char prefix = pshader ? 'P' : 'V';

    /* Prototype the subroutines */
    for (i = 0; i < This->baseShader.limits.label; i++) {
        if (reg_maps->labels[i])
            shader_addline(buffer, "void subroutine%u();\n", i);
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

    if(!pshader) {
        shader_addline(buffer, "uniform vec4 posFixup;\n");
        /* Predeclaration; This function is added at link time based on the pixel shader.
         * VS 3.0 shaders have an array OUT[] the shader writes to, earlier versions don't have
         * that. We know the input to the reorder function at vertex shader compile time, so
         * we can deal with that. The reorder function for a 1.x and 2.x vertex shader can just
         * read gl_FrontColor. The output depends on the pixel shader. The reorder function for a
         * 1.x and 2.x pshader or for fixed function will write gl_FrontColor, and for a 3.0 shader
         * it will write to the varying array. Here we depend on the shader optimizer on sorting that
         * out. The nvidia driver only does that if the parameter is inout instead of out, hence the
         * inout.
         */
        if(This->baseShader.hex_version >= WINED3DVS_VERSION(3, 0)) {
            shader_addline(buffer, "void order_ps_input(in vec4[%u]);\n", MAX_REG_OUTPUT);
        } else {
            shader_addline(buffer, "void order_ps_input();\n");
        }
    } else {
        IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;

        ps_impl->numbumpenvmatconsts = 0;
        for(i = 0; i < (sizeof(reg_maps->bumpmat) / sizeof(reg_maps->bumpmat[0])); i++) {
            if(!reg_maps->bumpmat[i]) {
                continue;
            }

            ps_impl->bumpenvmatconst[(int) ps_impl->numbumpenvmatconsts].texunit = i;
            shader_addline(buffer, "uniform mat2 bumpenvmat%d;\n", i);

            if(reg_maps->luminanceparams) {
                ps_impl->luminanceconst[(int) ps_impl->numbumpenvmatconsts].texunit = i;
                shader_addline(buffer, "uniform float luminancescale%d;\n", i);
                shader_addline(buffer, "uniform float luminanceoffset%d;\n", i);
                extra_constants_needed++;
            } else {
                ps_impl->luminanceconst[(int) ps_impl->numbumpenvmatconsts].texunit = -1;
            }

            extra_constants_needed++;
            ps_impl->numbumpenvmatconsts++;
        }

        if(device->stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE]) {
            ps_impl->srgb_enabled = 1;
            if(This->baseShader.limits.constant_float + extra_constants_needed + 1 < GL_LIMITS(pshader_constantsF)) {
                shader_addline(buffer, "uniform vec4 srgb_mul_low;\n");
                shader_addline(buffer, "uniform vec4 srgb_comparison;\n");
                ps_impl->srgb_mode_hardcoded = 0;
                extra_constants_needed++;
            } else {
                ps_impl->srgb_mode_hardcoded = 1;
                shader_addline(buffer, "const vec4 srgb_mul_low = vec4(%f, %f, %f, %f);\n",
                               srgb_mul_low, srgb_mul_low, srgb_mul_low, srgb_mul_low);
                shader_addline(buffer, "const vec4 srgb_comparison = vec4(%f, %f, %f, %f);\n",
                               srgb_cmp, srgb_cmp, srgb_cmp, srgb_cmp);
            }
        } else {
            IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;

            /* Do not write any srgb fixup into the shader to save shader size and processing time.
             * As a consequence, we can't toggle srgb write on without recompilation
             */
            ps_impl->srgb_enabled = 0;
            ps_impl->srgb_mode_hardcoded = 1;
        }
        if(reg_maps->vpos || reg_maps->usesdsy) {
            if(This->baseShader.limits.constant_float + extra_constants_needed + 1 < GL_LIMITS(pshader_constantsF)) {
                shader_addline(buffer, "uniform vec4 ycorrection;\n");
                ((IWineD3DPixelShaderImpl *) This)->vpos_uniform = 1;
                extra_constants_needed++;
            } else {
                /* This happens because we do not have proper tracking of the constant registers that are
                 * actually used, only the max limit of the shader version
                 */
                FIXME("Cannot find a free uniform for vpos correction params\n");
                shader_addline(buffer, "const vec4 ycorrection = vec4(%f, %f, 0.0, 0.0);\n",
                               device->render_offscreen ? 0.0 : ((IWineD3DSurfaceImpl *) device->render_targets[0])->currentDesc.Height,
                               device->render_offscreen ? 1.0 : -1.0);
            }
            shader_addline(buffer, "vec4 vpos;\n");
        }
    }

    /* Declare texture samplers */ 
    for (i = 0; i < This->baseShader.limits.sampler; i++) {
        if (reg_maps->samplers[i]) {

            DWORD stype = reg_maps->samplers[i] & WINED3DSP_TEXTURETYPE_MASK;
            switch (stype) {

                case WINED3DSTT_1D:
                    shader_addline(buffer, "uniform sampler1D %csampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_2D:
                    if(device->stateBlock->textures[i] &&
                       IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[i]) == GL_TEXTURE_RECTANGLE_ARB) {
                        shader_addline(buffer, "uniform sampler2DRect %csampler%u;\n", prefix, i);
                    } else {
                        shader_addline(buffer, "uniform sampler2D %csampler%u;\n", prefix, i);
                    }
                    break;
                case WINED3DSTT_CUBE:
                    shader_addline(buffer, "uniform samplerCube %csampler%u;\n", prefix, i);
                    break;
                case WINED3DSTT_VOLUME:
                    shader_addline(buffer, "uniform sampler3D %csampler%u;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %csampler%u;\n", prefix, i);
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
            shader_addline(buffer, "vec4 T%u = gl_TexCoord[%u];\n", i, i);
    }

    /* Declare input register varyings. Only pixel shader, vertex shaders have that declared in the
     * helper function shader that is linked in at link time
     */
    if(pshader && This->baseShader.hex_version >= WINED3DPS_VERSION(3, 0)) {
        if(use_vs(device)) {
            shader_addline(buffer, "varying vec4 IN[%u];\n", GL_LIMITS(glsl_varyings) / 4);
        } else {
            /* TODO: Write a replacement shader for the fixed function vertex pipeline, so this isn't needed.
             * For fixed function vertex processing + 3.0 pixel shader we need a separate function in the
             * pixel shader that reads the fixed function color into the packed input registers.
             */
            shader_addline(buffer, "vec4 IN[%u];\n", GL_LIMITS(glsl_varyings) / 4);
        }
    }

    /* Declare output register temporaries */
    if(This->baseShader.limits.packed_output) {
        shader_addline(buffer, "vec4 OUT[%u];\n", This->baseShader.limits.packed_output);
    }

    /* Declare temporary variables */
    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "vec4 R%u;\n", i);
    }

    /* Declare attributes */
    for (i = 0; i < This->baseShader.limits.attributes; i++) {
        if (reg_maps->attributes[i])
            shader_addline(buffer, "attribute vec4 attrib%i;\n", i);
    }

    /* Declare loop registers aLx */
    for (i = 0; i < reg_maps->loop_depth; i++) {
        shader_addline(buffer, "int aL%u;\n", i);
        shader_addline(buffer, "int tmpInt%u;\n", i);
    }

    /* Temporary variables for matrix operations */
    shader_addline(buffer, "vec4 tmp0;\n");
    shader_addline(buffer, "vec4 tmp1;\n");

    /* Local constants use a different name so they can be loaded once at shader link time
     * They can't be hardcoded into the shader text via LC = {x, y, z, w}; because the
     * float -> string conversion can cause precision loss.
     */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            shader_addline(buffer, "uniform vec4 %cLC%u;\n", prefix, lconst->idx);
        }
    }

    /* Start the main program */
    shader_addline(buffer, "void main() {\n");
    if(pshader && reg_maps->vpos) {
        /* DirectX apps expect integer values, while OpenGL drivers add approximately 0.5. This causes
         * off-by-one problems as spotted by the vPos d3d9 visual test. Unfortunately the ATI cards do
         * not add exactly 0.5, but rather something like 0.49999999 or 0.50000001, which still causes
         * precision troubles when we just substract 0.5.
         *
         * To deal with that just floor() the position. This will eliminate the fraction on all cards.
         *
         * TODO: Test how that behaves with multisampling once we can enable multisampling in winex11.
         *
         * An advantage of floor is that it works even if the driver doesn't add 1/2. It is somewhat
         * questionable if 1.5, 2.5, ... are the proper values to return in gl_FragCoord, even though
         * coordinates specify the pixel centers instead of the pixel corners. This code will behave
         * correctly on drivers that returns integer values.
         */
        shader_addline(buffer, "vpos = floor(vec4(0, ycorrection[0], 0, 0) + gl_FragCoord * vec4(1, ycorrection[1], 1, 1));\n");
    }
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
    char tmpStr[150];

    *is_color = FALSE;   
 
    switch (regtype) {
    case WINED3DSPR_TEMP:
        sprintf(tmpStr, "R%u", reg);
    break;
    case WINED3DSPR_INPUT:
        if (pshader) {
            /* Pixel shaders >= 3.0 */
            if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3) {
                DWORD in_count = GL_LIMITS(glsl_varyings) / 4;

                if (param & WINED3DSHADER_ADDRMODE_RELATIVE) {
                    glsl_src_param_t rel_param;
                    shader_glsl_add_src_param(arg, addr_token, 0, WINED3DSP_WRITEMASK_0, &rel_param);

                    /* Removing a + 0 would be an obvious optimization, but macos doesn't see the NOP
                     * operation there
                     */
                    if(((IWineD3DPixelShaderImpl *) This)->input_reg_map[reg]) {
                        if (((IWineD3DPixelShaderImpl *)This)->declared_in_count > in_count) {
                            sprintf(tmpStr, "((%s + %u) > %d ? (%s + %u) > %d ? gl_SecondaryColor : gl_Color : IN[%s + %u])",
                                    rel_param.param_str, ((IWineD3DPixelShaderImpl *)This)->input_reg_map[reg], in_count - 1,
                                    rel_param.param_str, ((IWineD3DPixelShaderImpl *)This)->input_reg_map[reg], in_count,
                                    rel_param.param_str, ((IWineD3DPixelShaderImpl *)This)->input_reg_map[reg]);
                        } else {
                            sprintf(tmpStr, "IN[%s + %u]", rel_param.param_str, ((IWineD3DPixelShaderImpl *)This)->input_reg_map[reg]);
                        }
                    } else {
                        if (((IWineD3DPixelShaderImpl *)This)->declared_in_count > in_count) {
                            sprintf(tmpStr, "((%s) > %d ? (%s) > %d ? gl_SecondaryColor : gl_Color : IN[%s])",
                                    rel_param.param_str, in_count - 1,
                                    rel_param.param_str, in_count,
                                    rel_param.param_str);
                        } else {
                            sprintf(tmpStr, "IN[%s]", rel_param.param_str);
                        }
                    }
                } else {
                    DWORD idx = ((IWineD3DPixelShaderImpl *) This)->input_reg_map[reg];
                    if (idx == in_count) {
                        sprintf(tmpStr, "gl_Color");
                    } else if (idx == in_count + 1) {
                        sprintf(tmpStr, "gl_SecondaryColor");
                    } else {
                        sprintf(tmpStr, "IN[%u]", idx);
                    }
                }
            } else {
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
        const char prefix = pshader? 'P':'V';

        /* Relative addressing */
        if (param & WINED3DSHADER_ADDRMODE_RELATIVE) {

           /* Relative addressing on shaders 2.0+ have a relative address token, 
            * prior to that, it was hard-coded as "A0.x" because there's only 1 register */
           if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2)  {
               glsl_src_param_t rel_param;
               shader_glsl_add_src_param(arg, addr_token, 0, WINED3DSP_WRITEMASK_0, &rel_param);
               if(reg) {
                   sprintf(tmpStr, "%cC[%s + %u]", prefix, rel_param.param_str, reg);
               } else {
                   sprintf(tmpStr, "%cC[%s]", prefix, rel_param.param_str);
               }
           } else {
               if(reg) {
                   sprintf(tmpStr, "%cC[A0.x + %u]", prefix, reg);
               } else {
                   sprintf(tmpStr, "%cC[A0.x]", prefix);
               }
           }

        } else {
            if(shader_constant_is_local(This, reg)) {
                sprintf(tmpStr, "%cLC%u", prefix, reg);
            } else {
                sprintf(tmpStr, "%cC[%u]", prefix, reg);
            }
        }

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
        sprintf(tmpStr, "aL%u", This->baseShader.cur_loop_regno - 1);
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
            sprintf(tmpStr, "OUT[%u]", reg);
        else
            sprintf(tmpStr, "gl_TexCoord[%u]", reg);
    break;
    case WINED3DSPR_MISCTYPE:
        if (reg == 0) {
            /* vPos */
            sprintf(tmpStr, "vpos");
        } else if (reg == 1){
            /* Note that gl_FrontFacing is a bool, while vFace is
             * a float for which the sign determines front/back
             */
            sprintf(tmpStr, "(gl_FrontFacing ? 1.0 : -1.0)");
        } else {
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

static unsigned int shader_glsl_get_write_mask_size(DWORD write_mask) {
    unsigned int size = 0;

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

static void shader_glsl_get_sample_function(DWORD sampler_type, BOOL projected, BOOL texrect, glsl_sample_function_t *sample_function) {
    /* Note that there's no such thing as a projected cube texture. */
    switch(sampler_type) {
        case WINED3DSTT_1D:
            sample_function->name = projected ? "texture1DProj" : "texture1D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0;
            break;
        case WINED3DSTT_2D:
            if(texrect) {
                sample_function->name = projected ? "texture2DRectProj" : "texture2DRect";
            } else {
                sample_function->name = projected ? "texture2DProj" : "texture2D";
            }
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
            sample_function->coord_mask = 0;
            FIXME("Unrecognized sampler type: %#x;\n", sampler_type);
            break;
    }
}

static void shader_glsl_color_correction(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) shader->baseShader.device;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;
    glsl_dst_param_t dst_param;
    glsl_dst_param_t dst_param2;
    WINED3DFORMAT fmt;
    WINED3DFORMAT conversion_group;
    IWineD3DBaseTextureImpl *texture;
    DWORD mask, mask_size;
    UINT i;
    BOOL recorded = FALSE;
    DWORD sampler_idx;
    DWORD hex_version = shader->baseShader.hex_version;

    switch(arg->opcode->opcode) {
        case WINED3DSIO_TEX:
            if (hex_version < WINED3DPS_VERSION(2,0)) {
                sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            } else {
                sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
            }
            break;

        case WINED3DSIO_TEXLDL:
            FIXME("Add color fixup for vertex texture WINED3DSIO_TEXLDL\n");
            return;

        case WINED3DSIO_TEXDP3TEX:
        case WINED3DSIO_TEXM3x3TEX:
        case WINED3DSIO_TEXM3x3SPEC:
        case WINED3DSIO_TEXM3x3VSPEC:
        case WINED3DSIO_TEXBEM:
        case WINED3DSIO_TEXREG2AR:
        case WINED3DSIO_TEXREG2GB:
        case WINED3DSIO_TEXREG2RGB:
            sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            break;

        default:
            /* Not a texture sampling instruction, nothing to do */
            return;
    };

    texture = (IWineD3DBaseTextureImpl *) deviceImpl->stateBlock->textures[sampler_idx];
    if(texture) {
        fmt = texture->resource.format;
        conversion_group = texture->baseTexture.shader_conversion_group;
    } else {
        fmt = WINED3DFMT_UNKNOWN;
        conversion_group = WINED3DFMT_UNKNOWN;
    }

    /* before doing anything, record the sampler with the format in the format conversion list,
     * but check if it's not there already
     */
    for(i = 0; i < shader->baseShader.num_sampled_samplers; i++) {
        if(shader->baseShader.sampled_samplers[i] == sampler_idx) {
            recorded = TRUE;
            break;
        }
    }
    if(!recorded) {
        shader->baseShader.sampled_samplers[shader->baseShader.num_sampled_samplers] = sampler_idx;
        shader->baseShader.num_sampled_samplers++;
        shader->baseShader.sampled_format[sampler_idx] = conversion_group;
    }

    switch(fmt) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
            if(GL_SUPPORT(NV_TEXTURE_SHADER) && fmt == WINED3DFMT_V8U8) {
                /* The 3rd channel returns 1.0 in d3d, but 0.0 in gl. Fix this while we're at it :-) */
                mask = shader_glsl_add_dst_param(arg, arg->dst, WINED3DSP_WRITEMASK_2, &dst_param);
                mask_size = shader_glsl_get_write_mask_size(mask);
                if(mask_size >= 3) {
                    shader_addline(arg->buffer, "%s.%c = 1.0;\n", dst_param.reg_name, dst_param.mask_str[3]);
                }
            } else {
                /* Correct the sign, but leave the blue as it is - it was loaded correctly already */
                mask = shader_glsl_add_dst_param(arg, arg->dst,
                                          WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1,
                                          &dst_param);
                mask_size = shader_glsl_get_write_mask_size(mask);
                if(mask_size >= 2) {
                    shader_addline(arg->buffer, "%s.%c%c = %s.%c%c * 2.0 - 1.0;\n",
                    dst_param.reg_name, dst_param.mask_str[1], dst_param.mask_str[2],
                    dst_param.reg_name, dst_param.mask_str[1], dst_param.mask_str[2]);
                } else if(mask_size == 1) {
                    shader_addline(arg->buffer, "%s.%c = %s.%c * 2.0 - 1.0;\n", dst_param.reg_name, dst_param.mask_str[1],
                    dst_param.reg_name, dst_param.mask_str[1]);
                }
            }
            break;

        case WINED3DFMT_X8L8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Red and blue are the signed channels, fix them up; Blue(=L) is correct already,
                 * and a(X) is always 1.0
                 */
                mask = shader_glsl_add_dst_param(arg, arg->dst, WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &dst_param);
                mask_size = shader_glsl_get_write_mask_size(mask);
                if(mask_size >= 2) {
                    shader_addline(arg->buffer, "%s.%c%c = %s.%c%c * 2.0 - 1.0;\n",
                                   dst_param.reg_name, dst_param.mask_str[1], dst_param.mask_str[2],
                                   dst_param.reg_name, dst_param.mask_str[1], dst_param.mask_str[2]);
                } else if(mask_size == 1) {
                    shader_addline(arg->buffer, "%s.%c = %s.%c * 2.0 - 1.0;\n",
                                   dst_param.reg_name, dst_param.mask_str[1],
                                   dst_param.reg_name, dst_param.mask_str[1]);
                }
            }
            break;

        case WINED3DFMT_L6V5U5:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                mask = shader_glsl_add_dst_param(arg, arg->dst, WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &dst_param);
                mask_size = shader_glsl_get_write_mask_size(mask);
                shader_glsl_add_dst_param(arg, arg->dst, WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_2, &dst_param2);
                if(mask_size >= 3) {
                    /* Swap y and z (U and L), and do a sign conversion on x and the new y(V and U) */
                    shader_addline(arg->buffer, "tmp0.g = %s.%c;\n",
                                   dst_param.reg_name, dst_param.mask_str[2]);
                    shader_addline(arg->buffer, "%s.%c%c = %s.%c%c * 2.0 - 1.0;\n",
                                   dst_param.reg_name, dst_param.mask_str[2], dst_param.mask_str[1],
                                   dst_param2.reg_name, dst_param.mask_str[1], dst_param.mask_str[3]);
                    shader_addline(arg->buffer, "%s.%c = tmp0.g;\n", dst_param.reg_name,
                                   dst_param.mask_str[3]);
                } else if(mask_size == 2) {
                    /* This is bad: We have VL, but we need VU */
                    FIXME("2 components sampled from a converted L6V5U5 texture\n");
                } else {
                    shader_addline(arg->buffer, "%s.%c = %s.%c * 2.0 - 1.0;\n",
                                   dst_param.reg_name, dst_param.mask_str[1],
                                   dst_param2.reg_name, dst_param.mask_str[1]);
                }
            }
            break;

        case WINED3DFMT_Q8W8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Correct the sign in all channels. The writemask just applies as-is, no
                 * need for checking the mask size
                 */
                shader_glsl_add_dst_param(arg, arg->dst,
                                          WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 |
                                          WINED3DSP_WRITEMASK_2 | WINED3DSP_WRITEMASK_3,
                                          &dst_param);
                shader_addline(arg->buffer, "%s%s = %s%s * 2.0 - 1.0;\n", dst_param.reg_name, dst_param.mask_str,
                               dst_param.reg_name, dst_param.mask_str);
            }
            break;

        case WINED3DFMT_ATI2N:
            /* GL_ATI_texture_compression_3dc returns the two channels as luminance-alpha,
             * which means the first one is replicated across .rgb, and the 2nd one is in
             * .a. We need the 2nd in .g
             *
             * GL_EXT_texture_compression_rgtc returns the values in .rg, however, they
             * are swapped compared to d3d. So swap red and green.
             */
            mask = shader_glsl_add_dst_param(arg, arg->dst, WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &dst_param);
            mask_size = shader_glsl_get_write_mask_size(mask);
            if(GL_SUPPORT(EXT_TEXTURE_COMPRESSION_RGTC)) {
                if(mask_size >= 2) {
                    shader_addline(arg->buffer, "%s.%c%c = %s.%c%c;\n",
                                dst_param.reg_name, dst_param.mask_str[1],
                                                    dst_param.mask_str[2],
                                dst_param.reg_name, dst_param.mask_str[2],
                                                    dst_param.mask_str[1]);
                } else {
                    FIXME("%u components sampled from a converted ATI2N texture\n", mask_size);
                }
            } else {
                if(mask_size == 4) {
                    /* Swap y and z (U and L), and do a sign conversion on x and the new y(V and U) */
                    shader_addline(arg->buffer, "%s.%c = %s.%c;\n",
                                dst_param.reg_name, dst_param.mask_str[2],
                                dst_param.reg_name, dst_param.mask_str[4]);
                } else if(mask_size == 1) {
                    /* Nothing to do */
                } else {
                    FIXME("%u components sampled from a converted ATI2N texture\n", mask_size);
                    /* This is bad: We have .r[gb], but we need .ra */
                }
            }
            break;

            /* stupid compiler */
        default:
            break;
    }
}

/*****************************************************************************
 * 
 * Begin processing individual instruction opcodes
 * 
 ****************************************************************************/

/* Generate GLSL arithmetic functions (dst = src1 + src2) */
static void shader_glsl_arith(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_mov(SHADER_OPCODE_ARG* arg) {
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
            shader_get_regtype(arg->dst) == WINED3DSPR_ADDR)) {
        /* This is a simple floor() */
        unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
        if (mask_size > 1) {
            shader_addline(buffer, "ivec%d(floor(%s)));\n", mask_size, src0_param.param_str);
        } else {
            shader_addline(buffer, "int(floor(%s)));\n", src0_param.param_str);
        }
    } else if(arg->opcode->opcode == WINED3DSIO_MOVA) {
        /* We need to *round* to the nearest int here. */
        unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
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
static void shader_glsl_dot(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD dst_write_mask, src_write_mask;
    unsigned int dst_size = 0;

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
static void shader_glsl_cross(SHADER_OPCODE_ARG *arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    char dst_mask[6];

    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_mask, &src1_param);
    shader_addline(arg->buffer, "cross(%s, %s)%s);\n", src0_param.param_str, src1_param.param_str, dst_mask);
}

/* Process the WINED3DSIO_POW instruction in GLSL (dst = |src0|^src1)
 * Src0 and src1 are scalars. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_pow(SHADER_OPCODE_ARG *arg) {
    SHADER_BUFFER *buffer = arg->buffer;
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

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

/* Process the WINED3DSIO_LOG instruction in GLSL (dst = log2(|src0|))
 * Src0 is a scalar. Note that D3D uses the absolute of src0, while
 * GLSL uses the value as-is. */
static void shader_glsl_log(SHADER_OPCODE_ARG *arg) {
    SHADER_BUFFER *buffer = arg->buffer;
    glsl_src_param_t src0_param;
    DWORD dst_write_mask;
    unsigned int dst_size;

    dst_write_mask = shader_glsl_append_dst(buffer, arg);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(log2(abs(%s))));\n", dst_size, src0_param.param_str);
    } else {
        shader_addline(buffer, "log2(abs(%s)));\n", src0_param.param_str);
    }
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
static void shader_glsl_map2gl(SHADER_OPCODE_ARG* arg) {
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
        case WINED3DSIO_EXP: instruction = "exp2"; break;
        case WINED3DSIO_SGN: instruction = "sign"; break;
        case WINED3DSIO_DSX: instruction = "dFdx"; break;
        case WINED3DSIO_DSY: instruction = "ycorrection.y * dFdy"; break;
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
static void shader_glsl_expp(SHADER_OPCODE_ARG* arg) {
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
        unsigned int mask_size;

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
static void shader_glsl_rcp(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &src_param);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(1.0 / %s));\n", mask_size, src_param.param_str);
    } else {
        shader_addline(arg->buffer, "1.0 / %s);\n", src_param.param_str);
    }
}

static void shader_glsl_rsq(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src_param;
    DWORD write_mask;
    unsigned int mask_size;

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
static void shader_glsl_compare(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    DWORD write_mask;
    unsigned int mask_size;

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
        switch(arg->opcode->opcode) {
            case WINED3DSIO_SLT:
                /* Step(src0, src1) is not suitable here because if src0 == src1 SLT is supposed,
                 * to return 0.0 but step returns 1.0 because step is not < x
                 * An alternative is a bvec compare padded with an unused second component.
                 * step(src1 * -1.0, src0 * -1.0) is not an option because it suffers from the same
                 * issue. Playing with not() is not possible either because not() does not accept
                 * a scalar.
                 */
                shader_addline(arg->buffer, "(%s < %s) ? 1.0 : 0.0);\n", src0_param.param_str, src1_param.param_str);
                break;
            case WINED3DSIO_SGE:
                /* Here we can use the step() function and safe a conditional */
                shader_addline(arg->buffer, "step(%s, %s));\n", src1_param.param_str, src0_param.param_str);
                break;
            default:
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }

    }
}

/** Process CMP instruction in GLSL (dst = src0 >= 0.0 ? src1 : src2), per channel */
static void shader_glsl_cmp(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask, cmp_channel = 0;
    unsigned int i, j;
    char mask_char[6];
    BOOL temp_destination = FALSE;

    if(shader_is_scalar(arg->src[0])) {
        write_mask = shader_glsl_append_dst(arg->buffer, arg);

        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_ALL, &src0_param);
        shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
        shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);

        shader_addline(arg->buffer, "%s >= 0.0 ? %s : %s);\n",
                       src0_param.param_str, src1_param.param_str, src2_param.param_str);
    } else {
        DWORD src0reg = arg->src[0] & WINED3DSP_REGNUM_MASK;
        DWORD src1reg = arg->src[1] & WINED3DSP_REGNUM_MASK;
        DWORD src2reg = arg->src[2] & WINED3DSP_REGNUM_MASK;
        DWORD src0regtype = shader_get_regtype(arg->src[0]);
        DWORD src1regtype = shader_get_regtype(arg->src[1]);
        DWORD src2regtype = shader_get_regtype(arg->src[2]);
        DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
        DWORD dstregtype = shader_get_regtype(arg->dst);

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

            /* Splitting the cmp instruction up in multiple lines imposes a problem:
            * The first lines may overwrite source parameters of the following lines.
            * Deal with that by using a temporary destination register if needed
            */
            if((src0reg == dstreg && src0regtype == dstregtype) ||
            (src1reg == dstreg && src1regtype == dstregtype) ||
            (src2reg == dstreg && src2regtype == dstregtype)) {

                write_mask = shader_glsl_get_write_mask(arg->dst & (~WINED3DSP_SWIZZLE_MASK | write_mask), mask_char);
                if (!write_mask) continue;
                shader_addline(arg->buffer, "tmp0%s = (", mask_char);
                temp_destination = TRUE;
            } else {
                write_mask = shader_glsl_append_dst_ext(arg->buffer, arg, arg->dst & (~WINED3DSP_SWIZZLE_MASK | write_mask));
                if (!write_mask) continue;
            }

            shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], cmp_channel, &src0_param);
            shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, &src1_param);
            shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, &src2_param);

            shader_addline(arg->buffer, "%s >= 0.0 ? %s : %s);\n",
                        src0_param.param_str, src1_param.param_str, src2_param.param_str);
        }

        if(temp_destination) {
            shader_glsl_get_write_mask(arg->dst, mask_char);
            shader_glsl_append_dst_ext(arg->buffer, arg, arg->dst);
            shader_addline(arg->buffer, "tmp0%s);\n", mask_char);
        }
    }

}

/** Process the CND opcode in GLSL (dst = (src0 > 0.5) ? src1 : src2) */
/* For ps 1.1-1.3, only a single component of src0 is used. For ps 1.4
 * the compare is done per component of src0. */
static void shader_glsl_cnd(SHADER_OPCODE_ARG* arg) {
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

        /* Fun: The D3DSI_COISSUE flag changes the semantic of the cnd instruction for < 1.4 shaders */
        if(arg->opcode_token & WINED3DSI_COISSUE) {
            shader_addline(arg->buffer, "%s /* COISSUE! */);\n", src1_param.param_str);
        } else {
            shader_addline(arg->buffer, "%s > 0.5 ? %s : %s);\n",
                    src0_param.param_str, src1_param.param_str, src2_param.param_str);
        }
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
static void shader_glsl_mad(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_mnxn(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_lrp(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_lit(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src3_param;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &src3_param);

    /* The sdk specifies the instruction like this
     * dst.x = 1.0;
     * if(src.x > 0.0) dst.y = src.x
     * else dst.y = 0.0.
     * if(src.x > 0.0 && src.y > 0.0) dst.z = pow(src.y, power);
     * else dst.z = 0.0;
     * dst.w = 1.0;
     *
     * Obviously that has quite a few conditionals in it which we don't like. So the first step is this:
     * dst.x = 1.0                                  ... No further explanation needed
     * dst.y = max(src.y, 0.0);                     ... If x < 0.0, use 0.0, otherwise x. Same as the conditional
     * dst.z = x > 0.0 ? pow(max(y, 0.0), p) : 0;   ... 0 ^ power is 0, and otherwise we use y anyway
     * dst.w = 1.0.                                 ... Nothing fancy.
     *
     * So we still have one conditional in there. So do this:
     * dst.z = pow(max(0.0, src.y) * step(0.0, src.x), power);
     *
     * step(0.0, x) will return 1 if src.x > 0.0, and 0 otherwise. So if y is 0 we get pow(0.0 * 1.0, power),
     * which sets dst.z to 0. If y > 0, but x = 0.0, we get pow(y * 0.0, power), which results in 0 too.
     * if both x and y are > 0, we get pow(y * 1.0, power), as it is supposed to
     */
    shader_addline(arg->buffer, "vec4(1.0, max(%s, 0.0), pow(max(0.0, %s) * step(0.0, %s), clamp(%s, -128.0, 128.0)), 1.0)%s);\n",
                   src0_param.param_str, src1_param.param_str, src0_param.param_str, src3_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
static void shader_glsl_dst(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_sincos(SHADER_OPCODE_ARG* arg) {
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
static void shader_glsl_loop(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src1_param;
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    DWORD regtype = shader_get_regtype(arg->src[1]);
    DWORD reg = arg->src[1] & WINED3DSP_REGNUM_MASK;
    const DWORD *control_values = NULL;
    local_constant *constant;

    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_ALL, &src1_param);

    /* Try to hardcode the loop control parameters if possible. Direct3D 9 class hardware doesn't support real
     * varying indexing, but Microsoft designed this feature for Shader model 2.x+. If the loop control is
     * known at compile time, the GLSL compiler can unroll the loop, and replace indirect addressing with direct
     * addressing.
     */
    if(regtype == WINED3DSPR_CONSTINT) {
        LIST_FOR_EACH_ENTRY(constant, &shader->baseShader.constantsI, local_constant, entry) {
            if(constant->idx == reg) {
                control_values = constant->value;
                break;
            }
        }
    }

    if(control_values) {
        if(control_values[2] > 0) {
            shader_addline(arg->buffer, "for (aL%u = %d; aL%u < (%d * %d + %d); aL%u += %d) {\n",
                           shader->baseShader.cur_loop_depth, control_values[1],
                           shader->baseShader.cur_loop_depth, control_values[0], control_values[2], control_values[1],
                           shader->baseShader.cur_loop_depth, control_values[2]);
        } else if(control_values[2] == 0) {
            shader_addline(arg->buffer, "for (aL%u = %d, tmpInt%u = 0; tmpInt%u < %d; tmpInt%u++) {\n",
                           shader->baseShader.cur_loop_depth, control_values[1], shader->baseShader.cur_loop_depth,
                           shader->baseShader.cur_loop_depth, control_values[0],
                           shader->baseShader.cur_loop_depth);
        } else {
            shader_addline(arg->buffer, "for (aL%u = %d; aL%u > (%d * %d + %d); aL%u += %d) {\n",
                           shader->baseShader.cur_loop_depth, control_values[1],
                           shader->baseShader.cur_loop_depth, control_values[0], control_values[2], control_values[1],
                           shader->baseShader.cur_loop_depth, control_values[2]);
        }
    } else {
        shader_addline(arg->buffer, "for (tmpInt%u = 0, aL%u = %s.y; tmpInt%u < %s.x; tmpInt%u++, aL%u += %s.z) {\n",
                       shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_regno,
                       src1_param.reg_name, shader->baseShader.cur_loop_depth, src1_param.reg_name,
                       shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_regno, src1_param.reg_name);
    }

    shader->baseShader.cur_loop_depth++;
    shader->baseShader.cur_loop_regno++;
}

static void shader_glsl_end(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;

    shader_addline(arg->buffer, "}\n");

    if(arg->opcode->opcode == WINED3DSIO_ENDLOOP) {
        shader->baseShader.cur_loop_depth--;
        shader->baseShader.cur_loop_regno--;
    }
    if(arg->opcode->opcode == WINED3DSIO_ENDREP) {
        shader->baseShader.cur_loop_depth--;
    }
}

static void shader_glsl_rep(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(arg->buffer, "for (tmpInt%d = 0; tmpInt%d < %s; tmpInt%d++) {\n",
                   shader->baseShader.cur_loop_depth, shader->baseShader.cur_loop_depth,
                   src0_param.param_str, shader->baseShader.cur_loop_depth);
    shader->baseShader.cur_loop_depth++;
}

static void shader_glsl_if(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_addline(arg->buffer, "if (%s) {\n", src0_param.param_str);
}

static void shader_glsl_ifc(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(arg->buffer, "if (%s %s %s) {\n",
            src0_param.param_str, shader_get_comp_op(arg->opcode_token), src1_param.param_str);
}

static void shader_glsl_else(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "} else {\n");
}

static void shader_glsl_break(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "break;\n");
}

/* FIXME: According to MSDN the compare is done per component. */
static void shader_glsl_breakc(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);

    shader_addline(arg->buffer, "if (%s %s %s) break;\n",
            src0_param.param_str, shader_get_comp_op(arg->opcode_token), src1_param.param_str);
}

static void shader_glsl_label(SHADER_OPCODE_ARG* arg) {

    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "}\n");
    shader_addline(arg->buffer, "void subroutine%u () {\n",  snum);
}

static void shader_glsl_call(SHADER_OPCODE_ARG* arg) {
    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "subroutine%u();\n", snum);
}

static void shader_glsl_callnz(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src1_param;

    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0, &src1_param);
    shader_addline(arg->buffer, "if (%s) subroutine%u();\n", src1_param.param_str, snum);
}

/*********************************************
 * Pixel Shader Specific Code begins here
 ********************************************/
static void pshader_glsl_tex(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD hex_version = This->baseShader.hex_version;
    char dst_swizzle[6];
    glsl_sample_function_t sample_function;
    DWORD sampler_type;
    DWORD sampler_idx;
    BOOL projected, texrect = FALSE;
    DWORD mask = 0;

    /* All versions have a destination register */
    shader_glsl_append_dst(arg->buffer, arg);

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (hex_version < WINED3DPS_VERSION(2,0)) {
        sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    } else {
        sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
    }
    sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;

    if (hex_version < WINED3DPS_VERSION(1,4)) {
        DWORD flags = deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS];

        /* Projected cube textures don't make a lot of sense, the resulting coordinates stay the same. */
        if (flags & WINED3DTTFF_PROJECTED && sampler_type != WINED3DSTT_CUBE) {
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
        if(arg->opcode_token & WINED3DSI_TEXLD_PROJECT) {
                /* ps 2.0 texldp instruction always divides by the fourth component. */
                projected = TRUE;
                mask = WINED3DSP_WRITEMASK_3;
        } else {
            projected = FALSE;
        }
    }

    if(deviceImpl->stateBlock->textures[sampler_idx] &&
       IWineD3DBaseTexture_GetTextureDimensions(deviceImpl->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
        texrect = TRUE;
    }

    shader_glsl_get_sample_function(sampler_type, projected, texrect, &sample_function);
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
        if(arg->opcode_token & WINED3DSI_TEXLD_BIAS) {
            glsl_src_param_t bias;
            shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, &bias);

            shader_addline(arg->buffer, "%s(Psampler%u, %s, %s)%s);\n",
                    sample_function.name, sampler_idx, coord_param.param_str,
                    bias.param_str, dst_swizzle);
        } else {
            shader_addline(arg->buffer, "%s(Psampler%u, %s)%s);\n",
                    sample_function.name, sampler_idx, coord_param.param_str, dst_swizzle);
        }
    }
}

static void shader_glsl_texldl(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*)arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    glsl_sample_function_t sample_function;
    glsl_src_param_t coord_param, lod_param;
    char dst_swizzle[6];
    DWORD sampler_type;
    DWORD sampler_idx;
    BOOL texrect = FALSE;

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_swizzle(arg->src[1], FALSE, arg->dst, dst_swizzle);

    sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
    sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    if(deviceImpl->stateBlock->textures[sampler_idx] &&
       IWineD3DBaseTexture_GetTextureDimensions(deviceImpl->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
        texrect = TRUE;
    }
    shader_glsl_get_sample_function(sampler_type, FALSE, texrect, &sample_function);    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], sample_function.coord_mask, &coord_param);

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

static void pshader_glsl_texcoord(SHADER_OPCODE_ARG* arg) {

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
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
            shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_2, &div_param);

            if (mask_size > 1) {
                shader_addline(buffer, "gl_TexCoord[%u]%s / vec%d(%s));\n", reg, dst_swizzle, mask_size, div_param.param_str);
            } else {
                shader_addline(buffer, "gl_TexCoord[%u]%s / %s);\n", reg, dst_swizzle, div_param.param_str);
            }
        } else if (src_mod == WINED3DSPSM_DW) {
            glsl_src_param_t div_param;
            unsigned int mask_size = shader_glsl_get_write_mask_size(write_mask);
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
static void pshader_glsl_texdp3tex(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    char dst_mask[6];
    glsl_sample_function_t sample_function;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    /* Do I have to take care about the projected bit? I don't think so, since the dp3 returns only one
     * scalar, and projected sampling would require 4.
     *
     * It is a dependent read - not valid with conditional NP2 textures
     */
    shader_glsl_get_sample_function(sampler_type, FALSE, FALSE, &sample_function);

    switch(count_bits(sample_function.coord_mask)) {
        case 1:
            shader_addline(arg->buffer, "%s(Psampler%u, dot(gl_TexCoord[%u].xyz, %s))%s);\n",
                           sample_function.name, sampler_idx, sampler_idx, src0_param.param_str, dst_mask);
            break;

        case 2:
            shader_addline(arg->buffer, "%s(Psampler%u, vec2(dot(gl_TexCoord[%u].xyz, %s), 0.0))%s);\n",
                          sample_function.name, sampler_idx, sampler_idx, src0_param.param_str, dst_mask);
            break;

        case 3:
            shader_addline(arg->buffer, "%s(Psampler%u, vec3(dot(gl_TexCoord[%u].xyz, %s), 0.0, 0.0))%s);\n",
                           sample_function.name, sampler_idx, sampler_idx, src0_param.param_str, dst_mask);
            break;
        default:
            FIXME("Unexpected mask bitcount %d\n", count_bits(sample_function.coord_mask));
    }
}

/** Process the WINED3DSIO_TEXDP3 instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
static void pshader_glsl_texdp3(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dst_mask;
    unsigned int mask_size;

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
static void pshader_glsl_texdepth(SHADER_OPCODE_ARG* arg) {
    glsl_dst_param_t dst_param;

    shader_glsl_add_dst_param(arg, arg->dst, 0, &dst_param);

    /* Tests show that texdepth never returns anything below 0.0, and that r5.y is clamped to 1.0.
     * Negative input is accepted, -0.25 / -0.5 returns 0.5. GL should clamp gl_FragDepth to [0;1], but
     * this doesn't always work, so clamp the results manually. Whether or not the x value is clamped at 1
     * too is irrelevant, since if x = 0, any y value < 1.0 (and > 1.0 is not allowed) results in a result
     * >= 1.0 or < 0.0
     */
    shader_addline(arg->buffer, "gl_FragDepth = clamp((%s.x / min(%s.y, 1.0)), 0.0, 1.0);\n", dst_param.reg_name, dst_param.reg_name);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in GLSL:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
static void pshader_glsl_texm3x2depth(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_addline(arg->buffer, "tmp0.y = dot(T%u.xyz, %s);\n", dstreg, src0_param.param_str);
    shader_addline(arg->buffer, "gl_FragDepth = (tmp0.y == 0.0) ? 1.0 : clamp(tmp0.x / tmp0.y, 0.0, 1.0);\n");
}

/** Process the WINED3DSIO_TEXM3X2PAD instruction in GLSL
 * Calculate the 1st of a 2-row matrix multiplication. */
static void pshader_glsl_texm3x2pad(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_src_param_t src0_param;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_param.param_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
static void pshader_glsl_texm3x3pad(SHADER_OPCODE_ARG* arg) {

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

static void pshader_glsl_texm3x2tex(SHADER_OPCODE_ARG* arg) {
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
static void pshader_glsl_texm3x3tex(SHADER_OPCODE_ARG* arg) {
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
    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(sampler_type, FALSE, FALSE, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_addline(arg->buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3 instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply */
static void pshader_glsl_texm3x3(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, &src0_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_addline(arg->buffer, "vec4(tmp0.xy, dot(T%u.xyz, %s), 1.0)%s);\n", reg, src0_param.param_str, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3SPEC instruction in GLSL 
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void pshader_glsl_texm3x3spec(SHADER_OPCODE_ARG* arg) {

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
    /* Reflection calculation */
    shader_addline(buffer, "tmp0.xyz = -reflect((%s), normalize(tmp0.xyz));\n", src1_param.param_str);

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(stype, FALSE, FALSE, &sample_function);

    /* Sample the texture */
    shader_addline(buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL 
 * Perform the final texture lookup based on the previous 2 3x3 matrix multiplies */
static void pshader_glsl_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

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
    shader_addline(buffer, "tmp0.xyz = -reflect(tmp1.xyz, normalize(tmp0.xyz));\n");

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(sampler_type, FALSE, FALSE, &sample_function);

    /* Sample the texture using the calculated coordinates */
    shader_addline(buffer, "%s(Psampler%u, tmp0.xyz)%s);\n", sample_function.name, reg, dst_mask);

    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * texbem is pshader <= 1.3 only, this saves a few version checks
 */
static void pshader_glsl_texbem(SHADER_OPCODE_ARG* arg) {
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
    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(sampler_type, FALSE, FALSE, &sample_function);
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
    if(arg->opcode->opcode == WINED3DSIO_TEXBEML) {
        glsl_src_param_t luminance_param;
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_2, &luminance_param);
        shader_addline(arg->buffer, "(%s(Psampler%u, T%u%s + vec4(bumpenvmat%d * %s, 0.0, 0.0)%s )*(%s * luminancescale%d + luminanceoffset%d))%s);\n",
                       sample_function.name, sampler_idx, sampler_idx, coord_mask, sampler_idx, coord_param.param_str, coord_mask,
                       luminance_param.param_str, sampler_idx, sampler_idx, dst_swizzle);
    } else {
        shader_addline(arg->buffer, "%s(Psampler%u, T%u%s + vec4(bumpenvmat%d * %s, 0.0, 0.0)%s )%s);\n",
                       sample_function.name, sampler_idx, sampler_idx, coord_mask, sampler_idx, coord_param.param_str, coord_mask, dst_swizzle);
    }
}

static void pshader_glsl_bem(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param, src1_param;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0|WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0|WINED3DSP_WRITEMASK_1, &src1_param);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_addline(arg->buffer, "%s + bumpenvmat%d * %s);\n",
                   src0_param.param_str, sampler_idx, src1_param.param_str);
}

/** Process the WINED3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
static void pshader_glsl_texreg2ar(SHADER_OPCODE_ARG* arg) {

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
static void pshader_glsl_texreg2gb(SHADER_OPCODE_ARG* arg) {
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
static void pshader_glsl_texreg2rgb(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    char dst_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    glsl_sample_function_t sample_function;

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    /* Dependent read, not valid with conditional NP2 */
    shader_glsl_get_sample_function(sampler_type, FALSE, FALSE, &sample_function);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], sample_function.coord_mask, &src0_param);

    shader_addline(arg->buffer, "%s(Psampler%u, %s)%s);\n", sample_function.name, sampler_idx, src0_param.param_str, dst_mask);
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
static void pshader_glsl_texkill(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD hex_version = This->baseShader.hex_version;
    glsl_dst_param_t dst_param;

    /* The argument is a destination parameter, and no writemasks are allowed */
    shader_glsl_add_dst_param(arg, arg->dst, 0, &dst_param);
    if((hex_version >= WINED3DPS_VERSION(2,0))) {
        /* 2.0 shaders compare all 4 components in texkill */
        shader_addline(arg->buffer, "if (any(lessThan(%s.xyzw, vec4(0.0)))) discard;\n", dst_param.reg_name);
    } else {
        /* 1.X shaders only compare the first 3 components, probably due to the nature of the texkill
         * instruction as a tex* instruction, and phase, which kills all a / w components. Even if all
         * 4 components are defined, only the first 3 are used
         */
        shader_addline(arg->buffer, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;\n", dst_param.reg_name);
    }
}

/** Process the WINED3DSIO_DP2ADD instruction in GLSL.
 * dst = dot2(src0, src1) + src2 */
static void pshader_glsl_dp2add(SHADER_OPCODE_ARG* arg) {
    glsl_src_param_t src0_param;
    glsl_src_param_t src1_param;
    glsl_src_param_t src2_param;
    DWORD write_mask;
    unsigned int mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src0_param);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, &src1_param);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], WINED3DSP_WRITEMASK_0, &src2_param);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(dot(%s, %s) + %s));\n", mask_size, src0_param.param_str, src1_param.param_str, src2_param.param_str);
    } else {
        shader_addline(arg->buffer, "dot(%s, %s) + %s);\n", src0_param.param_str, src1_param.param_str, src2_param.param_str);
    }
}

static void pshader_glsl_input_pack(
   SHADER_BUFFER* buffer,
   semantic* semantics_in,
   IWineD3DPixelShader *iface) {

   unsigned int i;
   IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *) iface;

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

           case WINED3DDECLUSAGE_TEXCOORD:
               if(usage_idx < 8 && This->vertexprocessing == pretransformed) {
                   shader_addline(buffer, "IN[%u]%s = gl_TexCoord[%u]%s;\n",
                                  This->input_reg_map[i], reg_mask, usage_idx, reg_mask);
               } else {
                   shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                                  This->input_reg_map[i], reg_mask, reg_mask);
               }
               break;

           case WINED3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "IN[%u]%s = vec4(gl_Color)%s;\n",
                       This->input_reg_map[i], reg_mask, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "IN[%u]%s = vec4(gl_SecondaryColor)%s;\n",
                       This->input_reg_map[i], reg_mask, reg_mask);
               else
                   shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                       This->input_reg_map[i], reg_mask, reg_mask);
               break;

           default:
               shader_addline(buffer, "IN[%u]%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                   This->input_reg_map[i], reg_mask, reg_mask);
        }
    }
}

/*********************************************
 * Vertex Shader Specific Code begins here
 ********************************************/

static void add_glsl_program_entry(struct shader_glsl_priv *priv, struct glsl_shader_prog_link *entry) {
    glsl_program_key_t *key;

    key = HeapAlloc(GetProcessHeap(), 0, sizeof(glsl_program_key_t));
    key->vshader = entry->vshader;
    key->pshader = entry->pshader;

    hash_table_put(priv->glsl_program_lookup, key, entry);
}

static struct glsl_shader_prog_link *get_glsl_program_entry(struct shader_glsl_priv *priv,
        GLhandleARB vshader, GLhandleARB pshader) {
    glsl_program_key_t key;

    key.vshader = vshader;
    key.pshader = pshader;

    return (struct glsl_shader_prog_link *)hash_table_get(priv->glsl_program_lookup, &key);
}

static void delete_glsl_program_entry(struct shader_glsl_priv *priv, WineD3D_GL_Info *gl_info, struct glsl_shader_prog_link *entry) {
    glsl_program_key_t *key;

    key = HeapAlloc(GetProcessHeap(), 0, sizeof(glsl_program_key_t));
    key->vshader = entry->vshader;
    key->pshader = entry->pshader;
    hash_table_remove(priv->glsl_program_lookup, key);

    GL_EXTCALL(glDeleteObjectARB(entry->programId));
    if (entry->vshader) list_remove(&entry->vshader_entry);
    if (entry->pshader) list_remove(&entry->pshader_entry);
    HeapFree(GetProcessHeap(), 0, entry->vuniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry->puniformF_locations);
    HeapFree(GetProcessHeap(), 0, entry);
}

static void handle_ps3_input(SHADER_BUFFER *buffer, semantic *semantics_in, semantic *semantics_out, WineD3D_GL_Info *gl_info, DWORD *map) {
    unsigned int i, j;
    DWORD usage_token, usage_token_out;
    DWORD register_token, register_token_out;
    DWORD usage, usage_idx, usage_out, usage_idx_out;
    DWORD *set;
    DWORD in_idx;
    DWORD in_count = GL_LIMITS(glsl_varyings) / 4;
    char reg_mask[6], reg_mask_out[6];
    char destination[50];

    set = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*set) * (in_count + 2));

    if (!semantics_out) {
        /* Save gl_FrontColor & gl_FrontSecondaryColor before overwriting them. */
        shader_addline(buffer, "vec4 front_color = gl_FrontColor;\n");
        shader_addline(buffer, "vec4 front_secondary_color = gl_FrontSecondaryColor;\n");
    }

    for(i = 0; i < MAX_REG_INPUT; i++) {
        usage_token = semantics_in[i].usage;
        if (!usage_token) continue;

        in_idx = map[i];
        if (in_idx >= (in_count + 2)) {
            FIXME("More input varyings declared than supported, expect issues\n");
            continue;
        } else if(map[i] == -1) {
            /* Declared, but not read register */
            continue;
        }

        if (in_idx == in_count) {
            sprintf(destination, "gl_FrontColor");
        } else if (in_idx == in_count + 1) {
            sprintf(destination, "gl_FrontSecondaryColor");
        } else {
            sprintf(destination, "IN[%u]", in_idx);
        }

        register_token = semantics_in[i].reg;

        usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
        usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
        set[map[i]] = shader_glsl_get_write_mask(register_token, reg_mask);

        if(!semantics_out) {
            switch(usage) {
                case WINED3DDECLUSAGE_COLOR:
                    if (usage_idx == 0)
                        shader_addline(buffer, "%s%s = front_color%s;\n",
                                       destination, reg_mask, reg_mask);
                    else if (usage_idx == 1)
                        shader_addline(buffer, "%s%s = front_secondary_color%s;\n",
                                       destination, reg_mask, reg_mask);
                    else
                        shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                                       destination, reg_mask, reg_mask);
                    break;

                case WINED3DDECLUSAGE_TEXCOORD:
                    if (usage_idx < 8) {
                        shader_addline(buffer, "%s%s = gl_TexCoord[%u]%s;\n",
                                       destination, reg_mask, usage_idx, reg_mask);
                    } else {
                        shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                                       destination, reg_mask, reg_mask);
                    }
                    break;

                case WINED3DDECLUSAGE_FOG:
                    shader_addline(buffer, "%s%s = vec4(gl_FogFragCoord, 0.0, 0.0, 0.0)%s;\n",
                                   destination, reg_mask, reg_mask);
                    break;

                default:
                    shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                                   destination, reg_mask, reg_mask);
            }
        } else {
            BOOL found = FALSE;
            for(j = 0; j < MAX_REG_OUTPUT; j++) {
                usage_token_out = semantics_out[j].usage;
                if (!usage_token_out) continue;
                register_token_out = semantics_out[j].reg;

                usage_out = (usage_token_out & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
                usage_idx_out = (usage_token_out & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
                shader_glsl_get_write_mask(register_token_out, reg_mask_out);

                if(usage == usage_out &&
                   usage_idx == usage_idx_out) {
                    shader_addline(buffer, "%s%s = OUT[%u]%s;\n",
                                   destination, reg_mask, j, reg_mask);
                    found = TRUE;
                }
            }
            if(!found) {
                shader_addline(buffer, "%s%s = vec4(0.0, 0.0, 0.0, 0.0)%s;\n",
                               destination, reg_mask, reg_mask);
            }
        }
    }

    /* This is solely to make the compiler / linker happy and avoid warning about undefined
     * varyings. It shouldn't result in any real code executed on the GPU, since all read
     * input varyings are assigned above, if the optimizer works properly.
     */
    for(i = 0; i < in_count + 2; i++) {
        if(set[i] != WINED3DSP_WRITEMASK_ALL) {
            unsigned int size = 0;
            memset(reg_mask, 0, sizeof(reg_mask));
            if(!(set[i] & WINED3DSP_WRITEMASK_0)) {
                reg_mask[size] = 'x';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_1)) {
                reg_mask[size] = 'y';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_2)) {
                reg_mask[size] = 'z';
                size++;
            }
            if(!(set[i] & WINED3DSP_WRITEMASK_3)) {
                reg_mask[size] = 'w';
                size++;
            }

            if (i == in_count) {
                sprintf(destination, "gl_FrontColor");
            } else if (i == in_count + 1) {
                sprintf(destination, "gl_FrontSecondaryColor");
            } else {
                sprintf(destination, "IN[%u]", i);
            }

            if (size == 1) {
                shader_addline(buffer, "%s.%s = 0.0;\n", destination, reg_mask);
            } else {
                shader_addline(buffer, "%s.%s = vec%u(0.0);\n", destination, reg_mask, size);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, set);
}

static GLhandleARB generate_param_reorder_function(IWineD3DVertexShader *vertexshader,
        IWineD3DPixelShader *pixelshader,
        WineD3D_GL_Info *gl_info) {
    GLhandleARB ret = 0;
    IWineD3DVertexShaderImpl *vs = (IWineD3DVertexShaderImpl *) vertexshader;
    IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) pixelshader;
    IWineD3DDeviceImpl *device;
    DWORD vs_major = WINED3DSHADER_VERSION_MAJOR(vs->baseShader.hex_version);
    DWORD ps_major = ps ? WINED3DSHADER_VERSION_MAJOR(ps->baseShader.hex_version) : 0;
    unsigned int i;
    SHADER_BUFFER buffer;
    DWORD usage_token;
    DWORD register_token;
    DWORD usage, usage_idx, writemask;
    char reg_mask[6];
    semantic *semantics_out, *semantics_in;

    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE);
    buffer.bsize = 0;
    buffer.lineNo = 0;
    buffer.newline = TRUE;

    shader_addline(&buffer, "#version 120\n");

    if(vs_major < 3 && ps_major < 3) {
        /* That one is easy: The vertex shader writes to the builtin varyings, the pixel shader reads from them.
         * Take care about the texcoord .w fixup though if we're using the fixed function fragment pipeline
         */
        device = (IWineD3DDeviceImpl *) vs->baseShader.device;
        if((GLINFO_LOCATION).set_texcoord_w && ps_major == 0 && vs_major > 0 &&
            !device->frag_pipe->ffp_proj_control) {
            shader_addline(&buffer, "void order_ps_input() {\n");
            for(i = 0; i < min(8, MAX_REG_TEXCRD); i++) {
                if(vs->baseShader.reg_maps.texcoord_mask[i] != 0 &&
                   vs->baseShader.reg_maps.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL) {
                    shader_addline(&buffer, "gl_TexCoord[%u].w = 1.0;\n", i);
                }
            }
            shader_addline(&buffer, "}\n");
        } else {
            shader_addline(&buffer, "void order_ps_input() { /* do nothing */ }\n");
        }
    } else if(ps_major < 3 && vs_major >= 3) {
        /* The vertex shader writes to its own varyings, the pixel shader needs them in the builtin ones */
        semantics_out = vs->semantics_out;

        shader_addline(&buffer, "void order_ps_input(in vec4 OUT[%u]) {\n", MAX_REG_OUTPUT);
        for(i = 0; i < MAX_REG_OUTPUT; i++) {
            usage_token = semantics_out[i].usage;
            if (!usage_token) continue;
            register_token = semantics_out[i].reg;

            usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
            usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
            writemask = shader_glsl_get_write_mask(register_token, reg_mask);

            switch(usage) {
                case WINED3DDECLUSAGE_COLOR:
                    if (usage_idx == 0)
                        shader_addline(&buffer, "gl_FrontColor%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
                    else if (usage_idx == 1)
                        shader_addline(&buffer, "gl_FrontSecondaryColor%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
                    break;

                case WINED3DDECLUSAGE_POSITION:
                    shader_addline(&buffer, "gl_Position%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
                    break;

                case WINED3DDECLUSAGE_TEXCOORD:
                    if (usage_idx < 8) {
                        if(!(GLINFO_LOCATION).set_texcoord_w || ps_major > 0) writemask |= WINED3DSP_WRITEMASK_3;

                        shader_addline(&buffer, "gl_TexCoord[%u]%s = OUT[%u]%s;\n",
                                        usage_idx, reg_mask, i, reg_mask);
                        if(!(writemask & WINED3DSP_WRITEMASK_3)) {
                            shader_addline(&buffer, "gl_TexCoord[%u].w = 1.0;\n", usage_idx);
                        }
                    }
                    break;

                case WINED3DDECLUSAGE_PSIZE:
                    shader_addline(&buffer, "gl_PointSize = OUT[%u].x;\n", i);
                    break;

                case WINED3DDECLUSAGE_FOG:
                    shader_addline(&buffer, "gl_FogFragCoord = OUT[%u].%c;\n", i, reg_mask[1]);
                    break;

                default:
                    break;
            }
        }
        shader_addline(&buffer, "}\n");

    } else if(ps_major >= 3 && vs_major >= 3) {
        semantics_out = vs->semantics_out;
        semantics_in = ps->semantics_in;

        /* This one is tricky: a 3.0 pixel shader reads from a 3.0 vertex shader */
        shader_addline(&buffer, "varying vec4 IN[%u];\n", GL_LIMITS(glsl_varyings) / 4);
        shader_addline(&buffer, "void order_ps_input(in vec4 OUT[%u]) {\n", MAX_REG_OUTPUT);

        /* First, sort out position and point size. Those are not passed to the pixel shader */
        for(i = 0; i < MAX_REG_OUTPUT; i++) {
            usage_token = semantics_out[i].usage;
            if (!usage_token) continue;
            register_token = semantics_out[i].reg;

            usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
            usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
            shader_glsl_get_write_mask(register_token, reg_mask);

            switch(usage) {
                case WINED3DDECLUSAGE_POSITION:
                    shader_addline(&buffer, "gl_Position%s = OUT[%u]%s;\n", reg_mask, i, reg_mask);
                    break;

                case WINED3DDECLUSAGE_PSIZE:
                    shader_addline(&buffer, "gl_PointSize = OUT[%u].x;\n", i);
                    break;

                default:
                    break;
            }
        }

        /* Then, fix the pixel shader input */
        handle_ps3_input(&buffer, semantics_in, semantics_out, gl_info, ps->input_reg_map);

        shader_addline(&buffer, "}\n");
    } else if(ps_major >= 3 && vs_major < 3) {
        semantics_in = ps->semantics_in;

        shader_addline(&buffer, "varying vec4 IN[%u];\n", GL_LIMITS(glsl_varyings) / 4);
        shader_addline(&buffer, "void order_ps_input() {\n");
        /* The vertex shader wrote to the builtin varyings. There is no need to figure out position and
         * point size, but we depend on the optimizers kindness to find out that the pixel shader doesn't
         * read gl_TexCoord and gl_ColorX, otherwise we'll run out of varyings
         */
        handle_ps3_input(&buffer, semantics_in, NULL, gl_info, ps->input_reg_map);
        shader_addline(&buffer, "}\n");
    } else {
        ERR("Unexpected vertex and pixel shader version condition: vs: %d, ps: %d\n", vs_major, ps_major);
    }

    ret = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    checkGLcall("glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB)");
    GL_EXTCALL(glShaderSourceARB(ret, 1, (const char**)&buffer.buffer, NULL));
    checkGLcall("glShaderSourceARB(ret, 1, (const char**)&buffer.buffer, NULL)");
    GL_EXTCALL(glCompileShaderARB(ret));
    checkGLcall("glCompileShaderARB(ret)");

    HeapFree(GetProcessHeap(), 0, buffer.buffer);
    return ret;
}

static void hardcode_local_constants(IWineD3DBaseShaderImpl *shader, WineD3D_GL_Info *gl_info, GLhandleARB programId, char prefix) {
    local_constant* lconst;
    GLuint tmp_loc;
    float *value;
    char glsl_name[8];

    LIST_FOR_EACH_ENTRY(lconst, &shader->baseShader.constantsF, local_constant, entry) {
        value = (float *) lconst->value;
        snprintf(glsl_name, sizeof(glsl_name), "%cLC%u", prefix, lconst->idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
        GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, value));
    }
    checkGLcall("Hardcoding local constants\n");
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
    struct shader_glsl_priv *priv          = (struct shader_glsl_priv *)This->shader_priv;
    WineD3D_GL_Info *gl_info               = &This->adapter->gl_info;
    IWineD3DPixelShader  *pshader          = This->stateBlock->pixelShader;
    IWineD3DVertexShader *vshader          = This->stateBlock->vertexShader;
    struct glsl_shader_prog_link *entry    = NULL;
    GLhandleARB programId                  = 0;
    GLhandleARB reorder_shader_id          = 0;
    int i;
    char glsl_name[8];

    GLhandleARB vshader_id = use_vs ? ((IWineD3DBaseShaderImpl*)vshader)->baseShader.prgId : 0;
    GLhandleARB pshader_id = use_ps ? ((IWineD3DBaseShaderImpl*)pshader)->baseShader.prgId : 0;
    entry = get_glsl_program_entry(priv, vshader_id, pshader_id);
    if (entry) {
        priv->glsl_program = entry;
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
    add_glsl_program_entry(priv, entry);

    /* Set the current program */
    priv->glsl_program = entry;

    /* Attach GLSL vshader */
    if (vshader_id) {
        int max_attribs = 16;   /* TODO: Will this always be the case? It is at the moment... */
        char tmp_name[10];

        reorder_shader_id = generate_param_reorder_function(vshader, pshader, gl_info);
        TRACE("Attaching GLSL shader object %u to program %u\n", reorder_shader_id, programId);
        GL_EXTCALL(glAttachObjectARB(programId, reorder_shader_id));
        checkGLcall("glAttachObjectARB");
        /* Flag the reorder function for deletion, then it will be freed automatically when the program
         * is destroyed
         */
        GL_EXTCALL(glDeleteObjectARB(reorder_shader_id));

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
            if (((IWineD3DBaseShaderImpl*)vshader)->baseShader.reg_maps.attributes[i]) {
                snprintf(tmp_name, sizeof(tmp_name), "attrib%i", i);
                GL_EXTCALL(glBindAttribLocationARB(programId, i, tmp_name));
            }
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
        snprintf(glsl_name, sizeof(glsl_name), "VC[%i]", i);
        entry->vuniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    for (i = 0; i < MAX_CONST_I; ++i) {
        snprintf(glsl_name, sizeof(glsl_name), "VI[%i]", i);
        entry->vuniformI_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    entry->puniformF_locations = HeapAlloc(GetProcessHeap(), 0, sizeof(GLhandleARB) * GL_LIMITS(pshader_constantsF));
    for (i = 0; i < GL_LIMITS(pshader_constantsF); ++i) {
        snprintf(glsl_name, sizeof(glsl_name), "PC[%i]", i);
        entry->puniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    for (i = 0; i < MAX_CONST_I; ++i) {
        snprintf(glsl_name, sizeof(glsl_name), "PI[%i]", i);
        entry->puniformI_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }

    if(pshader) {
        for(i = 0; i < ((IWineD3DPixelShaderImpl*)pshader)->numbumpenvmatconsts; i++) {
            char name[32];
            sprintf(name, "bumpenvmat%d", ((IWineD3DPixelShaderImpl*)pshader)->bumpenvmatconst[i].texunit);
            entry->bumpenvmat_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
            sprintf(name, "luminancescale%d", ((IWineD3DPixelShaderImpl*)pshader)->luminanceconst[i].texunit);
            entry->luminancescale_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
            sprintf(name, "luminanceoffset%d", ((IWineD3DPixelShaderImpl*)pshader)->luminanceconst[i].texunit);
            entry->luminanceoffset_location[i] = GL_EXTCALL(glGetUniformLocationARB(programId, name));
        }
    }


    entry->posFixup_location = GL_EXTCALL(glGetUniformLocationARB(programId, "posFixup"));
    entry->srgb_comparison_location = GL_EXTCALL(glGetUniformLocationARB(programId, "srgb_comparison"));
    entry->srgb_mul_low_location = GL_EXTCALL(glGetUniformLocationARB(programId, "srgb_mul_low"));
    entry->ycorrection_location = GL_EXTCALL(glGetUniformLocationARB(programId, "ycorrection"));
    checkGLcall("Find glsl program uniform locations");

    if (pshader && WINED3DSHADER_VERSION_MAJOR(((IWineD3DPixelShaderImpl *)pshader)->baseShader.hex_version) >= 3
            && ((IWineD3DPixelShaderImpl *)pshader)->declared_in_count > GL_LIMITS(glsl_varyings) / 4) {
        TRACE("Shader %d needs vertex color clamping disabled\n", programId);
        entry->vertex_color_clamp = GL_FALSE;
    } else {
        entry->vertex_color_clamp = GL_FIXED_ONLY_ARB;
    }

    /* Set the shader to allow uniform loading on it */
    GL_EXTCALL(glUseProgramObjectARB(programId));
    checkGLcall("glUseProgramObjectARB(programId)");

    /* Load the vertex and pixel samplers now. The function that finds the mappings makes sure
     * that it stays the same for each vertexshader-pixelshader pair(=linked glsl program). If
     * a pshader with fixed function pipeline is used there are no vertex samplers, and if a
     * vertex shader with fixed function pixel processing is used we make sure that the card
     * supports enough samplers to allow the max number of vertex samplers with all possible
     * fixed function fragment processing setups. So once the program is linked these samplers
     * won't change.
     */
    if(vshader_id) {
        /* Load vertex shader samplers */
        shader_glsl_load_vsamplers(gl_info, (IWineD3DStateBlock*)This->stateBlock, programId);
    }
    if(pshader_id) {
        /* Load pixel shader samplers */
        shader_glsl_load_psamplers(gl_info, (IWineD3DStateBlock*)This->stateBlock, programId);
    }

    /* If the local constants do not have to be loaded with the environment constants,
     * load them now to have them hardcoded in the GLSL program. This saves some CPU cycles
     * later
     */
    if(pshader && !((IWineD3DPixelShaderImpl*)pshader)->baseShader.load_local_constsF) {
        hardcode_local_constants((IWineD3DBaseShaderImpl *) pshader, gl_info, programId, 'P');
    }
    if(vshader && !((IWineD3DVertexShaderImpl*)vshader)->baseShader.load_local_constsF) {
        hardcode_local_constants((IWineD3DBaseShaderImpl *) vshader, gl_info, programId, 'V');
    }
}

static GLhandleARB create_glsl_blt_shader(WineD3D_GL_Info *gl_info, enum tex_types tex_type) {
    GLhandleARB program_id;
    GLhandleARB vshader_id, pshader_id;
    const char *blt_vshader[] = {
        "#version 120\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "}\n"
    };

    const char *blt_pshaders[tex_type_count] = {
        /* tex_1d */
        NULL,
        /* tex_2d */
        "#version 120\n"
        "uniform sampler2D sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
        /* tex_3d */
        NULL,
        /* tex_cube */
        "#version 120\n"
        "uniform samplerCube sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = textureCube(sampler, gl_TexCoord[0].xyz).x;\n"
        "}\n",
        /* tex_rect */
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2DRect(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n",
    };

    if (!blt_pshaders[tex_type])
    {
        FIXME("tex_type %#x not supported\n", tex_type);
        tex_type = tex_2d;
    }

    vshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(vshader_id, 1, blt_vshader, NULL));
    GL_EXTCALL(glCompileShaderARB(vshader_id));

    pshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(pshader_id, 1, &blt_pshaders[tex_type], NULL));
    GL_EXTCALL(glCompileShaderARB(pshader_id));

    program_id = GL_EXTCALL(glCreateProgramObjectARB());
    GL_EXTCALL(glAttachObjectARB(program_id, vshader_id));
    GL_EXTCALL(glAttachObjectARB(program_id, pshader_id));
    GL_EXTCALL(glLinkProgramARB(program_id));

    print_glsl_info_log(&GLINFO_LOCATION, program_id);

    /* Once linked we can mark the shaders for deletion. They will be deleted once the program
     * is destroyed
     */
    GL_EXTCALL(glDeleteObjectARB(vshader_id));
    GL_EXTCALL(glDeleteObjectARB(pshader_id));
    return program_id;
}

static void shader_glsl_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *)This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    GLhandleARB program_id = 0;
    GLenum old_vertex_color_clamp, current_vertex_color_clamp;

    old_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vertex_color_clamp : GL_FIXED_ONLY_ARB;

    if (useVS || usePS) set_glsl_shader_program(iface, usePS, useVS);
    else priv->glsl_program = NULL;

    current_vertex_color_clamp = priv->glsl_program ? priv->glsl_program->vertex_color_clamp : GL_FIXED_ONLY_ARB;

    if (old_vertex_color_clamp != current_vertex_color_clamp) {
        if (GL_SUPPORT(ARB_COLOR_BUFFER_FLOAT)) {
            GL_EXTCALL(glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, current_vertex_color_clamp));
            checkGLcall("glClampColorARB");
        } else {
            FIXME("vertex color clamp needs to be changed, but extension not supported.\n");
        }
    }

    program_id = priv->glsl_program ? priv->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);
    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
}

static void shader_glsl_select_depth_blt(IWineD3DDevice *iface, enum tex_types tex_type) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *) This->shader_priv;
    GLhandleARB *blt_program = &priv->depth_blt_program[tex_type];

    if (!*blt_program) {
        GLhandleARB loc;
        *blt_program = create_glsl_blt_shader(gl_info, tex_type);
        loc = GL_EXTCALL(glGetUniformLocationARB(*blt_program, "sampler"));
        GL_EXTCALL(glUseProgramObjectARB(*blt_program));
        GL_EXTCALL(glUniform1iARB(loc, 0));
    } else {
        GL_EXTCALL(glUseProgramObjectARB(*blt_program));
    }
}

static void shader_glsl_deselect_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *) This->shader_priv;
    GLhandleARB program_id;

    program_id = priv->glsl_program ? priv->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);

    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
}

static void shader_glsl_cleanup(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    GL_EXTCALL(glUseProgramObjectARB(0));
}

static void shader_glsl_destroy(IWineD3DBaseShader *iface) {
    struct list *linked_programs;
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)This->baseShader.device;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *)device->shader_priv;
    WineD3D_GL_Info *gl_info = &device->adapter->gl_info;

    /* Note: Do not use QueryInterface here to find out which shader type this is because this code
     * can be called from IWineD3DBaseShader::Release
     */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);

    if(This->baseShader.prgId == 0) return;
    linked_programs = &This->baseShader.linked_programs;

    TRACE("Deleting linked programs\n");
    if (linked_programs->next) {
        struct glsl_shader_prog_link *entry, *entry2;

        if(pshader) {
            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, pshader_entry) {
                delete_glsl_program_entry(priv, gl_info, entry);
            }
        } else {
            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, linked_programs, struct glsl_shader_prog_link, vshader_entry) {
                delete_glsl_program_entry(priv, gl_info, entry);
            }
        }
    }

    TRACE("Deleting shader object %u\n", This->baseShader.prgId);
    GL_EXTCALL(glDeleteObjectARB(This->baseShader.prgId));
    checkGLcall("glDeleteObjectARB");
    This->baseShader.prgId = 0;
    This->baseShader.is_compiled = FALSE;
}

static unsigned int glsl_program_key_hash(void *key) {
    glsl_program_key_t *k = (glsl_program_key_t *)key;

    unsigned int hash = k->vshader | k->pshader << 16;
    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    hash ^=  (hash >> 16);

    return hash;
}

static BOOL glsl_program_key_compare(void *keya, void *keyb) {
    glsl_program_key_t *ka = (glsl_program_key_t *)keya;
    glsl_program_key_t *kb = (glsl_program_key_t *)keyb;

    return ka->vshader == kb->vshader && ka->pshader == kb->pshader;
}

static HRESULT shader_glsl_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_glsl_priv *priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_glsl_priv));
    priv->glsl_program_lookup = hash_table_create(glsl_program_key_hash, glsl_program_key_compare);
    This->shader_priv = priv;
    return WINED3D_OK;
}

static void shader_glsl_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    struct shader_glsl_priv *priv = (struct shader_glsl_priv *)This->shader_priv;
    int i;

    for (i = 0; i < tex_type_count; ++i)
    {
        if (priv->depth_blt_program[i])
        {
            GL_EXTCALL(glDeleteObjectARB(priv->depth_blt_program[i]));
        }
    }

    hash_table_destroy(priv->glsl_program_lookup, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, This->shader_priv);
    This->shader_priv = NULL;
}

static BOOL shader_glsl_dirty_const(IWineD3DDevice *iface) {
    /* TODO: GL_EXT_bindable_uniform can be used to share constants across shaders */
    return FALSE;
}

static void shader_glsl_generate_pshader(IWineD3DPixelShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    const char *fragcolor;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;

    /* Create the hw GLSL shader object and assign it as the baseShader.prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));

    shader_addline(buffer, "#version 120\n");

    if (GL_SUPPORT(ARB_DRAW_BUFFERS)) {
        shader_addline(buffer, "#extension GL_ARB_draw_buffers : enable\n");
    }
    if (GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
        /* The spec says that it doesn't have to be explicitly enabled, but the nvidia
         * drivers write a warning if we don't do so
         */
        shader_addline(buffer, "#extension GL_ARB_texture_rectangle : enable\n");
    }

    /* Base Declarations */
    shader_generate_glsl_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* Pack 3.0 inputs */
    if (This->baseShader.hex_version >= WINED3DPS_VERSION(3,0)) {

        if(((IWineD3DDeviceImpl *) This->baseShader.device)->strided_streams.u.s.position_transformed) {
            This->vertexprocessing = pretransformed;
            pshader_glsl_input_pack(buffer, This->semantics_in, iface);
        } else if(!use_vs((IWineD3DDeviceImpl *) This->baseShader.device)) {
            This->vertexprocessing = fixedfunction;
            pshader_glsl_input_pack(buffer, This->semantics_in, iface);
        } else {
            This->vertexprocessing = vertexshader;
        }
    }

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* Pixel shaders < 2.0 place the resulting color in R0 implicitly */
    if (This->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
        /* Some older cards like GeforceFX ones don't support multiple buffers, so also not gl_FragData */
        if(GL_SUPPORT(ARB_DRAW_BUFFERS))
            shader_addline(buffer, "gl_FragData[0] = R0;\n");
        else
            shader_addline(buffer, "gl_FragColor = R0;\n");
    }

    if(GL_SUPPORT(ARB_DRAW_BUFFERS)) {
        fragcolor = "gl_FragData[0]";
    } else {
        fragcolor = "gl_FragColor";
    }
    if(This->srgb_enabled) {
        shader_addline(buffer, "tmp0.xyz = pow(%s.xyz, vec3(%f, %f, %f)) * vec3(%f, %f, %f) - vec3(%f, %f, %f);\n",
                        fragcolor, srgb_pow, srgb_pow, srgb_pow, srgb_mul_high, srgb_mul_high, srgb_mul_high,
                        srgb_sub_high, srgb_sub_high, srgb_sub_high);
        shader_addline(buffer, "tmp1.xyz = %s.xyz * srgb_mul_low.xyz;\n", fragcolor);
        shader_addline(buffer, "%s.x = %s.x < srgb_comparison.x ? tmp1.x : tmp0.x;\n", fragcolor, fragcolor);
        shader_addline(buffer, "%s.y = %s.y < srgb_comparison.y ? tmp1.y : tmp0.y;\n", fragcolor, fragcolor);
        shader_addline(buffer, "%s.z = %s.z < srgb_comparison.z ? tmp1.z : tmp0.z;\n", fragcolor, fragcolor);
        shader_addline(buffer, "%s = clamp(%s, 0.0, 1.0);\n", fragcolor, fragcolor);
    }
    /* Pixel shader < 3.0 do not replace the fog stage.
     * This implements linear fog computation and blending.
     * TODO: non linear fog
     * NOTE: gl_Fog.start and gl_Fog.end don't hold fog start s and end e but
     * -1/(e-s) and e/(e-s) respectively.
     */
    if(This->baseShader.hex_version < WINED3DPS_VERSION(3,0)) {
        shader_addline(buffer, "float Fog = clamp(gl_FogFragCoord * gl_Fog.start + gl_Fog.end, 0.0, 1.0);\n");
        shader_addline(buffer, "%s.xyz = mix(gl_Fog.color.xyz, %s.xyz, Fog);\n", fragcolor, fragcolor);
    }

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u\n", shader_obj);
    GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer->buffer, NULL));
    GL_EXTCALL(glCompileShaderARB(shader_obj));
    print_glsl_info_log(&GLINFO_LOCATION, shader_obj);

    /* Store the shader object */
    This->baseShader.prgId = shader_obj;
}

static void shader_glsl_generate_vshader(IWineD3DVertexShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;

    /* Create the hw GLSL shader program and assign it as the baseShader.prgId */
    GLhandleARB shader_obj = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));

    shader_addline(buffer, "#version 120\n");

    /* Base Declarations */
    shader_generate_glsl_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* Unpack 3.0 outputs */
    if (This->baseShader.hex_version >= WINED3DVS_VERSION(3,0)) {
        shader_addline(buffer, "order_ps_input(OUT);\n");
    } else {
        shader_addline(buffer, "order_ps_input();\n");
    }

    /* If this shader doesn't use fog copy the z coord to the fog coord so that we can use table fog */
    if (!reg_maps->fog)
        shader_addline(buffer, "gl_FogFragCoord = gl_Position.z;\n");

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad.
     */
    shader_addline(buffer, "gl_Position.y = gl_Position.y * posFixup.y;\n");
    shader_addline(buffer, "gl_Position.xy += posFixup.zw * gl_Position.ww;\n");

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     *
     * Basically we want (in homogeneous coordinates) z = z * 2 - 1. However, shaders are run
     * before the homogeneous divide, so we have to take the w into account: z = ((z / w) * 2 - 1) * w,
     * which is the same as z = z * 2 - w.
     */
    shader_addline(buffer, "gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");

    shader_addline(buffer, "}\n");

    TRACE("Compiling shader object %u\n", shader_obj);
    GL_EXTCALL(glShaderSourceARB(shader_obj, 1, (const char**)&buffer->buffer, NULL));
    GL_EXTCALL(glCompileShaderARB(shader_obj));
    print_glsl_info_log(&GLINFO_LOCATION, shader_obj);

    /* Store the shader object */
    This->baseShader.prgId = shader_obj;
}

static void shader_glsl_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct shader_caps *pCaps) {
    /* Nvidia Geforce6/7 or Ati R4xx/R5xx cards with GLSL support, support VS 3.0 but older Nvidia/Ati
     * models with GLSL support only support 2.0. In case of nvidia we can detect VS 2.0 support using
     * vs_nv_version which is based on NV_vertex_program.
     * For Ati cards there's no way using glsl (it abstracts the lowlevel info away) and also not
     * using ARB_vertex_program. It is safe to assume that when a card supports pixel shader 2.0 it
     * supports vertex shader 2.0 too and the way around. We can detect ps2.0 using the maximum number
     * of native instructions, so use that here. For more info see the pixel shader versioning code below.
     */
    if((GLINFO_LOCATION.vs_nv_version == VS_VERSION_20) || (GLINFO_LOCATION.ps_arb_max_instructions <= 512))
        pCaps->VertexShaderVersion = WINED3DVS_VERSION(2,0);
    else
        pCaps->VertexShaderVersion = WINED3DVS_VERSION(3,0);
    TRACE_(d3d_caps)("Hardware vertex shader version %d.%d enabled (GLSL)\n", (pCaps->VertexShaderVersion >> 8) & 0xff, pCaps->VertexShaderVersion & 0xff);
    pCaps->MaxVertexShaderConst = GL_LIMITS(vshader_constantsF);

    /* Older DX9-class videocards (GeforceFX / Radeon >9500/X*00) only support pixel shader 2.0/2.0a/2.0b.
     * In OpenGL the extensions related to GLSL abstract lowlevel GL info away which is needed
     * to distinguish between 2.0 and 3.0 (and 2.0a/2.0b). In case of Nvidia we use their fragment
     * program extensions. On other hardware including ATI GL_ARB_fragment_program offers the info
     * in max native instructions. Intel and others also offer the info in this extension but they
     * don't support GLSL (at least on Windows).
     *
     * PS2.0 requires at least 96 instructions, 2.0a/2.0b go up to 512. Assume that if the number
     * of instructions is 512 or less we have to do with ps2.0 hardware.
     * NOTE: ps3.0 hardware requires 512 or more instructions but ati and nvidia offer 'enough' (1024 vs 4096) on their most basic ps3.0 hardware.
     */
    if((GLINFO_LOCATION.ps_nv_version == PS_VERSION_20) || (GLINFO_LOCATION.ps_arb_max_instructions <= 512))
        pCaps->PixelShaderVersion = WINED3DPS_VERSION(2,0);
    else
        pCaps->PixelShaderVersion = WINED3DPS_VERSION(3,0);

    /* FIXME: The following line is card dependent. -8.0 to 8.0 is the
     * Direct3D minimum requirement.
     *
     * Both GL_ARB_fragment_program and GLSL require a "maximum representable magnitude"
     * of colors to be 2^10, and 2^32 for other floats. Should we use 1024 here?
     *
     * The problem is that the refrast clamps temporary results in the shader to
     * [-MaxValue;+MaxValue]. If the card's max value is bigger than the one we advertize here,
     * then applications may miss the clamping behavior. On the other hand, if it is smaller,
     * the shader will generate incorrect results too. Unfortunately, GL deliberately doesn't
     * offer a way to query this.
     */
    pCaps->PixelShader1xMaxValue = 8.0;
    TRACE_(d3d_caps)("Hardware pixel shader version %d.%d enabled (GLSL)\n", (pCaps->PixelShaderVersion >> 8) & 0xff, pCaps->PixelShaderVersion & 0xff);
}

static BOOL shader_glsl_conv_supported(WINED3DFORMAT fmt) {
    TRACE("Checking shader format support for format %s:", debug_d3dformat(fmt));
    switch(fmt) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
        case WINED3DFMT_X8L8V8U8:
        case WINED3DFMT_L6V5U5:
        case WINED3DFMT_Q8W8V8U8:
        case WINED3DFMT_ATI2N:
            TRACE("[OK]\n");
            return TRUE;
        default:
            TRACE("[FAILED\n");
            return FALSE;
    }
}

static const SHADER_HANDLER shader_glsl_instruction_handler_table[WINED3DSIH_TABLE_SIZE] =
{
    /* WINED3DSIH_ABS           */ shader_glsl_map2gl,
    /* WINED3DSIH_ADD           */ shader_glsl_arith,
    /* WINED3DSIH_BEM           */ pshader_glsl_bem,
    /* WINED3DSIH_BREAK         */ shader_glsl_break,
    /* WINED3DSIH_BREAKC        */ shader_glsl_breakc,
    /* WINED3DSIH_BREAKP        */ NULL,
    /* WINED3DSIH_CALL          */ shader_glsl_call,
    /* WINED3DSIH_CALLNZ        */ shader_glsl_callnz,
    /* WINED3DSIH_CMP           */ shader_glsl_cmp,
    /* WINED3DSIH_CND           */ shader_glsl_cnd,
    /* WINED3DSIH_CRS           */ shader_glsl_cross,
    /* WINED3DSIH_DCL           */ NULL,
    /* WINED3DSIH_DEF           */ NULL,
    /* WINED3DSIH_DEFB          */ NULL,
    /* WINED3DSIH_DEFI          */ NULL,
    /* WINED3DSIH_DP2ADD        */ pshader_glsl_dp2add,
    /* WINED3DSIH_DP3           */ shader_glsl_dot,
    /* WINED3DSIH_DP4           */ shader_glsl_dot,
    /* WINED3DSIH_DST           */ shader_glsl_dst,
    /* WINED3DSIH_DSX           */ shader_glsl_map2gl,
    /* WINED3DSIH_DSY           */ shader_glsl_map2gl,
    /* WINED3DSIH_ELSE          */ shader_glsl_else,
    /* WINED3DSIH_ENDIF         */ shader_glsl_end,
    /* WINED3DSIH_ENDLOOP       */ shader_glsl_end,
    /* WINED3DSIH_ENDREP        */ shader_glsl_end,
    /* WINED3DSIH_EXP           */ shader_glsl_map2gl,
    /* WINED3DSIH_EXPP          */ shader_glsl_expp,
    /* WINED3DSIH_FRC           */ shader_glsl_map2gl,
    /* WINED3DSIH_IF            */ shader_glsl_if,
    /* WINED3DSIH_IFC           */ shader_glsl_ifc,
    /* WINED3DSIH_LABEL         */ shader_glsl_label,
    /* WINED3DSIH_LIT           */ shader_glsl_lit,
    /* WINED3DSIH_LOG           */ shader_glsl_log,
    /* WINED3DSIH_LOGP          */ shader_glsl_log,
    /* WINED3DSIH_LOOP          */ shader_glsl_loop,
    /* WINED3DSIH_LRP           */ shader_glsl_lrp,
    /* WINED3DSIH_M3x2          */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x3          */ shader_glsl_mnxn,
    /* WINED3DSIH_M3x4          */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x3          */ shader_glsl_mnxn,
    /* WINED3DSIH_M4x4          */ shader_glsl_mnxn,
    /* WINED3DSIH_MAD           */ shader_glsl_mad,
    /* WINED3DSIH_MAX           */ shader_glsl_map2gl,
    /* WINED3DSIH_MIN           */ shader_glsl_map2gl,
    /* WINED3DSIH_MOV           */ shader_glsl_mov,
    /* WINED3DSIH_MOVA          */ shader_glsl_mov,
    /* WINED3DSIH_MUL           */ shader_glsl_arith,
    /* WINED3DSIH_NOP           */ NULL,
    /* WINED3DSIH_NRM           */ shader_glsl_map2gl,
    /* WINED3DSIH_PHASE         */ NULL,
    /* WINED3DSIH_POW           */ shader_glsl_pow,
    /* WINED3DSIH_RCP           */ shader_glsl_rcp,
    /* WINED3DSIH_REP           */ shader_glsl_rep,
    /* WINED3DSIH_RET           */ NULL,
    /* WINED3DSIH_RSQ           */ shader_glsl_rsq,
    /* WINED3DSIH_SETP          */ NULL,
    /* WINED3DSIH_SGE           */ shader_glsl_compare,
    /* WINED3DSIH_SGN           */ shader_glsl_map2gl,
    /* WINED3DSIH_SINCOS        */ shader_glsl_sincos,
    /* WINED3DSIH_SLT           */ shader_glsl_compare,
    /* WINED3DSIH_SUB           */ shader_glsl_arith,
    /* WINED3DSIH_TEX           */ pshader_glsl_tex,
    /* WINED3DSIH_TEXBEM        */ pshader_glsl_texbem,
    /* WINED3DSIH_TEXBEML       */ pshader_glsl_texbem,
    /* WINED3DSIH_TEXCOORD      */ pshader_glsl_texcoord,
    /* WINED3DSIH_TEXDEPTH      */ pshader_glsl_texdepth,
    /* WINED3DSIH_TEXDP3        */ pshader_glsl_texdp3,
    /* WINED3DSIH_TEXDP3TEX     */ pshader_glsl_texdp3tex,
    /* WINED3DSIH_TEXKILL       */ pshader_glsl_texkill,
    /* WINED3DSIH_TEXLDD        */ NULL,
    /* WINED3DSIH_TEXLDL        */ shader_glsl_texldl,
    /* WINED3DSIH_TEXM3x2DEPTH  */ pshader_glsl_texm3x2depth,
    /* WINED3DSIH_TEXM3x2PAD    */ pshader_glsl_texm3x2pad,
    /* WINED3DSIH_TEXM3x2TEX    */ pshader_glsl_texm3x2tex,
    /* WINED3DSIH_TEXM3x3       */ pshader_glsl_texm3x3,
    /* WINED3DSIH_TEXM3x3DIFF   */ NULL,
    /* WINED3DSIH_TEXM3x3PAD    */ pshader_glsl_texm3x3pad,
    /* WINED3DSIH_TEXM3x3SPEC   */ pshader_glsl_texm3x3spec,
    /* WINED3DSIH_TEXM3x3TEX    */ pshader_glsl_texm3x3tex,
    /* WINED3DSIH_TEXM3x3VSPEC  */ pshader_glsl_texm3x3vspec,
    /* WINED3DSIH_TEXREG2AR     */ pshader_glsl_texreg2ar,
    /* WINED3DSIH_TEXREG2GB     */ pshader_glsl_texreg2gb,
    /* WINED3DSIH_TEXREG2RGB    */ pshader_glsl_texreg2rgb,
};

const shader_backend_t glsl_shader_backend = {
    shader_glsl_instruction_handler_table,
    shader_glsl_select,
    shader_glsl_select_depth_blt,
    shader_glsl_deselect_depth_blt,
    shader_glsl_load_constants,
    shader_glsl_cleanup,
    shader_glsl_color_correction,
    shader_glsl_destroy,
    shader_glsl_alloc,
    shader_glsl_free,
    shader_glsl_dirty_const,
    shader_glsl_generate_pshader,
    shader_glsl_generate_vshader,
    shader_glsl_get_caps,
    shader_glsl_conv_supported,
};
