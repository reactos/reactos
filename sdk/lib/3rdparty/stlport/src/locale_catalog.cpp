/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */
#include "stlport_prefix.h"

#include <hash_map>
#include <string>

#include <locale>
#include <istream>

#include "c_locale.h"
#include "locale_impl.h"
#include "acquire_release.h"

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

// Those wrappers are needed to use locale functions in __acquire_category,
// all functions have to present the same prototype.

static void* _Loc_ctype_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_ctype_create(s, hint, __err_code); }
static void* _Loc_codecvt_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_codecvt_create(s, hint, __err_code); }
static void* _Loc_numeric_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_numeric_create(s, hint, __err_code); }
static void* _Loc_time_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_time_create(s, hint, __err_code); }
static void* _Loc_collate_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_collate_create(s, hint, __err_code); }
static void* _Loc_monetary_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_monetary_create(s, hint, __err_code); }
static void* _Loc_messages_create(const char * s, _Locale_name_hint* hint, int *__err_code)
{ return _Locale_messages_create(s, hint, __err_code); }

static char const* _Loc_ctype_name(void* l, char* s)
{ return _Locale_ctype_name((_Locale_ctype*)l, s); }
static char const* _Loc_codecvt_name(void* l, char* s)
{ return _Locale_codecvt_name((_Locale_codecvt*)l, s); }
static char const* _Loc_numeric_name(void* l, char* s)
{ return _Locale_numeric_name((_Locale_numeric*)l, s); }
static char const* _Loc_time_name(void* l, char* s)
{ return _Locale_time_name((_Locale_time*)l, s); }
static char const* _Loc_collate_name(void* l, char* s)
{ return _Locale_collate_name((_Locale_collate*)l, s); }
static char const* _Loc_monetary_name(void* l, char* s)
{ return _Locale_monetary_name((_Locale_monetary*)l, s); }
static char const* _Loc_messages_name(void* l, char* s)
{ return _Locale_messages_name((_Locale_messages*)l, s); }

static const char* _Loc_ctype_default(char* p)
{ return _Locale_ctype_default(p); }
static const char* _Loc_numeric_default(char * p)
{ return _Locale_numeric_default(p); }
static const char* _Loc_time_default(char* p)
{ return _Locale_time_default(p); }
static const char* _Loc_collate_default(char* p)
{ return _Locale_collate_default(p); }
static const char* _Loc_monetary_default(char* p)
{ return _Locale_monetary_default(p); }
static const char* _Loc_messages_default(char* p)
{ return _Locale_messages_default(p); }

static void _Loc_ctype_destroy(void* p)    {_Locale_ctype_destroy((_Locale_ctype*)p); }
static void _Loc_codecvt_destroy(void* p)    {_Locale_codecvt_destroy((_Locale_codecvt*)p); }
static void _Loc_numeric_destroy(void* p)  {_Locale_numeric_destroy((_Locale_numeric*)p); }
static void _Loc_time_destroy(void* p)     {_Locale_time_destroy((_Locale_time*)p);}
static void _Loc_collate_destroy(void* p)  {_Locale_collate_destroy((_Locale_collate*)p);}
static void _Loc_monetary_destroy(void* p) {_Locale_monetary_destroy((_Locale_monetary*)p);}
static void _Loc_messages_destroy(void* p) {_Locale_messages_destroy((_Locale_messages*)p);}

typedef void* (*loc_create_func_t)(const char *, _Locale_name_hint*, int *__err_code);
typedef char const* (*loc_name_func_t)(void* l, char* s);
typedef void (*loc_destroy_func_t)(void* l);
typedef const char* (*loc_default_name_func_t)(char* s);
typedef char const* (*loc_extract_name_func_t)(const char*, char*, _Locale_name_hint*, int *__err_code);

//----------------------------------------------------------------------
// Acquire and release low-level category objects.  The whole point of
// this is so that we don't allocate (say) four different _Locale_ctype
// objects for a single locale.

// Global hash tables for category objects.
typedef hash_map<string, pair<void*, size_t>, hash<string>, equal_to<string> > Category_Map;

