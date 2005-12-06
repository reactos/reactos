/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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
 * \file shaderobjects_3dlabs.c
 * shader objects definitions for 3dlabs compiler
 * \author Michal Krol
 */

/* Set this to 1 when we are ready to use 3dlabs' front-end */
#define USE_3DLABS_FRONTEND 0

#include "glheader.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "context.h"
#include "macros.h"
#include "hash.h"

#if USE_3DLABS_FRONTEND
#include "slang_mesa.h"
#include "Public/ShaderLang.h"
#else
#include "slang_compile.h"
#endif

struct gl2_unknown_obj
{
	GLuint reference_count;
	void (* _destructor) (struct gl2_unknown_intf **);
};

struct gl2_unknown_impl
{
	struct gl2_unknown_intf *_vftbl;
	struct gl2_unknown_obj _obj;
};

static void
_unknown_destructor (struct gl2_unknown_intf **intf)
{
}

static void
_unknown_AddRef (struct gl2_unknown_intf **intf)
{
	struct gl2_unknown_impl *impl = (struct gl2_unknown_impl *) intf;

	impl->_obj.reference_count++;
}

static void
_unknown_Release (struct gl2_unknown_intf **intf)
{
	struct gl2_unknown_impl *impl = (struct gl2_unknown_impl *) intf;

	impl->_obj.reference_count--;
	if (impl->_obj.reference_count == 0)
	{
		impl->_obj._destructor (intf);
		_mesa_free ((void *) intf);
	}
}

static struct gl2_unknown_intf **
_unknown_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_UNKNOWN)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return NULL;
}

static struct gl2_unknown_intf _unknown_vftbl = {
	_unknown_AddRef,
	_unknown_Release,
	_unknown_QueryInterface
};

static void
_unknown_constructor (struct gl2_unknown_impl *impl)
{
	impl->_vftbl = &_unknown_vftbl;
	impl->_obj.reference_count = 1;
	impl->_obj._destructor = _unknown_destructor;
}

struct gl2_unkinner_obj
{
	struct gl2_unknown_intf **unkouter;
};

struct gl2_unkinner_impl
{
	struct gl2_unknown_intf *_vftbl;
	struct gl2_unkinner_obj _obj;
};

static void
_unkinner_destructor (struct gl2_unknown_intf **intf)
{
}

static void
_unkinner_AddRef (struct gl2_unknown_intf **intf)
{
	struct gl2_unkinner_impl *impl = (struct gl2_unkinner_impl *) intf;

	(**impl->_obj.unkouter).AddRef (impl->_obj.unkouter);
}

static void
_unkinner_Release (struct gl2_unknown_intf **intf)
{
	struct gl2_unkinner_impl *impl = (struct gl2_unkinner_impl *) intf;

	(**impl->_obj.unkouter).Release (impl->_obj.unkouter);
}

static struct gl2_unknown_intf **
_unkinner_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	struct gl2_unkinner_impl *impl = (struct gl2_unkinner_impl *) intf;

	return (**impl->_obj.unkouter).QueryInterface (impl->_obj.unkouter, uiid);
}

static struct gl2_unknown_intf _unkinner_vftbl = {
	_unkinner_AddRef,
	_unkinner_Release,
	_unkinner_QueryInterface
};

static void
_unkinner_constructor (struct gl2_unkinner_impl *impl, struct gl2_unknown_intf **outer)
{
	impl->_vftbl = &_unkinner_vftbl;
	impl->_obj.unkouter = outer;
}

struct gl2_generic_obj
{
	struct gl2_unknown_obj _unknown;
	GLhandleARB name;
	GLboolean delete_status;
	GLcharARB *info_log;
};

struct gl2_generic_impl
{
	struct gl2_generic_intf *_vftbl;
	struct gl2_generic_obj _obj;
};

static void
_generic_destructor (struct gl2_unknown_intf **intf)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	_mesa_free ((void *) impl->_obj.info_log);

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	_mesa_HashRemove (ctx->Shared->GL2Objects, impl->_obj.name);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	_unknown_destructor (intf);
}

static struct gl2_unknown_intf **
_generic_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_GENERIC)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _unknown_QueryInterface (intf, uiid);
}

static void
_generic_Delete (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	if (impl->_obj.delete_status == GL_FALSE)
	{
		impl->_obj.delete_status = GL_TRUE;
		(**intf)._unknown.Release ((struct gl2_unknown_intf **) intf);
	}
}

