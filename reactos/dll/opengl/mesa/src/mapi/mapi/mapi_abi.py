#!/usr/bin/env python

# Mesa 3-D graphics library
# Version:  7.9
#
# Copyright (C) 2010 LunarG Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# Authors:
#    Chia-I Wu <olv@lunarg.com>

import sys
# make it possible to import glapi
import os
GLAPI = "./%s/../glapi/gen" % (os.path.dirname(sys.argv[0]))
sys.path.append(GLAPI)

import re
from optparse import OptionParser

# number of dynamic entries
ABI_NUM_DYNAMIC_ENTRIES = 256

class ABIEntry(object):
    """Represent an ABI entry."""

    _match_c_param = re.compile(
            '^(?P<type>[\w\s*]+?)(?P<name>\w+)(\[(?P<array>\d+)\])?$')

    def __init__(self, cols, attrs):
        self._parse(cols)

        self.slot = attrs['slot']
        self.hidden = attrs['hidden']
        self.alias = attrs['alias']
        self.handcode = attrs['handcode']

    def c_prototype(self):
        return '%s %s(%s)' % (self.c_return(), self.name, self.c_params())

    def c_return(self):
        ret = self.ret
        if not ret:
            ret = 'void'

        return ret

    def c_params(self):
        """Return the parameter list used in the entry prototype."""
        c_params = []
        for t, n, a in self.params:
            sep = '' if t.endswith('*') else ' '
            arr = '[%d]' % a if a else ''
            c_params.append(t + sep + n + arr)
        if not c_params:
            c_params.append('void')

        return ", ".join(c_params)

    def c_args(self):
        """Return the argument list used in the entry invocation."""
        c_args = []
        for t, n, a in self.params:
            c_args.append(n)

        return ", ".join(c_args)

    def _parse(self, cols):
        ret = cols.pop(0)
        if ret == 'void':
            ret = None

        name = cols.pop(0)

        params = []
        if not cols:
            raise Exception(cols)
        elif len(cols) == 1 and cols[0] == 'void':
            pass
        else:
            for val in cols:
                params.append(self._parse_param(val))

        self.ret = ret
        self.name = name
        self.params = params

    def _parse_param(self, c_param):
        m = self._match_c_param.match(c_param)
        if not m:
            raise Exception('unrecognized param ' + c_param)

        c_type = m.group('type').strip()
        c_name = m.group('name')
        c_array = m.group('array')
        c_array = int(c_array) if c_array else 0

        return (c_type, c_name, c_array)

    def __str__(self):
        return self.c_prototype()

    def __cmp__(self, other):
        # compare slot, alias, and then name
        res = cmp(self.slot, other.slot)
        if not res:
            if not self.alias:
                res = -1
            elif not other.alias:
                res = 1

            if not res:
                res = cmp(self.name, other.name)

        return res

def abi_parse_xml(xml):
    """Parse a GLAPI XML file for ABI entries."""
    import gl_XML, glX_XML

    api = gl_XML.parse_GL_API(xml, glX_XML.glx_item_factory())

    entry_dict = {}
    for func in api.functionIterateByOffset():
        # make sure func.name appear first
        entry_points = func.entry_points[:]
        entry_points.remove(func.name)
        entry_points.insert(0, func.name)

        for name in entry_points:
            attrs = {
                    'slot': func.offset,
                    'hidden': not func.is_static_entry_point(name),
                    'alias': None if name == func.name else func.name,
                    'handcode': bool(func.has_different_protocol(name)),
            }

            # post-process attrs
            if attrs['alias']:
                try:
                    alias = entry_dict[attrs['alias']]
                except KeyError:
                    raise Exception('failed to alias %s' % attrs['alias'])
                if alias.alias:
                    raise Exception('recursive alias %s' % ent.name)
                attrs['alias'] = alias
            if attrs['handcode']:
                attrs['handcode'] = func.static_glx_name(name)
            else:
                attrs['handcode'] = None

            if entry_dict.has_key(name):
                raise Exception('%s is duplicated' % (name))

            cols = []
            cols.append(func.return_type)
            cols.append(name)
            params = func.get_parameter_string(name)
            cols.extend([p.strip() for p in params.split(',')])

            ent = ABIEntry(cols, attrs)
            entry_dict[ent.name] = ent

    entries = entry_dict.values()
    entries.sort()

    return entries