// Look up a category by name
static Category_Map** ctype_hash() {
  static Category_Map *_S_ctype_hash = 0;
  return &_S_ctype_hash;
}
static Category_Map** codecvt_hash() {
  static Category_Map *_S_codecvt_hash = 0;
  return &_S_codecvt_hash;
}
static Category_Map** numeric_hash() {
  static Category_Map *_S_numeric_hash = 0;
  return &_S_numeric_hash;
}
static Category_Map** time_hash() {
  static Category_Map *_S_time_hash = 0;
  return &_S_time_hash;
}
static Category_Map** collate_hash() {
  static Category_Map *_S_collate_hash = 0;
  return &_S_collate_hash;
}
static Category_Map** monetary_hash() {
  static Category_Map *_S_monetary_hash = 0;
  return &_S_monetary_hash;
}
static Category_Map** messages_hash() {
  static Category_Map *_S_messages_hash;
  return &_S_messages_hash;
}

// We have a single lock for all of the hash tables.  We may wish to
// replace it with six different locks.
/* REFERENCED */
static _STLP_STATIC_MUTEX& category_hash_mutex() {
  static _STLP_STATIC_MUTEX lock _STLP_MUTEX_INITIALIZER;
  return lock;
}

static void*
__acquire_category(const char* &name, char *buf, _Locale_name_hint* hint,
                   loc_extract_name_func_t extract_name,
                   loc_create_func_t create_obj, loc_default_name_func_t default_name,
                   Category_Map ** M, int *__err_code) {
#if !defined (__BORLANDC__) || (__BORLANDC__ >= 0x564)
  typedef Category_Map::iterator Category_iterator;
  pair<Category_iterator, bool> result;
#else
#  if !defined(_STLP_DEBUG)
  pair<_Ht_iterator<_Slist_iterator<pair<const string,pair<void *,unsigned int> >,_Nonconst_traits<pair<const string,pair<void *,unsigned int> > > >,_NonLocalHashMapTraitsT<pair<const string,pair<void *,unsigned int> > > >, bool> result;
#  else
  pair<_DBG_iter<_NonDbg_hashtable<pair<const string,pair<void *,unsigned int> >,string,hash<string>,_HashMapTraitsT<pair<const string,pair<void *,unsigned int> > >,_Select1st<pair<const string,pair<void *,unsigned int> > >,_DbgEqual<string,equal_to<string> >,allocator<pair<const string,pair<void *,unsigned int> > > >,_DbgTraits<_NonLocalHashMapTraitsT<pair<const string,pair<void *,unsigned int> > > > >, bool> result;
#  endif
#endif

  *__err_code = _STLP_LOC_UNDEFINED;

  // Find what name to look for. Be careful if user requests the default.
  if (name[0] == 0) {
    name = default_name(buf);
    if (name == 0 || name[0] == 0)
      name = "C";
  }
  else {
    const char* cname = extract_name(name, buf, hint, __err_code);
    if (cname == 0) {
      return 0;
    }
    name = cname;
  }

  Category_Map::value_type __e(name, pair<void*,size_t>((void*)0,size_t(0)));

  _STLP_auto_lock sentry(category_hash_mutex());

  if (!*M)
    *M = new Category_Map();

  // Look for an existing entry with that name.
  result = (*M)->insert_noresize(__e);

  if (result.second) {
    // There was no entry in the map already.  Create the category.
    (*result.first).second.first = create_obj(name, hint, __err_code);
    if (!(*result.first).second.first) {
      (*M)->erase(result.first);
#if defined (_STLP_LEAKS_PEDANTIC)
      if ((*M)->empty()) {
        delete *M;
        *M = 0;
      }
#endif
      return 0;
    }
  }

  // Increment the reference count.
  ++((*result.first).second.second);

  return (*result.first).second.first;
}