static GLhandleARB
_generic_GetName (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.name;
}

static GLboolean
_generic_GetDeleteStatus (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.delete_status;
}

static const GLcharARB *
_generic_GetInfoLog (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.info_log;
}

static struct gl2_generic_intf _generic_vftbl = {
	{
		_unknown_AddRef,
		_unknown_Release,
		_generic_QueryInterface
	},
	_generic_Delete,
	NULL,		/* abstract GetType */
	_generic_GetName,
	_generic_GetDeleteStatus,
	_generic_GetInfoLog
};

static void
_generic_constructor (struct gl2_generic_impl *impl)
{
	GET_CURRENT_CONTEXT(ctx);

	_unknown_constructor ((struct gl2_unknown_impl *) impl);
	impl->_vftbl = &_generic_vftbl;
	impl->_obj._unknown._destructor = _generic_destructor;
	impl->_obj.delete_status = GL_FALSE;
	impl->_obj.info_log = NULL;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	impl->_obj.name = _mesa_HashFindFreeKeyBlock (ctx->Shared->GL2Objects, 1);
	_mesa_HashInsert (ctx->Shared->GL2Objects, impl->_obj.name, (void *) impl);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);
}

struct gl2_container_obj
{
	struct gl2_generic_obj _generic;
	struct gl2_generic_intf ***attached;
	GLuint attached_count;
};

struct gl2_container_impl
{
	struct gl2_container_intf *_vftbl;
	struct gl2_container_obj _obj;
};

static void
_container_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i;

	for (i = 0; i < impl->_obj.attached_count; i++)
	{
		struct gl2_generic_intf **x = impl->_obj.attached[i];
		(**x)._unknown.Release ((struct gl2_unknown_intf **) x);
	}

	_generic_destructor (intf);
}

static struct gl2_unknown_intf **
_container_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_CONTAINER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _generic_QueryInterface (intf, uiid);
}

static GLboolean
_container_Attach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i;

	for (i = 0; i < impl->_obj.attached_count; i++)
		if (impl->_obj.attached[i] == att)
		{
			_mesa_error (ctx, GL_INVALID_OPERATION, "_container_Attach");
			return GL_FALSE;
		}

	impl->_obj.attached = (struct gl2_generic_intf ***) _mesa_realloc (impl->_obj.attached,
		impl->_obj.attached_count * sizeof (*impl->_obj.attached), (impl->_obj.attached_count + 1) *
		sizeof (*impl->_obj.attached));
	if (impl->_obj.attached == NULL)
		return GL_FALSE;

	impl->_obj.attached[impl->_obj.attached_count] = att;
	impl->_obj.attached_count++;
	(**att)._unknown.AddRef ((struct gl2_unknown_intf **) att);
	return GL_TRUE;
}

static GLboolean
_container_Detach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i, j;

	for (i = 0; i < impl->_obj.attached_count; i++)
		if (impl->_obj.attached[i] == att)
		{
			for (j = i; j < impl->_obj.attached_count - 1; j++)
				impl->_obj.attached[j] = impl->_obj.attached[j + 1];
			impl->_obj.attached = (struct gl2_generic_intf ***) _mesa_realloc (impl->_obj.attached,
				impl->_obj.attached_count * sizeof (*impl->_obj.attached),
				(impl->_obj.attached_count - 1) * sizeof (*impl->_obj.attached));
			impl->_obj.attached_count--;
			(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
			return GL_TRUE;
		}

	_mesa_error (ctx, GL_INVALID_OPERATION, "_container_Detach");
	return GL_FALSE;
}

static GLsizei
_container_GetAttachedCount (struct gl2_container_intf **intf)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;

	return impl->_obj.attached_count;
}

static struct gl2_generic_intf **
_container_GetAttached (struct gl2_container_intf **intf, GLuint index)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;

	(**impl->_obj.attached[index])._unknown.AddRef (
		(struct gl2_unknown_intf **)impl->_obj.attached[index]);
	return impl->_obj.attached[index];
}

static struct gl2_container_intf _container_vftbl = {
	{
		{
			_unknown_AddRef,
			_unknown_Release,
			_container_QueryInterface
		},
		_generic_Delete,
		NULL,		/* abstract GetType */
		_generic_GetName,
		_generic_GetDeleteStatus,
		_generic_GetInfoLog
	},
	_container_Attach,
	_container_Detach,
	_container_GetAttachedCount,
	_container_GetAttached
};

