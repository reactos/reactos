/*
 * Fixed-function pipeline replacement implemented using HLSL shaders
 *
 * Copyright 2006 Jason Green
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2007-2009,2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 * Copyright 2022,2024 Elizabeth Figura for CodeWeavers
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

#include "wined3d_private.h"
#include <vkd3d_shader.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static const char *get_material_colour_source(enum wined3d_material_color_source mcs, const char *material)
{
    switch (mcs)
    {
        case WINED3D_MCS_MATERIAL:
            return material;
        case WINED3D_MCS_COLOR1:
            return "i.diffuse";
        case WINED3D_MCS_COLOR2:
            return "i.specular";
        default:
            ERR("Invalid material color source %#x.\n", mcs);
            return "<invalid>";
    }
}

static void generate_lighting_footer(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, unsigned int idx, bool legacy_lighting)
{
    shader_addline(buffer, "        diffuse += saturate(dot(dir, normal)) * c.lights[%u].diffuse.xyz * att;\n", idx);
    if (settings->localviewer)
        shader_addline(buffer, "        t = dot(normal, ffp_normalize(dir - ffp_normalize(ec_pos.xyz)));\n");
    else
        shader_addline(buffer, "        t = dot(normal, ffp_normalize(dir + float3(0.0, 0.0, -1.0)));\n");
    if (settings->specular_enable)
    {
        shader_addline(buffer, "        if (dot(dir, normal) > 0.0 && t > 0.0");
        if (legacy_lighting)
            shader_addline(buffer, " && c.material.power > 0.0");
        shader_addline(buffer, ")\n");
        shader_addline(buffer, "            specular += pow(t, c.material.power) * c.lights[%u].specular * att;\n", idx);
    }
}

static void generate_lighting(struct wined3d_string_buffer *buffer,
        const struct wined3d_ffp_vs_settings *settings, bool legacy_lighting)
{
    const char *ambient, *diffuse, *specular, *emissive;
    unsigned int idx = 0;

    if (!settings->lighting)
    {
        if (settings->diffuse)
            shader_addline(buffer, "    o.diffuse = i.diffuse;\n");
        else
            shader_addline(buffer, "    o.diffuse = 1.0;\n");
        shader_addline(buffer, "    o.specular = i.specular;\n");
        return;
    }

    shader_addline(buffer, "    float3 ambient = c.ambient_colour.xyz;\n");
    shader_addline(buffer, "    float3 diffuse = 0.0;\n");
    shader_addline(buffer, "    float3 dir, dst;\n");
    shader_addline(buffer, "    float att, t, range, falloff, cos_htheta, cos_hphi;\n");

    if (settings->specular_enable)
        shader_addline(buffer, "    float4 specular = 0.0;\n");

    ambient = get_material_colour_source(settings->ambient_source, "c.material.ambient");
    diffuse = get_material_colour_source(settings->diffuse_source, "c.material.diffuse");
    specular = get_material_colour_source(settings->specular_source, "c.material.specular");
    emissive = get_material_colour_source(settings->emissive_source, "c.material.emissive");

    for (unsigned int i = 0; i < settings->point_light_count; ++i)
    {
        shader_addline(buffer, "    dir = c.lights[%u].position.xyz - ec_pos.xyz;\n", idx);
        shader_addline(buffer, "    dst.z = dot(dir, dir);\n");
        shader_addline(buffer, "    dst.y = sqrt(dst.z);\n");
        shader_addline(buffer, "    dst.x = 1.0;\n");

        shader_addline(buffer, "    range = c.lights[%u].packed_params.x;\n", idx);

        if (legacy_lighting)
        {
            shader_addline(buffer, "    dst.y = (range - dst.y) / range;\n");
            shader_addline(buffer, "    dst.z = dst.y * dst.y;\n");
            shader_addline(buffer, "    if (dst.y > 0.0)\n{\n");
            shader_addline(buffer, "        att = dot(dst.xyz, c.lights[%u].attenuation.xyz);\n", idx);
        }
        else
        {
            shader_addline(buffer, "    if (dst.y <= range)\n{\n");
            shader_addline(buffer, "        att = 1.0 / dot(dst.xyz, c.lights[%u].attenuation.xyz);\n", idx);
        }
        shader_addline(buffer, "        ambient += c.lights[%u].ambient.xyz * att;\n", idx);
        if (settings->normal)
        {
            shader_addline(buffer, "        dir = ffp_normalize(dir);\n");
            generate_lighting_footer(buffer, settings, idx, legacy_lighting);
        }
        shader_addline(buffer, "    }\n");

        ++idx;
    }

    for (unsigned int i = 0; i < settings->spot_light_count; ++i)
    {
        shader_addline(buffer, "    dir = c.lights[%u].position.xyz - ec_pos.xyz;\n", idx);
        shader_addline(buffer, "    dst.z = dot(dir, dir);\n");
        shader_addline(buffer, "    dst.y = sqrt(dst.z);\n");
        shader_addline(buffer, "    dst.x = 1.0;\n");

        shader_addline(buffer, "    range = c.lights[%u].packed_params.x;\n", idx);
        shader_addline(buffer, "    falloff = c.lights[%u].packed_params.y;\n", idx);
        shader_addline(buffer, "    cos_htheta = c.lights[%u].packed_params.z;\n", idx);
        shader_addline(buffer, "    cos_hphi = c.lights[%u].packed_params.w;\n", idx);

        if (legacy_lighting)
        {
            shader_addline(buffer, "    dst.y = (range - dst.y) / range;\n");
            shader_addline(buffer, "    dst.z = dst.y * dst.y;\n");
            shader_addline(buffer, "    if (dst.y > 0.0)\n{\n");
        }
        else
        {
            shader_addline(buffer, "    if (dst.y <= range)\n{\n");
        }
        shader_addline(buffer, "        dir = ffp_normalize(dir);\n");
        shader_addline(buffer, "        t = dot(-dir, ffp_normalize(c.lights[%u].direction.xyz));\n", idx);
        shader_addline(buffer, "        if (t > cos_htheta) att = 1.0;\n");
        shader_addline(buffer, "        else if (t <= cos_hphi) att = 0.0;\n");
        shader_addline(buffer, "        else att = pow((t - cos_hphi) / (cos_htheta - cos_hphi), falloff);\n");
        if (legacy_lighting)
            shader_addline(buffer, "        att *= dot(dst.xyz, c.lights[%u].attenuation.xyz);\n", idx);
        else
            shader_addline(buffer, "        att /= dot(dst.xyz, c.lights[%u].attenuation.xyz);\n", idx);
        shader_addline(buffer, "        ambient += c.lights[%u].ambient.xyz * att;\n", idx);
        if (settings->normal)
            generate_lighting_footer(buffer, settings, idx, legacy_lighting);
        shader_addline(buffer, "    }\n");

        ++idx;
    }

    for (unsigned int i = 0; i < settings->directional_light_count; ++i)
    {
        shader_addline(buffer, "    ambient += c.lights[%u].ambient.xyz;\n", idx);
        if (settings->normal)
        {
            shader_addline(buffer, "    att = 1.0;\n");
            shader_addline(buffer, "    dir = ffp_normalize(c.lights[%u].direction.xyz);\n", idx);
            generate_lighting_footer(buffer, settings, idx, legacy_lighting);
        }

        ++idx;
    }

    for (unsigned int i = 0; i < settings->parallel_point_light_count; ++i)
    {
        shader_addline(buffer, "    ambient += c.lights[%u].ambient.xyz;\n", idx);
        if (settings->normal)
        {
            shader_addline(buffer, "    att = 1.0;\n");
            shader_addline(buffer, "    dir = ffp_normalize(c.lights[%u].position.xyz);\n", idx);
            generate_lighting_footer(buffer, settings, idx, legacy_lighting);
        }

        ++idx;
    }

    shader_addline(buffer, "    o.diffuse.xyz = %s.xyz * ambient + %s.xyz * diffuse + %s.xyz;\n",
            ambient, diffuse, emissive);
    shader_addline(buffer, "    o.diffuse.w = %s.w;\n", diffuse);
    if (settings->specular_enable)
        shader_addline(buffer, "    o.specular = %s * specular;\n", specular);
    else
        shader_addline(buffer, "    o.specular = i.specular;\n");
}

static bool ffp_hlsl_generate_vertex_shader(const struct wined3d_ffp_vs_settings *settings,
        struct wined3d_string_buffer *buffer, bool legacy_lighting)
{
    struct wined3d_string_buffer texcoord;

    /* This must be kept in sync with struct wined3d_ffp_vs_constants. */
    shader_addline(buffer, "uniform struct\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4x4 modelview_matrices[%u];\n", MAX_VERTEX_BLENDS);
    shader_addline(buffer, "    float4x4 projection_matrix;\n");
    shader_addline(buffer, "    float4x4 texture_matrices[%u];\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "    float4 point_params;\n");
    shader_addline(buffer, "    struct\n");
    shader_addline(buffer, "    {\n");
    shader_addline(buffer, "        float4 diffuse;\n");
    shader_addline(buffer, "        float4 ambient;\n");
    shader_addline(buffer, "        float4 specular;\n");
    shader_addline(buffer, "        float4 emissive;\n");
    shader_addline(buffer, "        float power;\n");
    shader_addline(buffer, "    } material;\n");
    shader_addline(buffer, "    float4 ambient_colour;\n");
    shader_addline(buffer, "    struct\n");
    shader_addline(buffer, "    {\n");
    shader_addline(buffer, "        float4 diffuse, specular, ambient;\n");
    shader_addline(buffer, "        float4 position, direction;\n");
    shader_addline(buffer, "        float4 packed_params;\n");
    shader_addline(buffer, "        float4 attenuation;\n");
    shader_addline(buffer, "    } lights[%u];\n", WINED3D_MAX_ACTIVE_LIGHTS);
    shader_addline(buffer, "} c;\n");

    shader_addline(buffer, "struct input\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 pos : POSITION;\n");
    shader_addline(buffer, "    float4 blend_weight : BLENDWEIGHT;\n");
    shader_addline(buffer, "    uint blend_indices : BLENDINDICES;\n");
    shader_addline(buffer, "    float3 normal : NORMAL;\n");
    shader_addline(buffer, "    float point_size : PSIZE;\n");
    shader_addline(buffer, "    float4 diffuse : COLOR0;\n");
    shader_addline(buffer, "    float4 specular : COLOR1;\n");
    shader_addline(buffer, "    float4 texcoord[%u] : TEXCOORD;\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "struct output\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 pos : POSITION;\n");
    shader_addline(buffer, "    float4 diffuse : COLOR0;\n");
    shader_addline(buffer, "    float4 specular : COLOR1;\n");
    for (unsigned int i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        if (((settings->texgen[i] & 0xffff0000) != WINED3DTSS_TCI_PASSTHRU) || (settings->texcoords & (1u << i)))
            shader_addline(buffer, "    float4 texcoord%u : TEXCOORD%u;\n", i, i);
    }
    if (settings->fog_mode == WINED3D_FFP_VS_FOG_DEPTH || settings->fog_mode == WINED3D_FFP_VS_FOG_RANGE)
        shader_addline(buffer, "    float fogcoord : FOG;\n");
    shader_addline(buffer, "    float point_size : PSIZE;\n");
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "float3 ffp_normalize(float3 n)\n{\n");
    shader_addline(buffer, "    float lensq = dot(n, n);\n");
    shader_addline(buffer, "    return lensq == 0.0 ? n : (n * rsqrt(lensq));\n");
    shader_addline(buffer, "}\n\n");

    shader_addline(buffer, "void main(in struct input i, out struct output o)\n");
    shader_addline(buffer, "{\n");

    shader_addline(buffer, "    i.blend_weight[%u] = 1.0;\n", settings->vertexblends);

    if (settings->transformed)
    {
        shader_addline(buffer, "    float4 ec_pos = float4(i.pos.xyz, 1.0);\n");
        /* We reuse the projection matrix to undo the transformation from clip
         * coordinates to pixel coordinates. */
        shader_addline(buffer, "    float4 out_pos = mul(c.projection_matrix, ec_pos);\n\n");

        /* Use a ternary; this is not the most natural way to write it but is
         * nicer to the compiler. */
        shader_addline(buffer, "    o.pos = (i.pos.w == 0.0 ? out_pos : out_pos / i.pos.w);\n");
    }
    else
    {
        for (unsigned int i = 0; i < settings->vertexblends; ++i)
            shader_addline(buffer, "    i.blend_weight[%u] -= i.blend_weight[%u];\n", settings->vertexblends, i);

        shader_addline(buffer, "    float4 ec_pos = 0.0;\n\n");
        for (unsigned int i = 0; i < settings->vertexblends + 1; ++i)
            shader_addline(buffer, "    ec_pos += i.blend_weight[%u] * mul(c.modelview_matrices[%u], float4(i.pos.xyz, 1.0));\n", i, i);

        shader_addline(buffer, "    o.pos = mul(c.projection_matrix, ec_pos);\n");
        shader_addline(buffer, "    ec_pos /= ec_pos.w;\n\n");
    }

    shader_addline(buffer, "    float3 normal = 0.0;\n");
    if (settings->normal)
    {
        if (!settings->vertexblends)
        {
            if (settings->transformed)
                shader_addline(buffer, "    normal = i.normal;\n");
            else
                shader_addline(buffer, "    normal = mul((float3x3)c.modelview_matrices[1], i.normal);\n");
        }
        else
        {
            for (unsigned int i = 0; i < settings->vertexblends + 1; ++i)
            {
                if (settings->transformed)
                    shader_addline(buffer, "    normal += i.blend_weight[%u] * i.normal;\n", i);
                else
                    shader_addline(buffer, "    normal += i.blend_weight[%u]"
                            " * mul((float3x3)c.modelview_matrices[%u], i.normal);\n", i, i);
            }
        }

        if (settings->normalize)
            shader_addline(buffer, "    normal = ffp_normalize(normal);\n");
    }

    generate_lighting(buffer, settings, legacy_lighting);

    string_buffer_init(&texcoord);
    for (unsigned int i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        string_buffer_clear(&texcoord);

        switch (settings->texgen[i] & 0xffff0000)
        {
            case WINED3DTSS_TCI_PASSTHRU:
                if (settings->texcoords & (1u << i))
                    shader_addline(&texcoord, "i.texcoord[%u]", settings->texgen[i] & 0x0000ffff);
                else
                    continue;
                break;

            default:
                FIXME("Unhandled texgen %#x.\n", settings->texgen[i]);
                break;
        }

        if (settings->transformed)
            shader_addline(buffer, "    o.texcoord%u = %s;\n", i, texcoord.buffer);
        else
            shader_addline(buffer, "    o.texcoord%u = mul(c.texture_matrices[%u], %s);\n", i, i, texcoord.buffer);
    }
    string_buffer_free(&texcoord);

    switch (settings->fog_mode)
    {
        case WINED3D_FFP_VS_FOG_OFF:
            break;

        case WINED3D_FFP_VS_FOG_FOGCOORD:
            /* This is FOGTABLEMODE == NONE, FOGVERTEXMODE == NONE.
             * The specular W is used in that case, even if we output fogcoord,
             * so there's no point in outputting fogcoord. */
            break;

        case WINED3D_FFP_VS_FOG_RANGE:
            shader_addline(buffer, "    o.fogcoord = length(ec_pos.xyz);\n");
            break;

        case WINED3D_FFP_VS_FOG_DEPTH:
            if (settings->transformed)
                shader_addline(buffer, "    o.fogcoord = ec_pos.z;\n");
            else
                shader_addline(buffer, "    o.fogcoord = abs(o.pos.z);\n");
            break;
    }

    shader_addline(buffer, "    o.point_size = %s / sqrt(c.point_params.x"
            " + c.point_params.y * length(ec_pos.xyz)"
            " + c.point_params.z * dot(ec_pos.xyz, ec_pos.xyz));\n",
            settings->per_vertex_point_size ? "i.point_size" : "c.point_params.w");

    shader_addline(buffer, "}\n");

    return true;
}

static const char *get_fragment_op_arg(struct wined3d_string_buffer *buffer,
        unsigned int argnum, unsigned int stage, unsigned int arg)
{
    const char *ret;

    if (arg == ARG_UNUSED)
        return "<unused arg>";

    switch (arg & WINED3DTA_SELECTMASK)
    {
        case WINED3DTA_DIFFUSE:
            ret = "i.diffuse";
            break;

        case WINED3DTA_CURRENT:
            ret = "ret";
            break;

        case WINED3DTA_TEXTURE:
            switch (stage)
            {
                case 0: ret = "tex0"; break;
                case 1: ret = "tex1"; break;
                case 2: ret = "tex2"; break;
                case 3: ret = "tex3"; break;
                case 4: ret = "tex4"; break;
                case 5: ret = "tex5"; break;
                case 6: ret = "tex6"; break;
                case 7: ret = "tex7"; break;
                default:
                    ret = "<invalid texture>";
                    break;
            }
            break;

        case WINED3DTA_TFACTOR:
            ret = "c.texture_factor";
            break;

        case WINED3DTA_SPECULAR:
            ret = "i.specular";
            break;

        case WINED3DTA_TEMP:
            ret = "temp_reg";
            break;

        case WINED3DTA_CONSTANT:
            switch (stage)
            {
                case 0: ret = "c.texture_constants[0]"; break;
                case 1: ret = "c.texture_constants[1]"; break;
                case 2: ret = "c.texture_constants[2]"; break;
                case 3: ret = "c.texture_constants[3]"; break;
                case 4: ret = "c.texture_constants[4]"; break;
                case 5: ret = "c.texture_constants[5]"; break;
                case 6: ret = "c.texture_constants[6]"; break;
                case 7: ret = "c.texture_constants[7]"; break;
                default:
                    ret = "<invalid constant>";
                    break;
            }
            break;

        default:
            return "<unhandled arg>";
    }

    if (arg & WINED3DTA_COMPLEMENT)
    {
        shader_addline(buffer, "    arg%u = 1.0 - %s;\n", argnum, ret);
        if (argnum == 0)
            ret = "arg0";
        else if (argnum == 1)
            ret = "arg1";
        else if (argnum == 2)
            ret = "arg2";
    }

    if (arg & WINED3DTA_ALPHAREPLICATE)
    {
        shader_addline(buffer, "    arg%u = %s.w;\n", argnum, ret);
        if (argnum == 0)
            ret = "arg0";
        else if (argnum == 1)
            ret = "arg1";
        else if (argnum == 2)
            ret = "arg2";
    }

    return ret;
}