def abi_parse_line(line):
    cols = [col.strip() for col in line.split(',')]

    attrs = {
            'slot': -1,
            'hidden': False,
            'alias': None,
            'handcode': None,
    }

    # extract attributes from the first column
    vals = cols[0].split(':')
    while len(vals) > 1:
        val = vals.pop(0)
        if val.startswith('slot='):
            attrs['slot'] = int(val[5:])
        elif val == 'hidden':
            attrs['hidden'] = True
        elif val.startswith('alias='):
            attrs['alias'] = val[6:]
        elif val.startswith('handcode='):
            attrs['handcode'] = val[9:]
        elif not val:
            pass
        else:
            raise Exception('unknown attribute %s' % val)
    cols[0] = vals[0]

    return (attrs, cols)

def abi_parse(filename):
    """Parse a CSV file for ABI entries."""
    fp = open(filename) if filename != '-' else sys.stdin
    lines = [line.strip() for line in fp.readlines()
            if not line.startswith('#') and line.strip()]

    entry_dict = {}
    next_slot = 0
    for line in lines:
        attrs, cols = abi_parse_line(line)

        # post-process attributes
        if attrs['alias']:
            try:
                alias = entry_dict[attrs['alias']]
            except KeyError:
                raise Exception('failed to alias %s' % attrs['alias'])
            if alias.alias:
                raise Exception('recursive alias %s' % ent.name)
            slot = alias.slot
            attrs['alias'] = alias
        else:
            slot = next_slot
            next_slot += 1

        if attrs['slot'] < 0:
            attrs['slot'] = slot
        elif attrs['slot'] != slot:
            raise Exception('invalid slot in %s' % (line))

        ent = ABIEntry(cols, attrs)
        if entry_dict.has_key(ent.name):
            raise Exception('%s is duplicated' % (ent.name))
        entry_dict[ent.name] = ent

    entries = entry_dict.values()
    entries.sort()

    return entries

def abi_sanity_check(entries):
    if not entries:
        return

    all_names = []
    last_slot = entries[-1].slot
    i = 0
    for slot in xrange(last_slot + 1):
        if entries[i].slot != slot:
            raise Exception('entries are not ordered by slots')
        if entries[i].alias:
            raise Exception('first entry of slot %d aliases %s'
                    % (slot, entries[i].alias.name))
        handcode = None
        while i < len(entries) and entries[i].slot == slot:
            ent = entries[i]
            if not handcode and ent.handcode:
                handcode = ent.handcode
            elif ent.handcode != handcode:
                raise Exception('two aliases with handcode %s != %s',
                        ent.handcode, handcode)

            if ent.name in all_names:
                raise Exception('%s is duplicated' % (ent.name))
            if ent.alias and ent.alias.name not in all_names:
                raise Exception('failed to alias %s' % (ent.alias.name))
            all_names.append(ent.name)
            i += 1
    if i < len(entries):
        raise Exception('there are %d invalid entries' % (len(entries) - 1))