static void
_container_constructor (struct gl2_container_impl *impl)
{
	_generic_constructor ((struct gl2_generic_impl *) impl);
	impl->_vftbl = &_container_vftbl;
	impl->_obj._generic._unknown._destructor = _container_destructor;
	impl->_obj.attached = NULL;
	impl->_obj.attached_count = 0;
}

struct gl2_3dlabs_shhandle_obj
{
	struct gl2_unkinner_obj _unknown;
#if USE_3DLABS_FRONTEND
	ShHandle handle;
#endif
};

struct gl2_3dlabs_shhandle_impl
{
	struct gl2_3dlabs_shhandle_intf *_vftbl;
	struct gl2_3dlabs_shhandle_obj _obj;
};

static void
_3dlabs_shhandle_destructor (struct gl2_unknown_intf **intf)
{
#if USE_3DLABS_FRONTEND
	struct gl2_3dlabs_shhandle_impl *impl = (struct gl2_3dlabs_shhandle_impl *) intf;
	ShDestruct (impl->_obj.handle);
#endif
	_unkinner_destructor (intf);
}

static GLvoid *
_3dlabs_shhandle_GetShHandle (struct gl2_3dlabs_shhandle_intf **intf)
{
#if USE_3DLABS_FRONTEND
	struct gl2_3dlabs_shhandle_impl *impl = (struct gl2_3dlabs_shhandle_impl *) intf;
	return impl->_obj.handle;
#else
	return NULL;
#endif
}

static struct gl2_3dlabs_shhandle_intf _3dlabs_shhandle_vftbl = {
	{
		_unkinner_AddRef,
		_unkinner_Release,
		_unkinner_QueryInterface
	},
	_3dlabs_shhandle_GetShHandle
};

static void
_3dlabs_shhandle_constructor (struct gl2_3dlabs_shhandle_impl *impl, struct gl2_unknown_intf **outer)
{
	_unkinner_constructor ((struct gl2_unkinner_impl *) impl, outer);
	impl->_vftbl = &_3dlabs_shhandle_vftbl;
#if USE_3DLABS_FRONTEND
	impl->_obj.handle = NULL;
#endif
}

struct gl2_shader_obj
{
	struct gl2_generic_obj _generic;
	struct gl2_3dlabs_shhandle_impl _3dlabs_shhandle;
	GLboolean compile_status;
	GLcharARB *source;
	GLint *offsets;
	GLsizei offset_count;
};

struct gl2_shader_impl
{
	struct gl2_shader_intf *_vftbl;
	struct gl2_shader_obj _obj;
};

static void
_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	_mesa_free ((void *) impl->_obj.source);
	_mesa_free ((void *) impl->_obj.offsets);
	_3dlabs_shhandle_destructor ((struct gl2_unknown_intf **) &impl->_obj._3dlabs_shhandle._vftbl);
	_generic_destructor (intf);
}

static struct gl2_unknown_intf **
_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
#if USE_3DLABS_FRONTEND
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;
#endif

	if (uiid == UIID_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
#if USE_3DLABS_FRONTEND
	if (uiid == UIID_3DLABS_SHHANDLE)
	{
		(**intf).AddRef (intf);
		return (struct gl2_unknown_intf **) &impl->_obj._3dlabs_shhandle._vftbl;
	}
#endif
	return _generic_QueryInterface (intf, uiid);
}

static GLenum
_shader_GetType (struct gl2_generic_intf **intf)
{
	return GL_SHADER_OBJECT_ARB;
}

static GLboolean
_shader_GetCompileStatus (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	return impl->_obj.compile_status;
}

static GLvoid
_shader_SetSource (struct gl2_shader_intf **intf, GLcharARB *src, GLint *off, GLsizei cnt)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	_mesa_free ((void *) impl->_obj.source);
	impl->_obj.source = src;
	_mesa_free ((void *) impl->_obj.offsets);
	impl->_obj.offsets = off;
	impl->_obj.offset_count = cnt;
}

static const GLcharARB *
_shader_GetSource (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	return impl->_obj.source;
}

