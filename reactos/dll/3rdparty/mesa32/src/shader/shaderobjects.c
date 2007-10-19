/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2004-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file shaderobjects.c
 * ARB_shader_objects state management functions
 * \author Michal Krol
 */


#include "glheader.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "context.h"
#include "macros.h"
#include "hash.h"


void GLAPIENTRY
_mesa_DeleteObjectARB (GLhandleARB obj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_generic_intf **gen;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, obj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glDeleteObjectARB");
		return;
	}

	gen = (struct gl2_generic_intf **) (**unk).QueryInterface (unk, UIID_GENERIC);
	if (gen == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glDeleteObjectARB");
		return;
	}

	(**gen).Delete (gen);
	(**gen)._unknown.Release ((struct gl2_unknown_intf **) gen);
}

GLhandleARB GLAPIENTRY
_mesa_GetHandleARB (GLenum pname)
{
	GET_CURRENT_CONTEXT(ctx);

	switch (pname)
	{
	case GL_PROGRAM_OBJECT_ARB:
		if (ctx->ShaderObjects.current_program != NULL)
			return (**ctx->ShaderObjects.current_program)._container._generic.GetName (
				(struct gl2_generic_intf **) ctx->ShaderObjects.current_program);
		break;
	}

	return 0;
}

void GLAPIENTRY
_mesa_DetachObjectARB (GLhandleARB containerObj, GLhandleARB attachedObj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unkc, **unka;
	struct gl2_container_intf **con;
	struct gl2_generic_intf **att;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unkc = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, containerObj);
	unka = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, attachedObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unkc == NULL || unka == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glDetachObjectARB");
		return;
	}

	con = (struct gl2_container_intf **) (**unkc).QueryInterface (unkc, UIID_CONTAINER);
	if (con == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glDetachObjectARB");
		return;
	}

	att = (struct gl2_generic_intf **) (**unka).QueryInterface (unka, UIID_GENERIC);
	if (att == NULL)
	{
		(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
		_mesa_error (ctx, GL_INVALID_VALUE, "glDetachObjectARB");
		return;
	}

	if ((**con).Detach (con, att) == GL_FALSE)
	{
		(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
		(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
		return;
	}

	(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
	(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
}

GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB (GLenum shaderType)
{
	return _mesa_3dlabs_create_shader_object (shaderType);
}

void GLAPIENTRY
_mesa_ShaderSourceARB (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string,
					   const GLint *length)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_shader_intf **sha;
	GLint *offsets;
	GLsizei i;
	GLcharARB *source;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, shaderObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glShaderSourceARB");
		return;
	}

	sha = (struct gl2_shader_intf **) (**unk).QueryInterface (unk, UIID_SHADER);
	if (sha == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glShaderSourceARB");
		return;
	}

	/* this array holds offsets of where the appropriate string ends, thus the last
	element will be set to the total length of the source code */
	offsets = (GLint *) _mesa_malloc (count * sizeof (GLint));
	if (offsets == NULL)
	{
		(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
		_mesa_error (ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
		return;
	}

	for (i = 0; i < count; i++)
	{
		if (length == NULL || length[i] < 0)
			offsets[i] = _mesa_strlen (string[i]);
		else
			offsets[i] = length[i];
		/* accumulate string lengths */
		if (i > 0)
			offsets[i] += offsets[i - 1];
	}

	source = (GLcharARB *) _mesa_malloc ((offsets[count - 1] + 1) * sizeof (GLcharARB));
	if (source == NULL)
	{
		_mesa_free ((void *) offsets);
		(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
		_mesa_error (ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
		return;
	}

	for (i = 0; i < count; i++)
	{
		GLint start = (i > 0) ? offsets[i - 1] : 0;
		_mesa_memcpy (source + start, string[i], (offsets[i] - start) * sizeof (GLcharARB));
	}
	source[offsets[count - 1]] = '\0';

	(**sha).SetSource (sha, source, offsets, count);
	(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
}

void  GLAPIENTRY
_mesa_CompileShaderARB (GLhandleARB shaderObj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_shader_intf **sha;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, shaderObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glCompileShaderARB");
		return;
	}

	sha = (struct gl2_shader_intf **) (**unk).QueryInterface (unk, UIID_SHADER);
	if (sha == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glCompileShaderARB");
		return;
	}

	(**sha).Compile (sha);
	(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
}

GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB (void)
{
	return _mesa_3dlabs_create_program_object ();
}

void GLAPIENTRY
_mesa_AttachObjectARB (GLhandleARB containerObj, GLhandleARB obj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unkc, **unka;
	struct gl2_container_intf **con;
	struct gl2_generic_intf **att;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unkc = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, containerObj);
	unka = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, obj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unkc == NULL || unka == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glAttachObjectARB");
		return;
	}

	con = (struct gl2_container_intf **) (**unkc).QueryInterface (unkc, UIID_CONTAINER);
	if (con == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glAttachObjectARB");
		return;
	}

	att = (struct gl2_generic_intf **) (**unka).QueryInterface (unka, UIID_GENERIC);
	if (att == NULL)
	{
		(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
		_mesa_error (ctx, GL_INVALID_VALUE, "glAttachObjectARB");
		return;
	}

	if (!(**con).Attach (con, att))
	{
		(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
		(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
		return;
	}

	(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);
	(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
}

void GLAPIENTRY
_mesa_LinkProgramARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glLinkProgramARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glLinkProgramARB");
		return;
	}

	if (pro == ctx->ShaderObjects.current_program)
	{
		/* TODO re-install executable program */
	}

	(**pro).Link (pro);
	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

void GLAPIENTRY
_mesa_UseProgramObjectARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_program_intf **pro;

	if (programObj == 0)
	{
		pro = NULL;
	}
	else
	{
		struct gl2_unknown_intf **unk;

		_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
		unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
		_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

		if (unk == NULL)
		{
			_mesa_error (ctx, GL_INVALID_VALUE, "glUseProgramObjectARB");
			return;
		}

		pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
		if (pro == NULL)
		{
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUseProgramObjectARB");
			return;
		}

		if ((**pro).GetLinkStatus (pro) == GL_FALSE)
		{
			(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUseProgramObjectARB");
			return;
		}
	}

	if (ctx->ShaderObjects.current_program != NULL)
	{
		(**ctx->ShaderObjects.current_program)._container._generic._unknown.Release (
			(struct gl2_unknown_intf **) ctx->ShaderObjects.current_program);
	}

	ctx->ShaderObjects.current_program = pro;
}

void GLAPIENTRY
_mesa_ValidateProgramARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glValidateProgramARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glValidateProgramARB");
		return;
	}

	(**pro).Validate (pro);
	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

/*
Errors TODO

    The error INVALID_OPERATION is generated by the Uniform*ARB if the
    number of values loaded results in exceeding the declared extent of a
    uniform.

    The error INVALID_OPERATION is generated by the Uniform*ARB commands if
    the size does not match the size of the uniform declared in the shader.

    The error INVALID_OPERATION is generated by the Uniform*ARB commands if
    the type does not match the type of the uniform declared in the shader,
    if the uniform is not of type Boolean.

    The error INVALID_OPERATION is generated by the Uniform*ARB commands if
    <location> does not exist for the program object currently in use.

    The error INVALID_OPERATION is generated if a uniform command other than
    Uniform1i{v}ARB is used to load a sampler value.


*/

void GLAPIENTRY
_mesa_Uniform1fARB (GLint location, GLfloat v0)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1fARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform2fARB (GLint location, GLfloat v0, GLfloat v1)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2fARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform3fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3fARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform4fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4fARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform1iARB (GLint location, GLint v0)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1iARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform2iARB (GLint location, GLint v0, GLint v1)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2iARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform3iARB (GLint location, GLint v0, GLint v1, GLint v2)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3iARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform4iARB (GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4iARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform1fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform2fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform3fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform4fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform1ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1ivARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform2ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2ivARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform3ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3ivARB");
		return;
	}
}

void GLAPIENTRY
_mesa_Uniform4ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4ivARB");
		return;
	}
}

void GLAPIENTRY
_mesa_UniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix2fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_UniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix3fvARB");
		return;
	}
}

void GLAPIENTRY
_mesa_UniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);

	if (ctx->ShaderObjects.current_program == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix4fvARB");
		return;
	}
}

static GLboolean
_mesa_get_object_parameter (GLhandleARB obj, GLenum pname, GLvoid *params, GLboolean *integral,
	GLint *size)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_generic_intf **gen;
	struct gl2_shader_intf **sha;
	struct gl2_program_intf **pro;
	GLint *ipar = (GLint *) params;
	/*GLfloat *fpar = (GLfloat *) params;*/
	GLboolean success = GL_TRUE;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, obj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetObjectParameterivARB");
		return GL_FALSE;
	}

	gen = (struct gl2_generic_intf **) (**unk).QueryInterface (unk, UIID_GENERIC);
	if (gen == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
		return GL_FALSE;
	}

	sha = (struct gl2_shader_intf **) (**unk).QueryInterface (unk, UIID_SHADER);
	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);

	/* set default values */
	*integral = GL_TRUE;	/* indicates param type, TRUE: GLint, FALSE: GLfloat */
	*size = 1;				/* param array size */

	switch (pname)
	{
	case GL_OBJECT_TYPE_ARB:
		*ipar = (**gen).GetType (gen);
		break;
	case GL_OBJECT_SUBTYPE_ARB:
		if (sha != NULL)
			*ipar = (**sha).GetSubType (sha);
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_DELETE_STATUS_ARB:
		*ipar = (**gen).GetDeleteStatus (gen);
		break;
	case GL_OBJECT_COMPILE_STATUS_ARB:
		if (sha != NULL)
			*ipar = (**sha).GetCompileStatus (sha);
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_LINK_STATUS_ARB:
		if (pro != NULL)
			*ipar = (**pro).GetLinkStatus (pro);
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_VALIDATE_STATUS_ARB:
		if (pro != NULL)
			*ipar = (**pro).GetValidateStatus (pro);
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_INFO_LOG_LENGTH_ARB:
		{
			const GLcharARB *info = (**gen).GetInfoLog (gen);
			if (info == NULL)
				*ipar = 0;
			else
				*ipar = _mesa_strlen (info) + 1;
		}
		break;
	case GL_OBJECT_ATTACHED_OBJECTS_ARB:
		if (pro != NULL)
			*ipar = (**pro)._container.GetAttachedCount ((struct gl2_container_intf **) pro);
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
		if (pro != NULL)
			*ipar = 0;	/* TODO */
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
		if (pro != NULL)
			*ipar = 0;	/* TODO */
		else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	case GL_OBJECT_SHADER_SOURCE_LENGTH_ARB:
		if (sha != NULL) {
			const GLcharARB *src = (**sha).GetSource (sha);
			if (src == NULL)
				*ipar = 0;
			else
				*ipar = _mesa_strlen (src) + 1;
		} else {
			_mesa_error (ctx, GL_INVALID_OPERATION, "glGetObjectParameterivARB");
			success = GL_FALSE;
		}
		break;
	default:
		_mesa_error (ctx, GL_INVALID_ENUM, "glGetObjectParameterivARB");
		success = GL_FALSE;
		break;
	}

	(**gen)._unknown.Release ((struct gl2_unknown_intf **) gen);
	if (sha != NULL)
		(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
	if (pro != NULL)
		(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);

	return success;
}

void GLAPIENTRY
_mesa_GetObjectParameterfvARB (GLhandleARB obj, GLenum pname, GLfloat *params)
{
	GLboolean integral;
	GLint size, i;

	assert (sizeof (GLfloat) == sizeof (GLint));

	if (_mesa_get_object_parameter (obj, pname, (GLvoid *) params, &integral, &size) != GL_FALSE)
		if (integral != GL_FALSE)
			for (i = 0; i < size; i++)
				params[i] = (GLfloat) ((GLint *) params)[i];
}

void GLAPIENTRY
_mesa_GetObjectParameterivARB (GLhandleARB obj, GLenum pname, GLint *params)
{
	GLboolean integral;
	GLint size, i;

	assert (sizeof (GLfloat) == sizeof (GLint));

	if (_mesa_get_object_parameter (obj, pname, (GLvoid *) params, &integral, &size) != GL_FALSE)
		if (integral == GL_FALSE)
			for (i = 0; i < size; i++)
				params[i] = (GLint) ((GLfloat *) params)[i];
}

static void
_mesa_get_string (const GLcharARB *src, GLsizei maxLength, GLsizei *length, GLcharARB *str)
{
	GLsizei len;

	if (src == NULL)
		src = "";

	len = _mesa_strlen (src);
	if (len > maxLength)
	{
		len = maxLength;
		/* allocate space for null termination */
		if (len > 0)
			len--;
	}

	_mesa_memcpy (str, src, len * sizeof (GLcharARB));
	if (maxLength > 0)
		str[len] = '\0';

	if (length != NULL)
		*length = len;
}

void GLAPIENTRY
_mesa_GetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_generic_intf **gen;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, obj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetInfoLogARB");
		return;
	}

	gen = (struct gl2_generic_intf **) (**unk).QueryInterface (unk, UIID_GENERIC);
	if (gen == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetInfoLogARB");
		return;
	}

	_mesa_get_string ((**gen).GetInfoLog (gen), maxLength, length, infoLog);

	(**gen)._unknown.Release ((struct gl2_unknown_intf **) gen);
}

void GLAPIENTRY
_mesa_GetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_container_intf **con;
	GLsizei cnt, i;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, containerObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetAttachedObjectsARB");
		return;
	}

	con = (struct gl2_container_intf **) (**unk).QueryInterface (unk, UIID_CONTAINER);
	if (con == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetAttachedObjectsARB");
		return;
	}

	cnt = (**con).GetAttachedCount (con);
	if (cnt > maxCount)
		cnt = maxCount;

	for (i = 0; i < cnt; i++)
	{
		struct gl2_generic_intf **x = (**con).GetAttached (con, i);
		obj[i] = (**x).GetName (x);
		(**x)._unknown.Release ((struct gl2_unknown_intf **) x);
	}

	(**con)._generic._unknown.Release ((struct gl2_unknown_intf **) con);

	if (count != NULL)
		*count = cnt;
}

GLint GLAPIENTRY
_mesa_GetUniformLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;
	GLint loc = -1;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetUniformLocationARB");
		return -1;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformLocationARB");
		return -1;
	}

	if ((**pro).GetLinkStatus (pro) == GL_FALSE)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformLocationARB");
		(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
		return -1;
	}

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
	return loc;
}

void GLAPIENTRY
_mesa_GetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetActiveUniformARB");
		return;
	}

/*	if (index >= val (OBJECT_ACTIVE_ATTRIBUTES_ARB))
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
		(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
		return;
	}*/

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

void GLAPIENTRY
_mesa_GetUniformfvARB (GLhandleARB programObj, GLint location, GLfloat *params)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetUniformfvARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformfvARB");
		return;
	}

	if ((**pro).GetLinkStatus (pro) == GL_FALSE)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformfvARB");
		(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
		return;
	}

	/* TODO validate location (OPERATION) */

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

void GLAPIENTRY
_mesa_GetUniformivARB (GLhandleARB programObj, GLint location, GLint *params)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetUniformivARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformivARB");
		return;
	}

	if ((**pro).GetLinkStatus (pro) == GL_FALSE)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetUniformivARB");
		(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
		return;
	}

	/* TODO validate location (GL_INVALID_OPERATION) */

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

void GLAPIENTRY
_mesa_GetShaderSourceARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_shader_intf **sha;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, obj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetShaderSourceARB");
		return;
	}

	sha = (struct gl2_shader_intf **) (**unk).QueryInterface (unk, UIID_SHADER);
	if (sha == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "glGetShaderSourceARB");
		return;
	}

	_mesa_get_string ((**sha).GetSource (sha), maxLength, length, source);

	(**sha)._generic._unknown.Release ((struct gl2_unknown_intf **) sha);
}

/* GL_ARB_vertex_shader */

void GLAPIENTRY
_mesa_BindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glBindAttribLocationARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glBindAttribLocationARB");
		return;
	}

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

void GLAPIENTRY
_mesa_GetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveAttribARB");
		return;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveAttribARB");
		return;
	}

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **unk;
	struct gl2_program_intf **pro;
	GLint loc = 0;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	unk = (struct gl2_unknown_intf **) _mesa_HashLookup (ctx->Shared->GL2Objects, programObj);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	if (unk == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetAttribLocationARB");
		return 0;
	}

	pro = (struct gl2_program_intf **) (**unk).QueryInterface (unk, UIID_PROGRAM);
	if (pro == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetAttribLocationARB");
		return 0;
	}

	/* TODO */

	(**pro)._container._generic._unknown.Release ((struct gl2_unknown_intf **) pro);
	return loc;
}

void
_mesa_init_shaderobjects (GLcontext *ctx)
{
	ctx->ShaderObjects.current_program = NULL;

	_mesa_init_shaderobjects_3dlabs (ctx);
}