class ABIPrinter(object):
    """MAPI Printer"""

    def __init__(self, entries):
        self.entries = entries

        # sort entries by their names
        self.entries_sorted_by_names = self.entries[:]
        self.entries_sorted_by_names.sort(lambda x, y: cmp(x.name, y.name))

        self.indent = ' ' * 3
        self.noop_warn = 'noop_warn'
        self.noop_generic = 'noop_generic'
        self.current_get = 'entry_current_get'

        self.api_defines = []
        self.api_headers = ['"KHR/khrplatform.h"']
        self.api_call = 'KHRONOS_APICALL'
        self.api_entry = 'KHRONOS_APIENTRY'
        self.api_attrs = 'KHRONOS_APIATTRIBUTES'

        self.c_header = ''

        self.lib_need_table_size = True
        self.lib_need_noop_array = True
        self.lib_need_stubs = True
        self.lib_need_all_entries = True
        self.lib_need_non_hidden_entries = False

    def c_notice(self):
        return '/* This file is automatically generated by mapi_abi.py.  Do not modify. */'

    def c_public_includes(self):
        """Return includes of the client API headers."""
        defines = ['#define ' + d for d in self.api_defines]
        includes = ['#include ' + h for h in self.api_headers]
        return "\n".join(defines + includes)

    def need_entry_point(self, ent):
        """Return True if an entry point is needed for the entry."""
        # non-handcode hidden aliases may share the entry they alias
        use_alias = (ent.hidden and ent.alias and not ent.handcode)
        return not use_alias

    def c_public_declarations(self, prefix):
        """Return the declarations of public entry points."""
        decls = []
        for ent in self.entries:
            if not self.need_entry_point(ent):
                continue
            export = self.api_call if not ent.hidden else ''
            decls.append(self._c_decl(ent, prefix, True, export) + ';')

        return "\n".join(decls)

    def c_mapi_table(self):
        """Return defines of the dispatch table size."""
        num_static_entries = self.entries[-1].slot + 1
        return ('#define MAPI_TABLE_NUM_STATIC %d\n' + \
                '#define MAPI_TABLE_NUM_DYNAMIC %d') % (
                        num_static_entries, ABI_NUM_DYNAMIC_ENTRIES)

    def c_mapi_table_initializer(self, prefix):
        """Return the array initializer for mapi_table_fill."""
        entries = [self._c_function(ent, prefix)
                for ent in self.entries if not ent.alias]
        pre = self.indent + '(mapi_proc) '
        return pre + (',\n' + pre).join(entries)

    def c_mapi_table_spec(self):
        """Return the spec for mapi_init."""
        specv1 = []
        line = '"1'
        for ent in self.entries:
            if not ent.alias:
                line += '\\0"\n'
                specv1.append(line)
                line = '"'
            line += '%s\\0' % ent.name
        line += '";'
        specv1.append(line)

        return self.indent + self.indent.join(specv1)

    def _c_function(self, ent, prefix, mangle=False, stringify=False):
        """Return the function name of an entry."""
        formats = {
                True: { True: '%s_STR(%s)', False: '%s(%s)' },
                False: { True: '"%s%s"', False: '%s%s' },
        }
        fmt = formats[prefix.isupper()][stringify]
        name = ent.name
        if mangle and ent.hidden:
            name = '_dispatch_stub_' + str(ent.slot)
        return fmt % (prefix, name)

    def _c_function_call(self, ent, prefix):
        """Return the function name used for calling."""
        if ent.handcode:
            # _c_function does not handle this case
            formats = { True: '%s(%s)', False: '%s%s' }
            fmt = formats[prefix.isupper()]
            name = fmt % (prefix, ent.handcode)
        elif self.need_entry_point(ent):
            name = self._c_function(ent, prefix, True)
        else:
            name = self._c_function(ent.alias, prefix, True)
        return name

    def _c_decl(self, ent, prefix, mangle=False, export=''):
        """Return the C declaration for the entry."""
        decl = '%s %s %s(%s)' % (ent.c_return(), self.api_entry,
                self._c_function(ent, prefix, mangle), ent.c_params())
        if export:
            decl = export + ' ' + decl
        if self.api_attrs:
            decl += ' ' + self.api_attrs

        return decl

    def _c_cast(self, ent):
        """Return the C cast for the entry."""
        cast = '%s (%s *)(%s)' % (
                ent.c_return(), self.api_entry, ent.c_params())

        return cast

    def c_private_declarations(self, prefix):
        """Return the declarations of private functions."""
        decls = [self._c_decl(ent, prefix) + ';'
                for ent in self.entries if not ent.alias]

        return "\n".join(decls)

    def c_public_dispatches(self, prefix, no_hidden):
        """Return the public dispatch functions."""
        dispatches = []
        for ent in self.entries:
            if ent.hidden and no_hidden:
                continue

            if not self.need_entry_point(ent):
                continue

            export = self.api_call if not ent.hidden else ''

            proto = self._c_decl(ent, prefix, True, export)
            cast = self._c_cast(ent)

            ret = ''
            if ent.ret:
                ret = 'return '
            stmt1 = self.indent
            stmt1 += 'const struct mapi_table *_tbl = %s();' % (
                    self.current_get)
            stmt2 = self.indent
            stmt2 += 'mapi_func _func = ((const mapi_func *) _tbl)[%d];' % (
                    ent.slot)
            stmt3 = self.indent
            stmt3 += '%s((%s) _func)(%s);' % (ret, cast, ent.c_args())

            disp = '%s\n{\n%s\n%s\n%s\n}' % (proto, stmt1, stmt2, stmt3)

            if ent.handcode:
                disp = '#if 0\n' + disp + '\n#endif'

            dispatches.append(disp)

        return '\n\n'.join(dispatches)

    def c_public_initializer(self, prefix):
        """Return the initializer for public dispatch functions."""
        names = []
        for ent in self.entries:
            if ent.alias:
                continue

            name = '%s(mapi_func) %s' % (self.indent,
                    self._c_function_call(ent, prefix))
            names.append(name)

        return ',\n'.join(names)

    def c_stub_string_pool(self):
        """Return the string pool for use by stubs."""
        # sort entries by their names
        sorted_entries = self.entries[:]
        sorted_entries.sort(lambda x, y: cmp(x.name, y.name))

        pool = []
        offsets = {}
        count = 0
        for ent in sorted_entries:
            offsets[ent] = count
            pool.append('%s' % (ent.name))
            count += len(ent.name) + 1

        pool_str =  self.indent + '"' + \
                ('\\0"\n' + self.indent + '"').join(pool) + '";'
        return (pool_str, offsets)

    def c_stub_initializer(self, prefix, pool_offsets):
        """Return the initializer for struct mapi_stub array."""
        stubs = []
        for ent in self.entries_sorted_by_names:
            stubs.append('%s{ (void *) %d, %d, NULL }' % (
                self.indent, pool_offsets[ent], ent.slot))

        return ',\n'.join(stubs)

    def c_noop_functions(self, prefix, warn_prefix):
        """Return the noop functions."""
        noops = []
        for ent in self.entries:
            if ent.alias:
                continue

            proto = self._c_decl(ent, prefix, False, 'static')

            stmt1 = self.indent;
            space = ''
            for t, n, a in ent.params:
                stmt1 += "%s(void) %s;" % (space, n)
                space = ' '

            if ent.params:
                stmt1 += '\n';

            stmt1 += self.indent + '%s(%s);' % (self.noop_warn,
                    self._c_function(ent, warn_prefix, False, True))

            if ent.ret:
                stmt2 = self.indent + 'return (%s) 0;' % (ent.ret)
                noop = '%s\n{\n%s\n%s\n}' % (proto, stmt1, stmt2)
            else:
                noop = '%s\n{\n%s\n}' % (proto, stmt1)

            noops.append(noop)

        return '\n\n'.join(noops)

    def c_noop_initializer(self, prefix, use_generic):
        """Return an initializer for the noop dispatch table."""
        entries = [self._c_function(ent, prefix)
                for ent in self.entries if not ent.alias]
        if use_generic:
            entries = [self.noop_generic] * len(entries)

        entries.extend([self.noop_generic] * ABI_NUM_DYNAMIC_ENTRIES)

        pre = self.indent + '(mapi_func) '
        return pre + (',\n' + pre).join(entries)

    def c_asm_gcc(self, prefix, no_hidden):
        asm = []

        for ent in self.entries:
            if ent.hidden and no_hidden:
                continue

            if not self.need_entry_point(ent):
                continue

            name = self._c_function(ent, prefix, True, True)

            if ent.handcode:
                asm.append('#if 0')

            if ent.hidden:
                asm.append('".hidden "%s"\\n"' % (name))

            if ent.alias and not (ent.alias.hidden and no_hidden):
                asm.append('".globl "%s"\\n"' % (name))
                asm.append('".set "%s", "%s"\\n"' % (name,
                    self._c_function(ent.alias, prefix, True, True)))
            else:
                asm.append('STUB_ASM_ENTRY(%s)"\\n"' % (name))
                asm.append('"\\t"STUB_ASM_CODE("%d")"\\n"' % (ent.slot))

            if ent.handcode:
                asm.append('#endif')
            asm.append('')

        return "\n".join(asm)

    def output_for_lib(self):
        print self.c_notice()

        if self.c_header:
            print
            print self.c_header

        print
        print '#ifdef MAPI_TMP_DEFINES'
        print self.c_public_includes()
        print
        print self.c_public_declarations(self.prefix_lib)
        print '#undef MAPI_TMP_DEFINES'
        print '#endif /* MAPI_TMP_DEFINES */'

        if self.lib_need_table_size:
            print
            print '#ifdef MAPI_TMP_TABLE'
            print self.c_mapi_table()
            print '#undef MAPI_TMP_TABLE'
            print '#endif /* MAPI_TMP_TABLE */'

        if self.lib_need_noop_array:
            print
            print '#ifdef MAPI_TMP_NOOP_ARRAY'
            print '#ifdef DEBUG'
            print
            print self.c_noop_functions(self.prefix_noop, self.prefix_warn)
            print
            print 'const mapi_func table_%s_array[] = {' % (self.prefix_noop)
            print self.c_noop_initializer(self.prefix_noop, False)
            print '};'
            print
            print '#else /* DEBUG */'
            print
            print 'const mapi_func table_%s_array[] = {' % (self.prefix_noop)
            print self.c_noop_initializer(self.prefix_noop, True)
            print '};'
            print
            print '#endif /* DEBUG */'
            print '#undef MAPI_TMP_NOOP_ARRAY'
            print '#endif /* MAPI_TMP_NOOP_ARRAY */'

        if self.lib_need_stubs:
            pool, pool_offsets = self.c_stub_string_pool()
            print
            print '#ifdef MAPI_TMP_PUBLIC_STUBS'
            print 'static const char public_string_pool[] ='
            print pool
            print
            print 'static const struct mapi_stub public_stubs[] = {'
            print self.c_stub_initializer(self.prefix_lib, pool_offsets)
            print '};'
            print '#undef MAPI_TMP_PUBLIC_STUBS'
            print '#endif /* MAPI_TMP_PUBLIC_STUBS */'

        if self.lib_need_all_entries:
            print
            print '#ifdef MAPI_TMP_PUBLIC_ENTRIES'
            print self.c_public_dispatches(self.prefix_lib, False)
            print
            print 'static const mapi_func public_entries[] = {'
            print self.c_public_initializer(self.prefix_lib)
            print '};'
            print '#undef MAPI_TMP_PUBLIC_ENTRIES'
            print '#endif /* MAPI_TMP_PUBLIC_ENTRIES */'

            print
            print '#ifdef MAPI_TMP_STUB_ASM_GCC'
            print '__asm__('
            print self.c_asm_gcc(self.prefix_lib, False)
            print ');'
            print '#undef MAPI_TMP_STUB_ASM_GCC'
            print '#endif /* MAPI_TMP_STUB_ASM_GCC */'

        if self.lib_need_non_hidden_entries:
            all_hidden = True
            for ent in self.entries:
                if not ent.hidden:
                    all_hidden = False
                    break
            if not all_hidden:
                print
                print '#ifdef MAPI_TMP_PUBLIC_ENTRIES_NO_HIDDEN'
                print self.c_public_dispatches(self.prefix_lib, True)
                print
                print '/* does not need public_entries */'
                print '#undef MAPI_TMP_PUBLIC_ENTRIES_NO_HIDDEN'
                print '#endif /* MAPI_TMP_PUBLIC_ENTRIES_NO_HIDDEN */'

                print
                print '#ifdef MAPI_TMP_STUB_ASM_GCC_NO_HIDDEN'
                print '__asm__('
                print self.c_asm_gcc(self.prefix_lib, True)
                print ');'
                print '#undef MAPI_TMP_STUB_ASM_GCC_NO_HIDDEN'
                print '#endif /* MAPI_TMP_STUB_ASM_GCC_NO_HIDDEN */'

    def output_for_app(self):
        print self.c_notice()
        print
        print self.c_private_declarations(self.prefix_app)
        print
        print '#ifdef API_TMP_DEFINE_SPEC'
        print
        print 'static const char %s_spec[] =' % (self.prefix_app)
        print self.c_mapi_table_spec()
        print
        print 'static const mapi_proc %s_procs[] = {' % (self.prefix_app)
        print self.c_mapi_table_initializer(self.prefix_app)
        print '};'
        print
        print '#endif /* API_TMP_DEFINE_SPEC */'