static GLvoid
_shader_Compile (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;
#if USE_3DLABS_FRONTEND
	char **strings;
	TBuiltInResource res;
#else
	slang_translation_unit unit;
	slang_unit_type type;
	slang_info_log info_log;
#endif

	impl->_obj.compile_status = GL_FALSE;
	_mesa_free ((void *) impl->_obj._generic.info_log);
	impl->_obj._generic.info_log = NULL;

#if USE_3DLABS_FRONTEND
	/* 3dlabs compiler expects us to feed it with null-terminated string array,
	we've got only one big string with offsets, so we must split it; but when
	there's only one string to deal with, we pass its address directly */

	if (impl->_obj.offset_count <= 1)
		strings = &impl->_obj.source;
	else
	{
		GLsizei i, offset = 0;

		strings = (char **) _mesa_malloc (impl->_obj.offset_count * sizeof (char *));
		if (strings == NULL)
			return;

		for (i = 0; i < impl->_obj.offset_count; i++)
		{
			GLsizei size = impl->_obj.offsets[i] - offset;

			strings[i] = (char *) _mesa_malloc ((size + 1) * sizeof (char));
			if (strings[i] == NULL)
			{
				GLsizei j;

				for (j = 0; j < i; j++)
					_mesa_free (strings[j]);
				_mesa_free (strings);
				return;
			}

			_mesa_memcpy (strings[i], impl->_obj.source + offset, size * sizeof (char));
			strings[i][size] = '\0';
			offset = impl->_obj.offsets[i];
		}
	}

	/* TODO set these fields to some REAL numbers */
	res.maxLights = 8;
	res.maxClipPlanes = 6;
	res.maxTextureUnits = 2;
	res.maxTextureCoords = 2;
	res.maxVertexAttribs = 8;
	res.maxVertexUniformComponents = 64;
	res.maxVaryingFloats = 8;
	res.maxVertexTextureImageUnits = 2;
	res.maxCombinedTextureImageUnits = 2;
	res.maxTextureImageUnits = 2;
	res.maxFragmentUniformComponents = 64;
	res.maxDrawBuffers = 1;

	if (ShCompile (impl->_obj._3dlabs_shhandle._obj.handle, strings, impl->_obj.offset_count,
			EShOptFull, &res, 0))
		impl->_obj.compile_status = GL_TRUE;
	if (impl->_obj.offset_count > 1)
	{
		GLsizei i;

		for (i = 0; i < impl->_obj.offset_count; i++)
			_mesa_free (strings[i]);
		_mesa_free (strings);
	}

	impl->_obj._generic.info_log = _mesa_strdup (ShGetInfoLog (
		impl->_obj._3dlabs_shhandle._obj.handle));
#else
	if (impl->_vftbl->GetSubType (intf) == GL_FRAGMENT_SHADER)
		type = slang_unit_fragment_shader;
	else
		type = slang_unit_vertex_shader;
	slang_info_log_construct (&info_log);
	if (_slang_compile (impl->_obj.source, &unit, type, &info_log))
	{
		impl->_obj.compile_status = GL_TRUE;
	}
	if (info_log.text != NULL)
		impl->_obj._generic.info_log = _mesa_strdup (info_log.text);
	else
		impl->_obj._generic.info_log = _mesa_strdup ("");
	slang_info_log_destruct (&info_log);
#endif
}

static struct gl2_shader_intf _shader_vftbl = {
	{
		{
			_unknown_AddRef,
			_unknown_Release,
			_shader_QueryInterface
		},
		_generic_Delete,
		_shader_GetType,
		_generic_GetName,
		_generic_GetDeleteStatus,
		_generic_GetInfoLog
	},
	NULL,		/* abstract GetSubType */
	_shader_GetCompileStatus,
	_shader_SetSource,
	_shader_GetSource,
	_shader_Compile
};

static void
_shader_constructor (struct gl2_shader_impl *impl)
{
	_generic_constructor ((struct gl2_generic_impl *) impl);
	_3dlabs_shhandle_constructor (&impl->_obj._3dlabs_shhandle, (struct gl2_unknown_intf **)
		&impl->_vftbl);
	impl->_vftbl = &_shader_vftbl;
	impl->_obj._generic._unknown._destructor = _shader_destructor;
	impl->_obj.compile_status = GL_FALSE;
	impl->_obj.source = NULL;
	impl->_obj.offsets = NULL;
	impl->_obj.offset_count = 0;
}

