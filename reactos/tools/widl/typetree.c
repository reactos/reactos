/*
 * IDL Type Tree
 *
 * Copyright 2008 Robert Shearman
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "typetree.h"
#include "header.h"

type_t *type_new_function(var_list_t *args)
{
    type_t *t = make_type(RPC_FC_FUNCTION, NULL);
    t->details.function = xmalloc(sizeof(*t->details.function));
    t->details.function->args = args;
    return t;
}

type_t *type_new_pointer(type_t *ref, attr_list_t *attrs)
{
    type_t *t = make_type(pointer_default, ref);
    t->attrs = attrs;
    return t;
}

static int compute_method_indexes(type_t *iface)
{
    int idx;
    func_t *f;

    if (iface->ref)
        idx = compute_method_indexes(iface->ref);
    else
        idx = 0;

    if (!iface->funcs)
        return idx;

    LIST_FOR_EACH_ENTRY( f, iface->funcs, func_t, entry )
        if (! is_callas(f->def->attrs))
            f->idx = idx++;

    return idx;
}

void type_interface_define(type_t *iface, type_t *inherit, statement_list_t *stmts)
{
    iface->ref = inherit;
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->funcs = gen_function_list(stmts);
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->stmts = stmts;
    iface->defined = TRUE;
    check_functions(iface);
    compute_method_indexes(iface);
}

void type_dispinterface_define(type_t *iface, var_list_t *props, func_list_t *methods)
{
    iface->ref = find_type("IDispatch", 0);
    if (!iface->ref) error_loc("IDispatch is undefined\n");
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->funcs = NULL;
    iface->details.iface->disp_props = props;
    iface->details.iface->disp_methods = methods;
    iface->stmts = NULL;
    iface->defined = TRUE;
    check_functions(iface);
    compute_method_indexes(iface);
}

void type_dispinterface_define_from_iface(type_t *dispiface, type_t *iface)
{
    type_dispinterface_define(dispiface, iface->details.iface->disp_props,
                              iface->details.iface->disp_methods);
}