static void
__release_category(void* cat,
                   loc_destroy_func_t destroy_fun,
                   loc_name_func_t get_name,
                   Category_Map** M) {
  Category_Map *pM = *M;

  if (cat && pM) {
    // Find the name of the category object.
    char buf[_Locale_MAX_SIMPLE_NAME + 1];
    char const* name = get_name(cat, buf);

    if (name != 0) {
      _STLP_auto_lock sentry(category_hash_mutex());
      Category_Map::iterator it = pM->find(name);
      if (it != pM->end()) {
        // Decrement the ref count.  If it goes to zero, delete this category
        // from the map.
        if (--((*it).second.second) == 0) {
          void* cat1 = (*it).second.first;
          destroy_fun(cat1);
          pM->erase(it);
#if defined (_STLP_LEAKS_PEDANTIC)
          if (pM->empty()) {
            delete pM;
            *M = 0;
          }
#endif
        }
      }
    }
  }
}

_Locale_ctype* _STLP_CALL __acquire_ctype(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_ctype*, __acquire_category(name, buf, hint,
                                                               _Locale_extract_ctype_name, _Loc_ctype_create, _Loc_ctype_default,
                                                               ctype_hash(), __err_code));
}
_Locale_codecvt* _STLP_CALL __acquire_codecvt(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_codecvt*, __acquire_category(name, buf, hint,
                                                                 _Locale_extract_ctype_name, _Loc_codecvt_create, _Loc_ctype_default,
                                                                 codecvt_hash(), __err_code));
}
_Locale_numeric* _STLP_CALL __acquire_numeric(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_numeric*, __acquire_category(name, buf, hint,
                                                                 _Locale_extract_numeric_name, _Loc_numeric_create, _Loc_numeric_default,
                                                                 numeric_hash(), __err_code));
}
_Locale_time* _STLP_CALL __acquire_time(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_time*, __acquire_category(name, buf, hint,
                                                              _Locale_extract_time_name, _Loc_time_create, _Loc_time_default,
                                                              time_hash(), __err_code));
}
_Locale_collate* _STLP_CALL __acquire_collate(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_collate*, __acquire_category(name, buf, hint,
                                                                 _Locale_extract_collate_name, _Loc_collate_create, _Loc_collate_default,
                                                                 collate_hash(), __err_code));
}
_Locale_monetary* _STLP_CALL __acquire_monetary(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_monetary*, __acquire_category(name, buf, hint,
                                                                  _Locale_extract_monetary_name, _Loc_monetary_create, _Loc_monetary_default,
                                                                  monetary_hash(), __err_code));
}
_Locale_messages* _STLP_CALL __acquire_messages(const char* &name, char *buf, _Locale_name_hint* hint, int *__err_code) {
  return __REINTERPRET_CAST(_Locale_messages*, __acquire_category(name, buf, hint,
                                                                  _Locale_extract_messages_name, _Loc_messages_create, _Loc_messages_default,
                                                                  messages_hash(), __err_code));
}

void _STLP_CALL __release_ctype(_Locale_ctype* cat)
{ __release_category(cat, _Loc_ctype_destroy, _Loc_ctype_name, ctype_hash()); }
void _STLP_CALL __release_codecvt(_Locale_codecvt* cat)
{ __release_category(cat, _Loc_codecvt_destroy, _Loc_codecvt_name, codecvt_hash()); }
void _STLP_CALL __release_numeric(_Locale_numeric* cat)
{ __release_category(cat, _Loc_numeric_destroy, _Loc_numeric_name, numeric_hash()); }
void _STLP_CALL __release_time(_Locale_time* cat)
{ __release_category(cat, _Loc_time_destroy, _Loc_time_name, time_hash()); }
void _STLP_CALL __release_collate(_Locale_collate* cat)
{ __release_category(cat, _Loc_collate_destroy, _Loc_collate_name, collate_hash()); }
void _STLP_CALL __release_monetary(_Locale_monetary* cat)
{ __release_category(cat, _Loc_monetary_destroy, _Loc_monetary_name, monetary_hash()); }
void _STLP_CALL __release_messages(_Locale_messages* cat)
{ __release_category(cat, _Loc_messages_destroy, _Loc_messages_name, messages_hash()); }

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE
