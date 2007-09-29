/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef PREFS_H_
#define PREFS_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum pref_type pref_type;
enum pref_type {
    PT_INVALID = 0,
    PT_INT,
    PT_STRING
};

typedef struct prefs_data prefs_data;

/* describes all preferences in a program */
struct prefs_data {
    const TCHAR * name;
    pref_type     type;
    union {
        void *    data_void;
        int *     data_int;
        TCHAR **  data_str;
    } data;
};

TCHAR *prefs_to_tstr(prefs_data *prefs, size_t *tstr_len_cb_ptr);
int prefs_from_tstr(prefs_data *prefs, const TCHAR *str, size_t str_len_cb);

#ifdef __cplusplus
}
#endif

#endif