class GLAPIPrinter(ABIPrinter):
    """OpenGL API Printer"""

    def __init__(self, entries, api=None):
        api_entries = self._get_api_entries(entries, api)
        super(GLAPIPrinter, self).__init__(api_entries)

        self.api_defines = ['GL_GLEXT_PROTOTYPES']
        self.api_headers = ['"GL/gl.h"', '"GL/glext.h"']
        self.api_call = 'GLAPI'
        self.api_entry = 'APIENTRY'
        self.api_attrs = ''

        self.lib_need_table_size = False
        self.lib_need_noop_array = False
        self.lib_need_stubs = False
        self.lib_need_all_entries = False
        self.lib_need_non_hidden_entries = True

        self.prefix_lib = 'GLAPI_PREFIX'
        self.prefix_app = '_mesa_'
        self.prefix_noop = 'noop'
        self.prefix_warn = self.prefix_lib

        self.c_header = self._get_c_header()

    def _get_api_entries(self, entries, api):
        """Override the entry attributes according to API."""
        import copy

        # no override
        if api is None:
            return entries

        api_entries = {}
        for ent in entries:
            ent = copy.copy(ent)

            # override 'hidden' and 'handcode'
            ent.hidden = ent.name not in api
            ent.handcode = False
            if ent.alias:
                ent.alias = api_entries[ent.alias.name]

            api_entries[ent.name] = ent

        # sanity check
        missed = [name for name in api if name not in api_entries]
        if missed:
            raise Exception('%s is missing' % str(missed))

        entries = api_entries.values()
        entries.sort()

        return entries

    def _get_c_header(self):
        header = """#ifndef _GLAPI_TMP_H_
#define _GLAPI_TMP_H_
#ifdef USE_MGL_NAMESPACE
#define GLAPI_PREFIX(func)  mgl##func
#define GLAPI_PREFIX_STR(func)  "mgl"#func
#else
#define GLAPI_PREFIX(func)  gl##func
#define GLAPI_PREFIX_STR(func)  "gl"#func
#endif /* USE_MGL_NAMESPACE */

typedef int GLfixed;
typedef int GLclampx;
#endif /* _GLAPI_TMP_H_ */"""

        return header

