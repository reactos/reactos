 10 cdecl ldap_abandon(ptr long) WLDAP32_ldap_abandon
 11 cdecl ldap_add(ptr str ptr) ldap_addA
 12 cdecl ldap_get_optionW(ptr long ptr)
 13 cdecl ldap_unbind(ptr) WLDAP32_ldap_unbind
 14 cdecl ldap_set_optionW(ptr long ptr)
 16 cdecl LdapGetLastError()
 17 cdecl cldap_open(str long) cldap_openA
 18 cdecl LdapMapErrorToWin32(long)
 19 cdecl ldap_compare(ptr str str str) ldap_compareA
 20 cdecl ldap_delete(ptr str) ldap_deleteA
 21 cdecl ldap_result2error(ptr ptr long) WLDAP32_ldap_result2error
 22 cdecl ldap_err2string(long) ldap_err2stringA
 23 cdecl ldap_modify(ptr str ptr) ldap_modifyA
 24 cdecl ldap_modrdn(ptr str ptr) ldap_modrdnA
 25 cdecl ldap_open(str long) ldap_openA
 26 cdecl ldap_first_entry(ptr ptr) WLDAP32_ldap_first_entry
 27 cdecl ldap_next_entry(ptr ptr) WLDAP32_ldap_next_entry
 28 cdecl cldap_openW(wstr long)
 29 cdecl LdapUTF8ToUnicode(str long ptr long)
 30 cdecl ldap_get_dn(ptr ptr) ldap_get_dnA
 31 cdecl ldap_dn2ufn(str) ldap_dn2ufnA
 32 cdecl ldap_first_attribute(ptr ptr ptr) ldap_first_attributeA
 33 cdecl ldap_next_attribute(ptr ptr ptr) ldap_next_attributeA
 34 cdecl ldap_get_values(ptr ptr str) ldap_get_valuesA
 35 cdecl ldap_get_values_len(ptr ptr str) ldap_get_values_lenA
 36 cdecl ldap_count_entries(ptr ptr) WLDAP32_ldap_count_entries
 37 cdecl ldap_count_values(ptr) ldap_count_valuesA
 38 cdecl ldap_value_free(ptr) ldap_value_freeA
 39 cdecl ldap_explode_dn(str long) ldap_explode_dnA
 40 cdecl ldap_result(ptr long long ptr ptr) WLDAP32_ldap_result
 41 cdecl ldap_msgfree(ptr) WLDAP32_ldap_msgfree
 42 cdecl ldap_addW(ptr wstr ptr)
 43 cdecl ldap_search(ptr str long str ptr long) ldap_searchA
 44 cdecl ldap_add_s(ptr str ptr) ldap_add_sA
 45 cdecl ldap_bind_s(ptr str str long) ldap_bind_sA
 46 cdecl ldap_unbind_s(ptr) WLDAP32_ldap_unbind_s
 47 cdecl ldap_delete_s(ptr str) ldap_delete_sA
 48 cdecl ldap_modify_s(ptr str ptr) ldap_modify_sA
 49 cdecl ldap_modrdn_s(ptr str ptr) ldap_modrdn_sA
 50 cdecl ldap_search_s(ptr str long str ptr long ptr) ldap_search_sA
 51 cdecl ldap_search_st(ptr str long str ptr long ptr ptr) ldap_search_stA
 52 cdecl ldap_compare_s(ptr str str str) ldap_compare_sA
 53 cdecl LdapUnicodeToUTF8(wstr long ptr long)
 54 cdecl ber_bvfree(ptr) WLDAP32_ber_bvfree
 55 cdecl cldap_openA(str long)
 56 cdecl ldap_addA(ptr str ptr)
 57 cdecl ldap_add_ext(ptr str ptr ptr ptr ptr) ldap_add_extA
 58 cdecl ldap_add_extA(ptr str ptr ptr ptr ptr)
 59 cdecl ldap_simple_bind(ptr str str) ldap_simple_bindA
 60 cdecl ldap_simple_bind_s(ptr str str) ldap_simple_bind_sA
 61 cdecl ldap_bind(ptr str str long) ldap_bindA
 62 cdecl ldap_add_extW(ptr wstr ptr ptr ptr ptr)
 63 cdecl ldap_add_ext_s(ptr str ptr ptr ptr) ldap_add_ext_sA
 64 cdecl ldap_add_ext_sA(ptr str ptr ptr ptr)
 65 cdecl ldap_add_ext_sW(ptr str ptr ptr ptr)
 66 cdecl ldap_add_sA(ptr str ptr)
 67 cdecl ldap_modrdn2(ptr str ptr long) ldap_modrdn2A
 68 cdecl ldap_modrdn2_s(ptr str ptr long) ldap_modrdn2_sA
 69 cdecl ldap_add_sW(ptr wstr ptr)
 70 cdecl ldap_bindA(ptr str str long)
 71 cdecl ldap_bindW(ptr wstr wstr long)
 72 cdecl ldap_bind_sA(ptr str str long)
 73 cdecl ldap_bind_sW(ptr wstr wstr long)
 74 cdecl ldap_close_extended_op(ptr long)
 75 cdecl ldap_compareA(ptr str str str)
 76 cdecl ldap_compareW(ptr wstr wstr wstr)
 77 cdecl ldap_count_values_len(ptr) WLDAP32_ldap_count_values_len
 78 cdecl ldap_compare_ext(ptr str str str ptr ptr ptr ptr) ldap_compare_extA
 79 cdecl ldap_value_free_len(ptr) WLDAP32_ldap_value_free_len
 80 cdecl ldap_compare_extA(ptr str str str ptr ptr ptr ptr)
 81 cdecl ldap_compare_extW(ptr wstr wstr wstr ptr ptr ptr ptr)
 82 cdecl ldap_perror(ptr ptr) WLDAP32_ldap_perror
 83 cdecl ldap_compare_ext_s(ptr str str str ptr ptr ptr) ldap_compare_ext_sA
 84 cdecl ldap_compare_ext_sA(ptr str str str ptr ptr ptr)
 85 cdecl ldap_compare_ext_sW(ptr wstr wstr wstr ptr ptr ptr)
 86 cdecl ldap_compare_sA(ptr str str str)
 87 cdecl ldap_compare_sW(ptr wstr wstr wstr)
 88 cdecl ldap_connect(ptr ptr)
 89 cdecl ldap_control_free(ptr) ldap_control_freeA
 90 cdecl ldap_control_freeA(ptr)
 91 cdecl ldap_control_freeW(ptr)
 92 cdecl ldap_controls_free(ptr) ldap_controls_freeA
 93 cdecl ldap_controls_freeA(ptr)
 94 cdecl ldap_controls_freeW(ptr)
 95 cdecl ldap_count_references(ptr ptr) WLDAP32_ldap_count_references
 96 cdecl ldap_count_valuesA(ptr)
 97 cdecl ldap_count_valuesW(ptr)
 98 cdecl ldap_create_page_control(ptr long ptr long ptr) ldap_create_page_controlA
 99 cdecl ldap_create_page_controlA(ptr long ptr long ptr)
100 cdecl ldap_create_page_controlW(ptr long ptr long ptr)
101 cdecl ldap_create_sort_control(ptr ptr long ptr) ldap_create_sort_controlA
102 cdecl ldap_create_sort_controlA(ptr ptr long ptr)
103 cdecl ldap_create_sort_controlW(ptr ptr long ptr)
104 cdecl ldap_deleteA(ptr str)
105 cdecl ldap_deleteW(ptr wstr)
106 cdecl ldap_delete_ext(ptr str ptr ptr ptr) ldap_delete_extA
107 cdecl ldap_delete_extA(ptr str ptr ptr ptr)
108 cdecl ldap_delete_extW(ptr wstr ptr ptr ptr)
109 cdecl ldap_delete_ext_s(ptr str ptr ptr) ldap_delete_ext_sA
110 cdecl ldap_delete_ext_sA(ptr str ptr ptr)
111 cdecl ldap_delete_ext_sW(ptr wstr ptr ptr)
112 cdecl ldap_delete_sA(ptr str)
113 cdecl ldap_delete_sW(ptr wstr)
114 cdecl ldap_dn2ufnW(wstr)
115 cdecl ldap_encode_sort_controlA(ptr ptr ptr long)
116 cdecl ldap_encode_sort_controlW(ptr ptr ptr long)
117 cdecl ldap_err2stringA(long)
118 cdecl ldap_err2stringW(long)
119 cdecl ldap_escape_filter_elementA(str long ptr long)
120 cdecl ldap_escape_filter_elementW(wstr long ptr long)
121 cdecl ldap_explode_dnA(str long)
122 cdecl ldap_explode_dnW(wstr long)
123 cdecl ldap_extended_operation(ptr str ptr ptr ptr ptr) ldap_extended_operationA
124 cdecl ldap_extended_operationA(ptr str ptr ptr ptr ptr)
125 cdecl ldap_extended_operationW(ptr wstr ptr ptr ptr ptr)
126 cdecl ldap_first_attributeA(ptr ptr ptr)
127 cdecl ldap_first_attributeW(ptr ptr ptr)
128 cdecl ldap_first_reference(ptr ptr) WLDAP32_ldap_first_reference
129 cdecl ldap_free_controls(ptr) ldap_free_controlsA
130 cdecl ldap_free_controlsA(ptr)
131 cdecl ldap_free_controlsW(ptr)
132 cdecl ldap_get_dnA(ptr ptr)
133 cdecl ldap_get_dnW(ptr ptr)
134 cdecl ldap_get_next_page(ptr ptr long ptr)
135 cdecl ldap_get_next_page_s(ptr ptr ptr long ptr ptr)
136 cdecl ldap_get_option(ptr long ptr) ldap_get_optionA
137 cdecl ldap_get_optionA(ptr long ptr)
138 cdecl ldap_get_paged_count(ptr ptr ptr ptr)
139 cdecl ldap_get_valuesA(ptr ptr str)
140 cdecl ldap_get_valuesW(ptr ptr wstr)
141 cdecl ldap_get_values_lenA(ptr ptr str)
142 cdecl ldap_get_values_lenW(ptr ptr wstr)
143 cdecl ldap_init(str long) ldap_initA
144 cdecl ldap_initA(str long)
145 cdecl ldap_initW(wstr long)
146 cdecl ldap_memfreeA(ptr)
147 cdecl ldap_memfreeW(ptr)
148 cdecl ldap_modifyA(ptr str ptr)
149 cdecl ldap_modifyW(ptr wstr ptr)
150 cdecl ldap_modify_ext(ptr str ptr ptr ptr ptr) ldap_modify_extA
151 cdecl ldap_modify_extA(ptr str ptr ptr ptr ptr)
152 cdecl ldap_modify_extW(ptr wstr ptr ptr ptr ptr)
153 cdecl ldap_modify_ext_s(ptr str ptr ptr ptr) ldap_modify_ext_sA
154 cdecl ldap_modify_ext_sA(ptr str ptr ptr ptr)
155 cdecl ldap_modify_ext_sW(ptr wstr ptr ptr ptr)
156 cdecl ldap_modify_sA(ptr str ptr)
157 cdecl ldap_modify_sW(ptr wstr ptr)
158 cdecl ldap_modrdn2A(ptr str ptr long)
159 cdecl ldap_modrdn2W(ptr wstr ptr long)
160 cdecl ldap_modrdn2_sA(ptr str ptr long)
161 cdecl ldap_modrdn2_sW(ptr wstr ptr long)
162 cdecl ldap_modrdnA(ptr str ptr)
163 cdecl ldap_modrdnW(ptr wstr ptr)
164 cdecl ldap_modrdn_sA(ptr str ptr)
165 cdecl ldap_modrdn_sW(ptr wstr ptr)
166 cdecl ldap_next_attributeA(ptr ptr ptr)
167 cdecl ldap_next_attributeW(ptr ptr ptr)
168 cdecl ldap_next_reference(ptr ptr) WLDAP32_ldap_next_reference
169 cdecl ldap_openA(str long)
170 cdecl ldap_openW(wstr long)
171 cdecl ldap_parse_page_control(ptr ptr ptr ptr) ldap_parse_page_controlA
172 cdecl ldap_parse_page_controlA(ptr ptr ptr ptr)
173 cdecl ldap_parse_page_controlW(ptr ptr ptr ptr)
174 cdecl ldap_parse_reference(ptr ptr ptr) ldap_parse_referenceA
175 cdecl ldap_parse_referenceA(ptr ptr ptr)
176 cdecl ldap_parse_referenceW(ptr ptr ptr)
177 cdecl ldap_parse_result(ptr ptr ptr ptr ptr ptr ptr long) ldap_parse_resultA
178 cdecl ldap_parse_resultA(ptr ptr ptr ptr ptr ptr ptr long)
179 cdecl ldap_parse_resultW(ptr ptr ptr ptr ptr ptr ptr long)
180 cdecl ldap_parse_sort_control(ptr ptr ptr ptr) ldap_parse_sort_controlA
181 cdecl ldap_parse_sort_controlA(ptr ptr ptr ptr)
182 cdecl ldap_parse_sort_controlW(ptr ptr ptr ptr)
183 cdecl ldap_rename_ext(ptr str str str long ptr ptr ptr) ldap_rename_extA
184 cdecl ldap_rename_extA(ptr str str str long ptr ptr ptr)
185 cdecl ldap_rename_extW(ptr wstr wstr wstr long ptr ptr ptr)
186 cdecl ldap_rename_ext_s(ptr str str str long ptr ptr) ldap_rename_ext_sA
187 cdecl ldap_rename_ext_sA(ptr str str str long ptr ptr)
188 cdecl ldap_rename_ext_sW(ptr wstr wstr wstr long ptr ptr)
189 cdecl ldap_searchA(ptr str long str ptr long)
190 cdecl ldap_searchW(ptr wstr long wstr ptr long)
191 cdecl ldap_search_abandon_page(ptr ptr)
192 cdecl ldap_search_ext(ptr str long str ptr long ptr ptr long long ptr) ldap_search_extA
193 cdecl ldap_search_extA(ptr str long str ptr long ptr ptr long long ptr)
194 cdecl ldap_search_extW(ptr wstr long wstr ptr long ptr ptr long long ptr)
195 cdecl ldap_search_ext_s(ptr str long str ptr long ptr ptr ptr long ptr) ldap_search_ext_sA
196 cdecl ldap_search_ext_sA(ptr str long str ptr long ptr ptr ptr long ptr)
197 cdecl ldap_escape_filter_element(str long ptr long) ldap_escape_filter_elementA
198 stub ldap_set_dbg_flags
199 stub ldap_set_dbg_routine
200 cdecl ldap_memfree(ptr) ldap_memfreeA
201 cdecl ldap_startup(ptr ptr)
202 cdecl ldap_cleanup(long)
203 cdecl ldap_search_ext_sW(ptr wstr long wstr ptr long ptr ptr ptr long ptr)
204 cdecl ldap_search_init_page(ptr str long str ptr long ptr ptr long long ptr) ldap_search_init_pageA
205 cdecl ldap_search_init_pageA(ptr str long str ptr long ptr ptr long long ptr)
206 cdecl ldap_search_init_pageW(ptr wstr long wstr ptr long ptr ptr long long ptr)
207 cdecl ldap_search_sA(ptr str long str ptr long ptr)
208 cdecl ldap_search_sW(ptr wstr long wstr ptr long ptr)
209 cdecl ldap_search_stA(ptr str long str ptr long ptr ptr)
210 cdecl ldap_search_stW(ptr wstr long wstr ptr long ptr ptr)
211 cdecl ldap_set_option(ptr long ptr) ldap_set_optionA
212 cdecl ldap_set_optionA(ptr long ptr)
213 cdecl ldap_simple_bindA(ptr str str)
214 cdecl ldap_simple_bindW(ptr wstr wstr)
215 cdecl ldap_simple_bind_sA(ptr str str)
216 cdecl ldap_simple_bind_sW(ptr wstr wstr)
217 cdecl ldap_sslinit(str long long) ldap_sslinitA
218 cdecl ldap_sslinitA(str long long)
219 cdecl ldap_sslinitW(str long long)
220 cdecl ldap_ufn2dn(str ptr) ldap_ufn2dnA
221 cdecl ldap_ufn2dnA(str ptr)
222 cdecl ldap_ufn2dnW(wstr ptr)
223 cdecl ldap_value_freeA(ptr)
224 cdecl ldap_value_freeW(ptr)
230 cdecl ldap_check_filterA(ptr str)
231 cdecl ldap_check_filterW(ptr wstr)
232 cdecl ldap_dn2ufnA(str)
300 cdecl ber_init(ptr) WLDAP32_ber_init
301 cdecl ber_free(ptr long) WLDAP32_ber_free
302 cdecl ber_bvecfree(ptr) WLDAP32_ber_bvecfree
303 cdecl ber_bvdup(ptr) WLDAP32_ber_bvdup
304 cdecl ber_alloc_t(long) WLDAP32_ber_alloc_t
305 cdecl ber_skip_tag(ptr ptr) WLDAP32_ber_skip_tag
306 cdecl ber_peek_tag(ptr ptr) WLDAP32_ber_peek_tag
307 cdecl ber_first_element(ptr ptr ptr) WLDAP32_ber_first_element
308 cdecl ber_next_element(ptr ptr ptr) WLDAP32_ber_next_element
309 cdecl ber_flatten(ptr ptr) WLDAP32_ber_flatten
310 varargs ber_printf(ptr str) WLDAP32_ber_printf
311 varargs ber_scanf(ptr str) WLDAP32_ber_scanf
312 cdecl ldap_conn_from_msg(ptr ptr)
313 cdecl ldap_sasl_bindW(ptr wstr wstr ptr ptr ptr ptr)
314 cdecl ldap_sasl_bind_sW(ptr wstr wstr ptr ptr ptr ptr)
315 cdecl ldap_sasl_bindA(ptr str str ptr ptr ptr ptr)
316 cdecl ldap_sasl_bind_sA(ptr str str ptr ptr ptr ptr)
317 cdecl ldap_parse_extended_resultW(ptr ptr ptr ptr long)
318 cdecl ldap_parse_extended_resultA(ptr ptr ptr ptr long)
319 cdecl ldap_create_vlv_controlW(ptr ptr long ptr)
320 cdecl ldap_create_vlv_controlA(ptr ptr long ptr)
321 cdecl ldap_parse_vlv_controlW(ptr ptr ptr ptr ptr ptr)
322 cdecl ldap_parse_vlv_controlA(ptr ptr ptr ptr ptr ptr)
329 cdecl ldap_start_tls_sW(ptr ptr ptr ptr ptr)
330 cdecl ldap_start_tls_sA(ptr ptr ptr ptr ptr)
331 cdecl ldap_stop_tls_s(ptr)
332 cdecl ldap_extended_operation_sW(ptr wstr ptr ptr ptr ptr ptr)
333 cdecl ldap_extended_operation_sA(ptr str ptr ptr ptr ptr ptr)
