/*
 * Copyright 2002 Ove Kaaven
 * Copyright 2006-2008 Robert Shearman
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

#include "widl.h"
#include "typetree.h"

#include "parser.tab.h"

attr_t *attr_int( struct location where, enum attr_type attr_type, unsigned int val )
{
    attr_t *a = xmalloc( sizeof(attr_t) );
    a->where = where;
    a->type = attr_type;
    a->u.ival = val;
    return a;
}

attr_t *attr_ptr( struct location where, enum attr_type attr_type, void *val )
{
    attr_t *a = xmalloc( sizeof(attr_t) );
    a->where = where;
    a->type = attr_type;
    a->u.pval = val;
    return a;
}

int is_attr( const attr_list_t *list, enum attr_type attr_type )
{
    const attr_t *attr;
    if (!list) return 0;
    LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == attr_type) return 1;
    return 0;
}

int is_ptrchain_attr( const var_t *var, enum attr_type attr_type )
{
    type_t *type = var->declspec.type;
    if (is_attr( var->attrs, attr_type )) return 1;
    for (;;)
    {
        if (is_attr( type->attrs, attr_type )) return 1;
        else if (type_is_alias( type )) type = type_alias_get_aliasee_type( type );
        else if (type_is_ptr( type )) type = type_pointer_get_ref_type( type );
        else return 0;
    }
}

int is_aliaschain_attr( const type_t *type, enum attr_type attr_type )
{
    const type_t *t = type;
    for (;;)
    {
        if (is_attr( t->attrs, attr_type )) return 1;
        else if (type_is_alias( t )) t = type_alias_get_aliasee_type( t );
        else return 0;
    }
}

unsigned int get_attrv( const attr_list_t *list, enum attr_type attr_type )
{
    const attr_t *attr;
    if (!list) return 0;
    LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == attr_type) return attr->u.ival;
    return 0;
}

void *get_attrp( const attr_list_t *list, enum attr_type attr_type )
{
    const attr_t *attr;
    if (!list) return NULL;
    LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == attr_type) return attr->u.pval;
    return NULL;
}

void *get_aliaschain_attrp( const type_t *type, enum attr_type attr_type )
{
    for (;;)
    {
        if (is_attr( type->attrs, attr_type )) return get_attrp( type->attrs, attr_type );
        if (!type_is_alias( type )) return NULL;
        type = type_alias_get_aliasee_type( type );
    }
}

struct allowed_attr
{
    unsigned int dce_compatible : 1;
    unsigned int acf : 1;
    unsigned int multiple : 1;

    unsigned int on_interface : 1;
    unsigned int on_function : 1;
    unsigned int on_arg : 1;
    unsigned int on_type : 1;
    unsigned int on_enum : 1;
    unsigned int on_enum_member : 1;
    unsigned int on_struct : 2;
    unsigned int on_union : 1;
    unsigned int on_field : 1;
    unsigned int on_library : 1;
    unsigned int on_dispinterface : 1;
    unsigned int on_module : 1;
    unsigned int on_coclass : 1;
    unsigned int on_apicontract : 1;
    unsigned int on_runtimeclass : 1;
    const char *display_name;
};

struct allowed_attr allowed_attr[] =
{
    /* attr                        { D ACF M   I Fn ARG T En Enm St Un Fi L  DI M  C AC  R  <display name> } */
    /* ATTR_ACTIVATABLE */         { 0, 0, 1,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "activatable" },
    /* ATTR_AGGREGATABLE */        { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "aggregatable" },
    /* ATTR_ALLOCATE */            { 0, 1, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "allocate" },
    /* ATTR_ANNOTATION */          { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "annotation" },
    /* ATTR_APPOBJECT */           { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "appobject" },
    /* ATTR_ASYNC */               { 0, 1, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_ASYNCUUID */           { 1, 0, 0,  1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, "async_uuid" },
    /* ATTR_AUTO_HANDLE */         { 1, 1, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */            { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */           { 1, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */              { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */            { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */                { 1, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "case" },
    /* ATTR_CODE */                { 0, 1, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "code" },
    /* ATTR_COMMSTATUS */          { 0, 0, 0,  0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "comm_status" },
    /* ATTR_COMPOSABLE */          { 0, 0, 1,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "composable" },
    /* ATTR_CONTEXTHANDLE */       { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTRACT */            { 0, 0, 0,  1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, "contract" },
    /* ATTR_CONTRACTVERSION */     { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "contractversion" },
    /* ATTR_CONTROL */             { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, "control" },
    /* ATTR_CUSTOM */              { 0, 0, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, "custom" },
    /* ATTR_DECODE */              { 0, 0, 0,  1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "decode" },
    /* ATTR_DEFAULT */             { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "default" },
    /* ATTR_DEFAULT_OVERLOAD */    { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "default_overload" },
    /* ATTR_DEFAULTBIND */         { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultbind" },
    /* ATTR_DEFAULTCOLLELEM */     { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */        { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */       { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "defaultvtable" },
    /* ATTR_DEPRECATED */          { 0, 0, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "deprecated" },
 /* ATTR_DISABLECONSISTENCYCHECK */{ 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "disable_consistency_check" },
    /* ATTR_DISPINTERFACE */       { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */         { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */             { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "dllname" },
    /* ATTR_DUAL */                { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENABLEALLOCATE */      { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "enable_allocate" },
    /* ATTR_ENCODE */              { 0, 0, 0,  1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "encode" },
    /* ATTR_ENDPOINT */            { 1, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */               { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EVENTADD */            { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "eventadd" },
    /* ATTR_EVENTREMOVE */         { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "eventremove" },
    /* ATTR_EXCLUSIVETO */         { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "exclusive_to" },
    /* ATTR_EXPLICIT_HANDLE */     { 1, 1, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_FAULTSTATUS */         { 0, 0, 0,  0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "fault_status" },
    /* ATTR_FLAGS */               { 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "flags" },
    /* ATTR_FORCEALLOCATE */       { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "force_allocate" },
    /* ATTR_HANDLE */              { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */         { 0, 0, 0,  1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, "helpcontext" },
    /* ATTR_HELPFILE */            { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */          { 0, 0, 0,  1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */   { 0, 0, 0,  1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */       { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */              { 0, 0, 0,  1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, "hidden" },
    /* ATTR_ID */                  { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */          { 1, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IGNORE */              { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "ignore" },
    /* ATTR_IIDIS */               { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */       { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */     { 1, 1, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */                  { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INPUTSYNC */           { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */            { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */             { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "lcid" },
    /* ATTR_LICENSED */            { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "licensed" },
    /* ATTR_LOCAL */               { 1, 0, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_MARSHALING_BEHAVIOR */ { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "marshaling_behavior" },
    /* ATTR_MAYBE */               { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "maybe" },
    /* ATTR_MESSAGE */             { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "message" },
    /* ATTR_NOCODE */              { 0, 1, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nocode" },
    /* ATTR_NONBROWSABLE */        { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */        { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */       { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_NOTIFY */              { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify" },
    /* ATTR_NOTIFYFLAG */          { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify_flag" },
    /* ATTR_OBJECT */              { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */                 { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */       { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIMIZE */            { 0, 0, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optimize" },
    /* ATTR_OPTIONAL */            { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */                 { 1, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_OVERLOAD */            { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "overload" },
    /* ATTR_PARAMLCID */           { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "lcid" },
    /* ATTR_PARTIALIGNORE */       { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "partial_ignore" },
    /* ATTR_POINTERDEFAULT */      { 1, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */         { 1, 0, 0,  0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROGID */              { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "progid" },
    /* ATTR_PROPGET */             { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */             { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */          { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PROTECTED */           { 0, 0, 0,  0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "protected" },
    /* ATTR_PROXY */               { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "proxy" },
    /* ATTR_PUBLIC */              { 0, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */               { 0, 0, 0,  0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */            { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "readonly" },
    /* ATTR_REPRESENTAS */         { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "represent_as" },
    /* ATTR_REQUESTEDIT */         { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */          { 0, 0, 0,  1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, "restricted" },
    /* ATTR_RETVAL */              { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */              { 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */              { 0, 0, 0,  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "source" },
    /* ATTR_STATIC */              { 0, 0, 1,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "static" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */              { 1, 0, 0,  0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */            { 1, 0, 0,  0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */          { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "switch_type" },
    /* ATTR_THREADING */           { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, "threading" },
    /* ATTR_TRANSMITAS */          { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UIDEFAULT */           { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "uidefault" },
    /* ATTR_USESGETLASTERROR */    { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "usesgetlasterror" },
    /* ATTR_USERMARSHAL */         { 0, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "user_marshal" },
    /* ATTR_UUID */                { 1, 0, 0,  1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, "uuid" },
    /* ATTR_V1ENUM */              { 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */              { 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */             { 1, 0, 0,  1, 0, 0, 1, 1, 0, 2, 0, 0, 1, 0, 1, 1, 0, 1, "version" },
    /* ATTR_VIPROGID */            { 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "vi_progid" },
    /* ATTR_WIREMARSHAL */         { 1, 0, 0,  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "wire_marshal" },
};

static const char *get_attr_display_name( enum attr_type attr_type )
{
    return allowed_attr[attr_type].display_name;
}

attr_list_t *append_attr( attr_list_t *list, attr_t *attr )
{
    attr_t *attr_existing;
    if (!attr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    if (!allowed_attr[attr->type].multiple)
    {
        LIST_FOR_EACH_ENTRY( attr_existing, list, attr_t, entry )
        {
            if (attr_existing->type != attr->type) continue;
            warning_at( &attr->where, "duplicate attribute %s\n", get_attr_display_name( attr->type ) );
            /* use the last attribute, like MIDL does */
            list_remove( &attr_existing->entry );
            break;
        }
    }
    list_add_tail( list, &attr->entry );
    return list;
}

attr_list_t *append_attr_list( attr_list_t *new_list, attr_list_t *old_list )
{
    struct list *entry;

    if (!old_list) return new_list;

    while ((entry = list_head( old_list )))
    {
        attr_t *attr = LIST_ENTRY( entry, attr_t, entry );
        list_remove( entry );
        new_list = append_attr( new_list, attr );
    }
    return new_list;
}

attr_list_t *append_attribs( attr_list_t *l1, attr_list_t *l2 )
{
    if (!l2) return l1;
    if (!l1 || l1 == l2) return l2;
    list_move_tail( l1, l2 );
    return l1;
}

attr_list_t *map_attrs( const attr_list_t *list, map_attrs_filter_t filter )
{
    attr_list_t *new_list;
    const attr_t *attr;
    attr_t *new_attr;

    if (!list) return NULL;

    new_list = xmalloc( sizeof(*list) );
    list_init( new_list );
    LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
    {
        if (filter && !filter( new_list, attr )) continue;
        new_attr = xmalloc( sizeof(*new_attr) );
        *new_attr = *attr;
        list_add_tail( new_list, &new_attr->entry );
    }
    return new_list;
}

attr_list_t *move_attr( attr_list_t *dst, attr_list_t *src, enum attr_type type )
{
    attr_t *attr;
    if (!src) return dst;
    LIST_FOR_EACH_ENTRY( attr, src, attr_t, entry )
    {
        if (attr->type == type)
        {
            list_remove( &attr->entry );
            return append_attr( dst, attr );
        }
    }
    return dst;
}

attr_list_t *check_apicontract_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_apicontract)
            error_at( &attr->where, "inapplicable attribute %s for apicontract %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_coclass_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_coclass)
            error_at( &attr->where, "inapplicable attribute %s for coclass %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_dispiface_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_dispinterface)
            error_at( &attr->where, "inapplicable attribute %s for dispinterface %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_enum_attrs( attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_enum)
            error_at( &attr->where, "inapplicable attribute %s for enum\n",
                      allowed_attr[attr->type].display_name );
    }
    return attrs;
}

attr_list_t *check_enum_member_attrs( attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_enum_member)
            error_at( &attr->where, "inapplicable attribute %s for enum member\n",
                      allowed_attr[attr->type].display_name );
    }
    return attrs;
}

attr_list_t *check_field_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_field)
            error_at( &attr->where, "inapplicable attribute %s for field %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_function_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_function)
            error_at( &attr->where, "inapplicable attribute %s for function %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_interface_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_interface)
            error_at( &attr->where, "inapplicable attribute %s for interface %s\n",
                      allowed_attr[attr->type].display_name, name );
        if (attr->type == ATTR_IMPLICIT_HANDLE)
        {
            const var_t *var = attr->u.pval;
            if (type_get_type( var->declspec.type ) == TYPE_BASIC &&
                type_basic_get_type( var->declspec.type ) == TYPE_BASIC_HANDLE)
                continue;
            if (is_aliaschain_attr( var->declspec.type, ATTR_HANDLE )) continue;
            error_at( &attr->where, "attribute %s requires a handle type in interface %s\n",
                      allowed_attr[attr->type].display_name, name );
        }
    }
    return attrs;
}

attr_list_t *check_library_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_library)
            error_at( &attr->where, "inapplicable attribute %s for library %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_module_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_module)
            error_at( &attr->where, "inapplicable attribute %s for module %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_runtimeclass_attrs( const char *name, attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_runtimeclass)
            error_at( &attr->where, "inapplicable attribute %s for runtimeclass %s\n",
                      allowed_attr[attr->type].display_name, name );
    }
    return attrs;
}

attr_list_t *check_struct_attrs( attr_list_t *attrs )
{
    int mask = winrt_mode ? 3 : 1;
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!(allowed_attr[attr->type].on_struct & mask))
            error_at( &attr->where, "inapplicable attribute %s for struct\n",
                      allowed_attr[attr->type].display_name );
    }
    return attrs;
}

attr_list_t *check_typedef_attrs( attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_type)
            error_at( &attr->where, "inapplicable attribute %s for typedef\n",
                      allowed_attr[attr->type].display_name );
    }
    return attrs;
}

attr_list_t *check_union_attrs( attr_list_t *attrs )
{
    const attr_t *attr;
    if (!attrs) return NULL;
    LIST_FOR_EACH_ENTRY( attr, attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_union)
            error_at( &attr->where, "inapplicable attribute %s for union\n",
                      allowed_attr[attr->type].display_name );
    }
    return attrs;
}

void check_arg_attrs( const var_t *arg )
{
    const attr_t *attr;
    if (!arg->attrs) return;
    LIST_FOR_EACH_ENTRY( attr, arg->attrs, const attr_t, entry )
    {
        if (!allowed_attr[attr->type].on_arg)
            error_at( &attr->where, "inapplicable attribute %s for argument %s\n",
                      allowed_attr[attr->type].display_name, arg->name );
    }
}