class ES1APIPrinter(GLAPIPrinter):
    """OpenGL ES 1.x API Printer"""

    def __init__(self, entries):
        from gles_api import es1_api

        super(ES1APIPrinter, self).__init__(entries, es1_api)
        self.prefix_lib = 'gl'
        self.prefix_warn = 'gl'

    def _get_c_header(self):
        header = """#ifndef _GLAPI_TMP_H_
#define _GLAPI_TMP_H_
typedef int GLfixed;
typedef int GLclampx;
#endif /* _GLAPI_TMP_H_ */"""

        return header

class ES2APIPrinter(GLAPIPrinter):
    """OpenGL ES 2.x API Printer"""

    def __init__(self, entries):
        from gles_api import es2_api

        super(ES2APIPrinter, self).__init__(entries, es2_api)
        self.prefix_lib = 'gl'
        self.prefix_warn = 'gl'

    def _get_c_header(self):
        header = """#ifndef _GLAPI_TMP_H_
#define _GLAPI_TMP_H_
typedef int GLfixed;
typedef int GLclampx;
#endif /* _GLAPI_TMP_H_ */"""

        return header

class SharedGLAPIPrinter(GLAPIPrinter):
    """Shared GLAPI API Printer"""

    def __init__(self, entries):
        super(SharedGLAPIPrinter, self).__init__(entries, [])

        self.lib_need_table_size = True
        self.lib_need_noop_array = True
        self.lib_need_stubs = True
        self.lib_need_all_entries = True
        self.lib_need_non_hidden_entries = False

        self.prefix_lib = 'shared'
        self.prefix_warn = 'gl'

    def _get_c_header(self):
        header = """#ifndef _GLAPI_TMP_H_
#define _GLAPI_TMP_H_
typedef int GLfixed;
typedef int GLclampx;
#endif /* _GLAPI_TMP_H_ */"""

        return header