struct gl2_program_obj
{
	struct gl2_container_obj _container;
	GLboolean link_status;
	GLboolean validate_status;
#if USE_3DLABS_FRONTEND
	ShHandle linker;
	ShHandle uniforms;
#endif
};

struct gl2_program_impl
{
	struct gl2_program_intf *_vftbl;
	struct gl2_program_obj _obj;
};

static void
_program_destructor (struct gl2_unknown_intf **intf)
{
#if USE_3DLABS_FRONTEND
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	ShDestruct (impl->_obj.linker);
	ShDestruct (impl->_obj.uniforms);
#endif
	_container_destructor (intf);
}

static struct gl2_unknown_intf **
_program_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_PROGRAM)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _container_QueryInterface (intf, uiid);
}

static GLenum
_program_GetType (struct gl2_generic_intf **intf)
{
	return GL_PROGRAM_OBJECT_ARB;
}

static GLboolean
_program_Attach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **sha;

	sha = (**att)._unknown.QueryInterface ((struct gl2_unknown_intf **) att, UIID_SHADER);
	if (sha == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "_program_Attach");
		return GL_FALSE;
	}

	(**sha).Release (sha);
	return _container_Attach (intf, att);
}

static GLboolean
_program_GetLinkStatus (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	return impl->_obj.link_status;
}

static GLboolean
_program_GetValidateStatus (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	return impl->_obj.validate_status;
}

static GLvoid
_program_Link (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;
#if USE_3DLABS_FRONTEND
	ShHandle *handles;
	GLuint i;
#endif

	impl->_obj.link_status = GL_FALSE;
	_mesa_free ((void *) impl->_obj._container._generic.info_log);
	impl->_obj._container._generic.info_log = NULL;

#if USE_3DLABS_FRONTEND
	handles = (ShHandle *) _mesa_malloc (impl->_obj._container.attached_count * sizeof (ShHandle));
	if (handles == NULL)
		return;

	for (i = 0; i < impl->_obj._container.attached_count; i++)
	{
		struct gl2_generic_intf **gen = impl->_obj._container.attached[i];
		struct gl2_3dlabs_shhandle_intf **sh;

		sh = (struct gl2_3dlabs_shhandle_intf **) (**gen)._unknown.QueryInterface (
			(struct gl2_unknown_intf **) gen, UIID_3DLABS_SHHANDLE);
		if (sh != NULL)
		{
			handles[i] = (**sh).GetShHandle (sh);
			(**sh)._unknown.Release ((struct gl2_unknown_intf **) sh);
		}
		else
		{
			_mesa_free (handles);
			return;
		}
	}

	if (ShLink (impl->_obj.linker, handles, impl->_obj._container.attached_count,
			impl->_obj.uniforms, NULL, NULL))
		impl->_obj.link_status = GL_TRUE;

	impl->_obj._container._generic.info_log = _mesa_strdup (ShGetInfoLog (impl->_obj.linker));
#endif
}

static GLvoid
_program_Validate (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	impl->_obj.validate_status = GL_FALSE;
	_mesa_free ((void *) impl->_obj._container._generic.info_log);
	impl->_obj._container._generic.info_log = NULL;

	/* TODO validate */
}

static struct gl2_program_intf _program_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_program_QueryInterface
			},
			_generic_Delete,
			_program_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog
		},
		_program_Attach,
		_container_Detach,
		_container_GetAttachedCount,
		_container_GetAttached
	},
	_program_GetLinkStatus,
	_program_GetValidateStatus,
	_program_Link,
	_program_Validate
};

static void
_program_constructor (struct gl2_program_impl *impl)
{
	_container_constructor ((struct gl2_container_impl *) impl);
	impl->_vftbl = &_program_vftbl;
	impl->_obj._container._generic._unknown._destructor = _program_destructor;
	impl->_obj.link_status = GL_FALSE;
	impl->_obj.validate_status = GL_FALSE;
#if USE_3DLABS_FRONTEND
	impl->_obj.linker = ShConstructLinker (EShExVertexFragment, 0);
	impl->_obj.uniforms = ShConstructUniformMap ();
#endif
}

struct gl2_fragment_shader_obj
{
	struct gl2_shader_obj _shader;
};

struct gl2_fragment_shader_impl
{
	struct gl2_fragment_shader_intf *_vftbl;
	struct gl2_fragment_shader_obj _obj;
};