static void generate_fragment_op(struct wined3d_string_buffer *buffer, unsigned int stage, bool color,
        bool alpha, bool tmp_dst, unsigned int op, unsigned int dw_arg0, unsigned int dw_arg1, unsigned int dw_arg2)
{
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;

    if (color && alpha)
        dstmask = "";
    else if (color)
        dstmask = ".xyz";
    else
        dstmask = ".w";

    dstreg = tmp_dst ? "temp_reg" : "ret";

    arg0 = get_fragment_op_arg(buffer, 0, stage, dw_arg0);
    arg1 = get_fragment_op_arg(buffer, 1, stage, dw_arg1);
    arg2 = get_fragment_op_arg(buffer, 2, stage, dw_arg2);

    switch (op)
    {
        case WINED3D_TOP_DISABLE:
            break;

        case WINED3D_TOP_SELECT_ARG1:
            shader_addline(buffer, "    %s%s = %s%s;\n", dstreg, dstmask, arg1, dstmask);
            break;

        case WINED3D_TOP_SELECT_ARG2:
            shader_addline(buffer, "    %s%s = %s%s;\n", dstreg, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE:
            shader_addline(buffer, "    %s%s = %s%s * %s%s;\n", dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE_4X:
            shader_addline(buffer, "    %s%s = saturate(%s%s * %s%s * 4.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_MODULATE_2X:
            shader_addline(buffer, "    %s%s = saturate(%s%s * %s%s * 2.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD:
            shader_addline(buffer, "    %s%s = saturate(%s%s + %s%s);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SIGNED:
            shader_addline(buffer, "    %s%s = saturate(%s%s + (%s - 0.5)%s);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SIGNED_2X:
            shader_addline(buffer, "    %s%s = saturate((%s%s + (%s - 0.5)%s) * 2.0);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_SUBTRACT:
            shader_addline(buffer, "    %s%s = saturate(%s%s - %s%s);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask);
            break;

        case WINED3D_TOP_ADD_SMOOTH:
            shader_addline(buffer, "    %s%s = saturate((1.0 - %s)%s * %s%s + %s%s);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1, dstmask);
            break;

        case WINED3D_TOP_BLEND_DIFFUSE_ALPHA:
            arg0 = get_fragment_op_arg(buffer, 0, stage, WINED3DTA_DIFFUSE);
            shader_addline(buffer, "    %s%s = lerp(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            arg0 = get_fragment_op_arg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "    %s%s = lerp(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_FACTOR_ALPHA:
            arg0 = get_fragment_op_arg(buffer, 0, stage, WINED3DTA_TFACTOR);
            shader_addline(buffer, "    %s%s = lerp(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
            arg0 = get_fragment_op_arg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "    %s%s = saturate(%s%s * (1.0 - %s.w) + %s%s);\n",
                    dstreg, dstmask, arg2, dstmask, arg0, arg1, dstmask);
            break;

        case WINED3D_TOP_BLEND_CURRENT_ALPHA:
            arg0 = get_fragment_op_arg(buffer, 0, stage, WINED3DTA_CURRENT);
            shader_addline(buffer, "    %s%s = lerp(%s%s, %s%s, %s.w);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0);
            break;

        case WINED3D_TOP_MODULATE_ALPHA_ADD_COLOR:
            shader_addline(buffer, "    %s%s = saturate(%s%s * %s.w + %s%s);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, arg1, dstmask);
            break;

        case WINED3D_TOP_MODULATE_COLOR_ADD_ALPHA:
            shader_addline(buffer, "    %s%s = saturate(%s%s * %s%s + %s.w);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1);
            break;

        case WINED3D_TOP_MODULATE_INVALPHA_ADD_COLOR:
            shader_addline(buffer, "    %s%s = saturate(%s%s * (1.0 - %s.w) + %s%s);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, arg1, dstmask);
            break;
        case WINED3D_TOP_MODULATE_INVCOLOR_ADD_ALPHA:
            shader_addline(buffer, "    %s%s = saturate((1.0 - %s)%s * %s%s + %s.w);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg1);
            break;

        case WINED3D_TOP_BUMPENVMAP:
        case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
            /* These are handled in the first pass, nothing to do. */
            break;

        case WINED3D_TOP_DOTPRODUCT3:
            shader_addline(buffer, "    %s%s = saturate(dot(%s.xyz - 0.5, %s.xyz - 0.5) * 4.0);\n",
                    dstreg, dstmask, arg1, arg2);
            break;

        case WINED3D_TOP_MULTIPLY_ADD:
            shader_addline(buffer, "    %s%s = saturate(%s%s * %s%s + %s%s);\n",
                    dstreg, dstmask, arg1, dstmask, arg2, dstmask, arg0, dstmask);
            break;

        case WINED3D_TOP_LERP:
            /* MSDN isn't quite right here. */
            shader_addline(buffer, "    %s%s = lerp(%s%s, %s%s, %s%s);\n",
                    dstreg, dstmask, arg2, dstmask, arg1, dstmask, arg0, dstmask);
            break;

        default:
            FIXME("Unhandled operation %#x.\n", op);
            break;
    }
}

static bool ffp_hlsl_generate_pixel_shader(const struct ffp_frag_settings *settings,
        struct wined3d_string_buffer *buffer)
{
    uint8_t tex_map = 0;
    unsigned int i;

    /* Find out which textures are read. */
    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        unsigned int arg0, arg1, arg2;

        if (settings->op[i].cop == WINED3D_TOP_DISABLE)
            break;

        arg0 = settings->op[i].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[i].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[i].carg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE
                || (i == 0 && settings->color_key_enabled))
            tex_map |= 1u << i;

        switch (settings->op[i].cop)
        {
            case WINED3D_TOP_BUMPENVMAP_LUMINANCE:
            case WINED3D_TOP_BUMPENVMAP:
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA:
            case WINED3D_TOP_BLEND_TEXTURE_ALPHA_PM:
                tex_map |= 1u << i;
                break;

            default:
                break;
        }

        if (settings->op[i].aop == WINED3D_TOP_DISABLE)
            continue;

        arg0 = settings->op[i].aarg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[i].aarg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[i].aarg2 & WINED3DTA_SELECTMASK;

        if (arg0 == WINED3DTA_TEXTURE || arg1 == WINED3DTA_TEXTURE || arg2 == WINED3DTA_TEXTURE)
            tex_map |= 1u << i;
    }

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        const char *sampler_type;

        if (!(tex_map & (1u << i)))
            continue;

        switch (settings->op[i].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                sampler_type = "1D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                sampler_type = "2D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                sampler_type = "3D";
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                sampler_type = "CUBE";
                break;
            default:
                FIXME("Unhandled sampler type %#x.\n", settings->op[i].tex_type);
                sampler_type = NULL;
                break;
        }
        if (sampler_type)
            shader_addline(buffer, "sampler%s ps_sampler%u : register(s%u);\n", sampler_type, i, i);

        shader_addline(buffer, "static float4 tex%u;\n", i);
    }

    /* This must be kept in sync with struct wined3d_ffp_vs_constants. */
    shader_addline(buffer, "uniform struct\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 texture_constants[%u];\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "    float4 texture_factor;\n");
    shader_addline(buffer, "    float4 specular_enable;\n");
    shader_addline(buffer, "    float4 color_key[2];\n");
    /* Bumpenv constants are manually packed. */
    shader_addline(buffer, "    float4 bumpenv_matrices[%u];\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "    float4 bumpenv_lscale[2];\n");
    shader_addline(buffer, "    float4 bumpenv_loffset[2];\n");
    shader_addline(buffer, "} c;\n");

    shader_addline(buffer, "struct input\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 pos : POSITION;\n");
    shader_addline(buffer, "    float4 diffuse : COLOR0;\n");
    shader_addline(buffer, "    float4 specular : COLOR1;\n");
    shader_addline(buffer, "    float4 texcoord[%u] : TEXCOORD;\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "struct output\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 colour : COLOR;\n");
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "void main(in struct input i, out struct output o)\n");
    shader_addline(buffer, "{\n");

    shader_addline(buffer, "    float4 texcoord[%u];\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "    float4 temp_reg = 0.0;\n\n");
    shader_addline(buffer, "    float4 ret, arg0, arg1, arg2;\n\n");

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        if (tex_map & (1u << i))
        {
            if (settings->pointsprite || (settings->texcoords_initialized & (1u << i)))
                shader_addline(buffer, "    texcoord[%u] = i.texcoord[%u];\n", i, i);
            else
                shader_addline(buffer, "    texcoord[%u] = 0.0;\n", i);
        }
    }

    /* Texture sampling. */

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES && settings->op[i].cop != WINED3D_TOP_DISABLE; ++i)
    {
        const char *texture_function, *coord_mask;
        bool proj;

        if (!(tex_map & (1u << i)))
            continue;

        if (settings->op[i].projected == WINED3D_PROJECTION_NONE)
        {
            proj = false;
        }
        else if (settings->op[i].projected == WINED3D_PROJECTION_COUNT4
                || settings->op[i].projected == WINED3D_PROJECTION_COUNT3)
        {
            proj = true;
        }
        else
        {
            FIXME("Unexpected projection mode %d.\n", settings->op[i].projected);
            proj = true;
        }

        switch (settings->op[i].tex_type)
        {
            case WINED3D_GL_RES_TYPE_TEX_1D:
                texture_function = "tex1D";
                coord_mask = "x";
                break;
            case WINED3D_GL_RES_TYPE_TEX_2D:
                texture_function = "tex2D";
                coord_mask = "xy";
                break;
            case WINED3D_GL_RES_TYPE_TEX_3D:
                texture_function = "tex3D";
                coord_mask = "xyz";
                break;
            case WINED3D_GL_RES_TYPE_TEX_CUBE:
                texture_function = "texCUBE";
                coord_mask = "xyz";
                break;
            default:
                FIXME("Unhandled texture type %#x.\n", settings->op[i].tex_type);
                texture_function = "";
                coord_mask = "xyzw";
                proj = false;
                break;
        }

        if (proj)
            coord_mask = "xyzw";

        if (i > 0
                && (settings->op[i - 1].cop == WINED3D_TOP_BUMPENVMAP
                || settings->op[i - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE))
        {
            shader_addline(buffer, "ret.xy = mul(transpose((float2x2)c.bumpenv_matrices[%u]), tex%u.xy);\n", i - 1, i - 1);

            /* With projective textures, texbem only divides the static texture
             * coordinate, not the displacement, so multiply the displacement
             * with the dividing parameter before sampling. */
            if (settings->op[i].projected != WINED3D_PROJECTION_NONE)
            {
                if (settings->op[i].projected == WINED3D_PROJECTION_COUNT4)
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * texcoord[%u].w) + texcoord[%u].xy;\n", i, i);
                    shader_addline(buffer, "ret.zw = texcoord[%u].ww;\n", i);
                }
                else
                {
                    shader_addline(buffer, "ret.xy = (ret.xy * texcoord[%u].z) + texcoord[%u].xy;\n", i, i);
                    shader_addline(buffer, "ret.zw = texcoord[%u].zz;\n", i);
                }
            }
            else
            {
                shader_addline(buffer, "ret = texcoord[%u] + ret.xyxy;\n", i);
            }

            shader_addline(buffer, "tex%u = %s%s(ps_sampler%u, ret.%s);\n",
                    i, texture_function, proj ? "proj" : "", i, coord_mask);

            if (settings->op[i - 1].cop == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
            {
                shader_addline(buffer, "tex%u *= saturate(tex%u.z * c.bumpenv_lscale[%u][%u] + c.bumpenv_loffset[%u][%u]);\n",
                        i, i - 1, (i - 1) / 4, (i - 1) % 4, (i - 1) / 4, (i - 1) % 4);
            }
        }
        else if (settings->op[i].projected == WINED3D_PROJECTION_COUNT3)
        {
            shader_addline(buffer, "    tex%u = %s%s(ps_sampler%u, texcoord[%u].xyzz);\n",
                    i, texture_function, proj ? "proj" : "", i, i);
        }
        else
        {
            shader_addline(buffer, "    tex%u = %s%s(ps_sampler%u, texcoord[%u].%s);\n",
                    i, texture_function, proj ? "proj" : "", i, i, coord_mask);
        }
    }

    if (settings->color_key_enabled)
    {
        shader_addline(buffer, "    if (all(tex0 >= c.color_key[0]) && all(tex0 < c.color_key[1]))\n");
        shader_addline(buffer, "        discard;\n");
    }

    shader_addline(buffer, "    ret = i.diffuse;\n");

    for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        bool op_equal;

        if (settings->op[i].cop == WINED3D_TOP_DISABLE)
            break;

        if (settings->op[i].cop == WINED3D_TOP_SELECT_ARG1
                && settings->op[i].aop == WINED3D_TOP_SELECT_ARG1)
            op_equal = settings->op[i].carg1 == settings->op[i].aarg1;
        else if (settings->op[i].cop == WINED3D_TOP_SELECT_ARG1
                && settings->op[i].aop == WINED3D_TOP_SELECT_ARG2)
            op_equal = settings->op[i].carg1 == settings->op[i].aarg2;
        else if (settings->op[i].cop == WINED3D_TOP_SELECT_ARG2
                && settings->op[i].aop == WINED3D_TOP_SELECT_ARG1)
            op_equal = settings->op[i].carg2 == settings->op[i].aarg1;
        else if (settings->op[i].cop == WINED3D_TOP_SELECT_ARG2
                && settings->op[i].aop == WINED3D_TOP_SELECT_ARG2)
            op_equal = settings->op[i].carg2 == settings->op[i].aarg2;
        else
            op_equal = settings->op[i].aop == settings->op[i].cop
                    && settings->op[i].carg0 == settings->op[i].aarg0
                    && settings->op[i].carg1 == settings->op[i].aarg1
                    && settings->op[i].carg2 == settings->op[i].aarg2;

        if (settings->op[i].aop == WINED3D_TOP_DISABLE)
        {
            generate_fragment_op(buffer, i, true, false, settings->op[i].tmp_dst,
                    settings->op[i].cop, settings->op[i].carg0,
                    settings->op[i].carg1, settings->op[i].carg2);
        }
        else if (op_equal)
        {
            generate_fragment_op(buffer, i, true, true, settings->op[i].tmp_dst,
                    settings->op[i].cop, settings->op[i].carg0,
                    settings->op[i].carg1, settings->op[i].carg2);
        }
        else if (settings->op[i].cop != WINED3D_TOP_BUMPENVMAP
                && settings->op[i].cop != WINED3D_TOP_BUMPENVMAP_LUMINANCE)
        {
            generate_fragment_op(buffer, i, true, false, settings->op[i].tmp_dst,
                    settings->op[i].cop, settings->op[i].carg0,
                    settings->op[i].carg1, settings->op[i].carg2);
            generate_fragment_op(buffer, i, false, true, settings->op[i].tmp_dst,
                    settings->op[i].aop, settings->op[i].aarg0,
                    settings->op[i].aarg1, settings->op[i].aarg2);
        }
    }

    shader_addline(buffer, "    o.colour = i.specular * c.specular_enable + ret;\n");

    shader_addline(buffer, "}\n");

    return true;
}

static bool compile_hlsl_shader(const struct wined3d_string_buffer *hlsl,
        struct vkd3d_shader_code *sm1, const char *profile)
{
    struct vkd3d_shader_hlsl_source_info hlsl_source_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO};
    struct vkd3d_shader_compile_info compile_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO};
    char *messages;
    int ret;

    compile_info.source.code = hlsl->buffer;
    compile_info.source.size = hlsl->content_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    compile_info.target_type = VKD3D_SHADER_TARGET_D3D_BYTECODE;
    compile_info.log_level = VKD3D_SHADER_LOG_WARNING;

    compile_info.next = &hlsl_source_info;
    hlsl_source_info.profile = profile;

    ret = vkd3d_shader_compile(&compile_info, sm1, &messages);
    if (messages && *messages && FIXME_ON(d3d_shader))
    {
        const char *ptr, *end, *line;

        FIXME("Shader log:\n");
        ptr = messages;
        end = ptr + strlen(ptr);
        while ((line = wined3d_get_line(&ptr, end)))
            FIXME("    %.*s", (int)(ptr - line), line);
        FIXME("\n");
    }
    vkd3d_shader_free_messages(messages);

    if (ret < 0)
    {
        ERR("Failed to compile HLSL, ret %d.\n", ret);
        return false;
    }

    return true;
}

bool ffp_hlsl_compile_vs(const struct wined3d_ffp_vs_settings *settings,
        struct wined3d_shader_desc *shader_desc, struct wined3d_device *device)
{
    struct wined3d_string_buffer string;
    struct vkd3d_shader_code sm1;

    if (!string_buffer_init(&string))
        return false;

    if (!ffp_hlsl_generate_vertex_shader(settings, &string, device->wined3d->flags & WINED3D_LEGACY_FFP_LIGHTING))
    {
        string_buffer_free(&string);
        return false;
    }

    if (!compile_hlsl_shader(&string, &sm1, "vs_2_a"))
    {
        string_buffer_free(&string);
        return false;
    }
    string_buffer_free(&string);

    shader_desc->byte_code = sm1.code;
    shader_desc->byte_code_size = ~(size_t)0;
    return true;
}

bool ffp_hlsl_compile_ps(const struct ffp_frag_settings *settings, struct wined3d_shader_desc *shader_desc)
{
    struct wined3d_string_buffer string;
    struct vkd3d_shader_code sm1;

    if (!string_buffer_init(&string))
        return false;

    if (!ffp_hlsl_generate_pixel_shader(settings, &string))
    {
        string_buffer_free(&string);
        return false;
    }

    if (!compile_hlsl_shader(&string, &sm1, "ps_2_a"))
    {
        string_buffer_free(&string);
        return false;
    }
    string_buffer_free(&string);

    shader_desc->byte_code = sm1.code;
    shader_desc->byte_code_size = ~(size_t)0;
    return true;
}