class VGAPIPrinter(ABIPrinter):
    """OpenVG API Printer"""

    def __init__(self, entries):
        super(VGAPIPrinter, self).__init__(entries)

        self.api_defines = ['VG_VGEXT_PROTOTYPES']
        self.api_headers = ['"VG/openvg.h"', '"VG/vgext.h"']
        self.api_call = 'VG_API_CALL'
        self.api_entry = 'VG_API_ENTRY'
        self.api_attrs = 'VG_API_EXIT'

        self.prefix_lib = 'vg'
        self.prefix_app = 'vega'
        self.prefix_noop = 'noop'
        self.prefix_warn = 'vg'

def parse_args():
    printers = ['vgapi', 'glapi', 'es1api', 'es2api', 'shared-glapi']
    modes = ['lib', 'app']

    parser = OptionParser(usage='usage: %prog [options] <filename>')
    parser.add_option('-p', '--printer', dest='printer',
            help='printer to use: %s' % (", ".join(printers)))
    parser.add_option('-m', '--mode', dest='mode',
            help='target user: %s' % (", ".join(modes)))

    options, args = parser.parse_args()
    if not args or options.printer not in printers or \
            options.mode not in modes:
        parser.print_help()
        sys.exit(1)

    return (args[0], options)

def main():
    printers = {
        'vgapi': VGAPIPrinter,
        'glapi': GLAPIPrinter,
        'es1api': ES1APIPrinter,
        'es2api': ES2APIPrinter,
        'shared-glapi': SharedGLAPIPrinter,
    }

    filename, options = parse_args()

    if filename.endswith('.xml'):
        entries = abi_parse_xml(filename)
    else:
        entries = abi_parse(filename)
    abi_sanity_check(entries)

    printer = printers[options.printer](entries)
    if options.mode == 'lib':
        printer.output_for_lib()
    else:
        printer.output_for_app()

if __name__ == '__main__':
    main()
