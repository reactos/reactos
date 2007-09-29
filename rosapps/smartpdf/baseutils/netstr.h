/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef NETSTR_H__
#define NETSTR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define NETSTR_SEP_TC _T(':')
#define NETSTR_END_TC _T(',')

size_t  digits_for_number(int num);

size_t  netstr_int_serialized_len_cb(int num);
size_t  netstr_tstr_serialized_len_cb(const TCHAR *str);
size_t  netstr_tstrn_serialized_len_cb(size_t str_len_cch);
int     netstr_tstr_serialize(const TCHAR *str, TCHAR **buf_ptr, size_t *buf_len_cb_ptr);
int     netstr_int_serialize(int num, TCHAR **buf_ptr, size_t *buf_len_cb_ptr);
int     netstr_parse_str(const TCHAR **str_ptr, size_t *str_len_cb_ptr, const TCHAR **str_out, size_t *str_len_cch_out);
int     netstr_parse_int(const TCHAR **str_ptr, size_t *str_len_cb_ptr, int *int_out);

#ifdef __cplusplus
}
#endif

#endif