static void
_fragment_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_fragment_shader_impl *impl = (struct gl2_fragment_shader_impl *) intf;

	(void) impl;
	/* TODO free fragment shader data */

	_shader_destructor (intf);
}

static struct gl2_unknown_intf **
_fragment_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_FRAGMENT_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _shader_QueryInterface (intf, uiid);
}

static GLenum
_fragment_shader_GetSubType (struct gl2_shader_intf **intf)
{
	return GL_FRAGMENT_SHADER_ARB;
}

static struct gl2_fragment_shader_intf _fragment_shader_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_fragment_shader_QueryInterface
			},
			_generic_Delete,
			_shader_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog
		},
		_fragment_shader_GetSubType,
		_shader_GetCompileStatus,
		_shader_SetSource,
		_shader_GetSource,
		_shader_Compile
	}
};

static void
_fragment_shader_constructor (struct gl2_fragment_shader_impl *impl)
{
	_shader_constructor ((struct gl2_shader_impl *) impl);
	impl->_vftbl = &_fragment_shader_vftbl;
	impl->_obj._shader._generic._unknown._destructor = _fragment_shader_destructor;
#if USE_3DLABS_FRONTEND
	impl->_obj._shader._3dlabs_shhandle._obj.handle = ShConstructCompiler (EShLangFragment, 0);
#endif
}

struct gl2_vertex_shader_obj
{
	struct gl2_shader_obj _shader;
};

struct gl2_vertex_shader_impl
{
	struct gl2_vertex_shader_intf *_vftbl;
	struct gl2_vertex_shader_obj _obj;
};

static void
_vertex_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_vertex_shader_impl *impl = (struct gl2_vertex_shader_impl *) intf;

	(void) impl;
	/* TODO free vertex shader data */

	_shader_destructor (intf);
}

static struct gl2_unknown_intf **
_vertex_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_VERTEX_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _shader_QueryInterface (intf, uiid);
}

static GLenum
_vertex_shader_GetSubType (struct gl2_shader_intf **intf)
{
	return GL_VERTEX_SHADER_ARB;
}

static struct gl2_vertex_shader_intf _vertex_shader_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_vertex_shader_QueryInterface
			},
			_generic_Delete,
			_shader_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog
		},
		_vertex_shader_GetSubType,
		_shader_GetCompileStatus,
		_shader_SetSource,
		_shader_GetSource,
		_shader_Compile
	}
};

static void
_vertex_shader_constructor (struct gl2_vertex_shader_impl *impl)
{
	_shader_constructor ((struct gl2_shader_impl *) impl);
	impl->_vftbl = &_vertex_shader_vftbl;
	impl->_obj._shader._generic._unknown._destructor = _vertex_shader_destructor;
#if USE_3DLABS_FRONTEND
	impl->_obj._shader._3dlabs_shhandle._obj.handle = ShConstructCompiler (EShLangVertex, 0);
#endif
}

GLhandleARB
_mesa_3dlabs_create_shader_object (GLenum shaderType)
{
	switch (shaderType)
	{
	case GL_FRAGMENT_SHADER_ARB:
		{
			struct gl2_fragment_shader_impl *x = (struct gl2_fragment_shader_impl *)
				_mesa_malloc (sizeof (struct gl2_fragment_shader_impl));

			if (x != NULL)
			{
				_fragment_shader_constructor (x);
				return x->_obj._shader._generic.name;
			}
		}
		break;
	case GL_VERTEX_SHADER_ARB:
		{
			struct gl2_vertex_shader_impl *x = (struct gl2_vertex_shader_impl *)
				_mesa_malloc (sizeof (struct gl2_vertex_shader_impl));

			if (x != NULL)
			{
				_vertex_shader_constructor (x);
				return x->_obj._shader._generic.name;
			}
		}
		break;
	}

	return 0;
}

GLhandleARB
_mesa_3dlabs_create_program_object (void)
{
	struct gl2_program_impl *x = (struct gl2_program_impl *) 
		_mesa_malloc (sizeof (struct gl2_program_impl));

	if (x != NULL)
	{
		_program_constructor (x);
		return x->_obj._container._generic.name;
	}

	return 0;
}

void
_mesa_init_shaderobjects_3dlabs (GLcontext *ctx)
{
#if USE_3DLABS_FRONTEND
	_glslang_3dlabs_InitProcess ();
	_glslang_3dlabs_ShInitialize ();
#endif
}

