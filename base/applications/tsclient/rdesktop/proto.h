/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef RDESKTOP_PROTO_H
#define RDESKTOP_PROTO_H

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */
/* bitmap.c */
BOOL bitmap_decompress(uint8 * output, int width, int height, uint8 * input, int size, int Bpp);
/* cache.c */
void cache_rebuild_bmpcache_linked_list(RDPCLIENT * This, uint8 id, sint16 * idx, int count);
void cache_bump_bitmap(RDPCLIENT * This, uint8 id, uint16 idx, int bump);
void cache_evict_bitmap(RDPCLIENT * This, uint8 id);
HBITMAP cache_get_bitmap(RDPCLIENT * This, uint8 id, uint16 idx);
void cache_put_bitmap(RDPCLIENT * This, uint8 id, uint16 idx, HBITMAP bitmap);
void cache_save_state(RDPCLIENT * This);
FONTGLYPH *cache_get_font(RDPCLIENT * This, uint8 font, uint16 character);
void cache_put_font(RDPCLIENT * This, uint8 font, uint16 character, uint16 offset, uint16 baseline, uint16 width,
		    uint16 height, HGLYPH pixmap);
DATABLOB *cache_get_text(RDPCLIENT * This, uint8 cache_id);
void cache_put_text(RDPCLIENT * This, uint8 cache_id, void *data, int length);
uint8 *cache_get_desktop(RDPCLIENT * This, uint32 offset, int cx, int cy, int bytes_per_pixel);
void cache_put_desktop(RDPCLIENT * This, uint32 offset, int cx, int cy, int scanline, int bytes_per_pixel,
		       uint8 * data);
HCURSOR cache_get_cursor(RDPCLIENT * This, uint16 cache_idx);
void cache_put_cursor(RDPCLIENT * This, uint16 cache_idx, HCURSOR cursor);
/* channels.c */
VCHANNEL *channel_register(RDPCLIENT * This, char *name, uint32 flags, void (*callback) (RDPCLIENT *, STREAM));
STREAM channel_init(RDPCLIENT * This, VCHANNEL * channel, uint32 length);
void channel_send(RDPCLIENT * This, STREAM s, VCHANNEL * channel);
void channel_process(RDPCLIENT * This, STREAM s, uint16 mcs_channel);
/* cliprdr.c */
void cliprdr_send_simple_native_format_announce(RDPCLIENT * This, uint32 format);
void cliprdr_send_native_format_announce(RDPCLIENT * This, uint8 * formats_data, uint32 formats_data_length);
void cliprdr_send_data_request(RDPCLIENT * This, uint32 format);
void cliprdr_send_data(RDPCLIENT * This, uint8 * data, uint32 length);
void cliprdr_set_mode(RDPCLIENT * This, const char *optarg);
BOOL cliprdr_init(RDPCLIENT * This);
#if 0
/* disk.c */
int disk_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg);
NTSTATUS disk_query_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM out);
NTSTATUS disk_set_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM in, STREAM out);
NTSTATUS disk_check_notify(RDPCLIENT * This, NTHANDLE handle);
NTSTATUS disk_create_notify(RDPCLIENT * This, NTHANDLE handle, uint32 info_class);
NTSTATUS disk_query_volume_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM out);
NTSTATUS disk_query_directory(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, char *pattern, STREAM out);
#endif
/* mppc.c */
int mppc_expand(RDPCLIENT * This, uint8 * data, uint32 clen, uint8 ctype, uint32 * roff, uint32 * rlen);
/* ewmhints.c */
int get_current_workarea(RDPCLIENT * This, uint32 * x, uint32 * y, uint32 * width, uint32 * height);
void ewmh_init(RDPCLIENT * This);
/* iso.c */
STREAM iso_init(RDPCLIENT * This, int length);
BOOL iso_send(RDPCLIENT * This, STREAM s);
STREAM iso_recv(RDPCLIENT * This, uint8 * rdpver);
BOOL iso_connect(RDPCLIENT * This, char *server, char *cookie);
BOOL iso_reconnect(RDPCLIENT * This, char *server, char *cookie);
BOOL iso_disconnect(RDPCLIENT * This);
void iso_reset_state(RDPCLIENT * This);
/* licence.c */
void licence_process(RDPCLIENT * This, STREAM s);
/* mcs.c */
STREAM mcs_init(RDPCLIENT * This, int length);
BOOL mcs_send_to_channel(RDPCLIENT * This, STREAM s, uint16 channel);
BOOL mcs_send(RDPCLIENT * This, STREAM s);
STREAM mcs_recv(RDPCLIENT * This, uint16 * channel, uint8 * rdpver);
BOOL mcs_connect(RDPCLIENT * This, char *server, char *cookie, STREAM mcs_data);
BOOL mcs_reconnect(RDPCLIENT * This, char *server, char *cookie, STREAM mcs_data);
void mcs_disconnect(RDPCLIENT * This);
void mcs_reset_state(RDPCLIENT * This);
/* orders.c */
void process_orders(RDPCLIENT * This, STREAM s, uint16 num_orders);
void reset_order_state(RDPCLIENT * This);
/* parallel.c */
int parallel_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg);
/* printer.c */
int printer_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg);
/* printercache.c */
int printercache_load_blob(char *printer_name, uint8 ** data);
void printercache_process(RDPCLIENT * This, STREAM s);
/* pstcache.c */
void pstcache_touch_bitmap(RDPCLIENT * This, uint8 cache_id, uint16 cache_idx, uint32 stamp);
BOOL pstcache_load_bitmap(RDPCLIENT * This, uint8 cache_id, uint16 cache_idx);
BOOL pstcache_save_bitmap(RDPCLIENT * This, uint8 cache_id, uint16 cache_idx, uint8 * key, uint8 width, uint8 height,
			  uint16 length, uint8 * data);
int pstcache_enumerate(RDPCLIENT * This, uint8 id, HASH_KEY * keylist);
BOOL pstcache_init(RDPCLIENT * This, uint8 cache_id);
/* rdesktop.c */
int main(int argc, char *argv[]);
void generate_random(uint8 * random);
void error(char *format, ...);
void warning(char *format, ...);
void unimpl(char *format, ...);
void hexdump(unsigned char *p, unsigned int len);
char *next_arg(char *src, char needle);
void toupper_str(char *p);
BOOL str_startswith(const char *s, const char *prefix);
BOOL str_handle_lines(RDPCLIENT * This, const char *input, char **rest, str_handle_lines_t linehandler, void *data);
BOOL subprocess(RDPCLIENT * This, char *const argv[], str_handle_lines_t linehandler, void *data);
char *l_to_a(long N, int base);
int load_licence(RDPCLIENT * This, unsigned char **data);
void save_licence(RDPCLIENT * This, unsigned char *data, int length);
BOOL rd_pstcache_mkdir(void);
int rd_open_file(char *filename);
void rd_close_file(int fd);
int rd_read_file(int fd, void *ptr, int len);
int rd_write_file(int fd, void *ptr, int len);
int rd_lseek_file(int fd, int offset);
BOOL rd_lock_file(int fd, int start, int len);
/* rdp5.c */
BOOL rdp5_process(RDPCLIENT * This, STREAM s);
/* rdp.c */
void rdp_out_unistr(RDPCLIENT * This, STREAM s, wchar_t *string, int len);
int rdp_in_unistr(RDPCLIENT * This, STREAM s, wchar_t *string, int uni_len);
BOOL rdp_send_input(RDPCLIENT * This, uint32 time, uint16 message_type, uint16 device_flags, uint16 param1,
		    uint16 param2);
BOOL rdp_send_client_window_status(RDPCLIENT * This, int status);
void process_colour_pointer_pdu(RDPCLIENT * This, STREAM s);
void process_cached_pointer_pdu(RDPCLIENT * This, STREAM s);
void process_system_pointer_pdu(RDPCLIENT * This, STREAM s);
void process_bitmap_updates(RDPCLIENT * This, STREAM s);
void process_palette(RDPCLIENT * This, STREAM s);
void process_disconnect_pdu(STREAM s, uint32 * ext_disc_reason);
void rdp_main_loop(RDPCLIENT * This, BOOL * deactivated, uint32 * ext_disc_reason);
BOOL rdp_loop(RDPCLIENT * This, BOOL * deactivated, uint32 * ext_disc_reason);
BOOL rdp_connect(RDPCLIENT * This, char *server, uint32 flags, wchar_t *username, wchar_t *domain, wchar_t *password, wchar_t *command,
		 wchar_t *directory, wchar_t *hostname, char *cookie);
BOOL rdp_reconnect(RDPCLIENT * This, char *server, uint32 flags, wchar_t *username, wchar_t *domain, wchar_t *password, wchar_t *command,
		   wchar_t *directory, wchar_t *hostname, char *cookie);
void rdp_reset_state(RDPCLIENT * This);
void rdp_disconnect(RDPCLIENT * This);
#if 0
/* rdpdr.c */
int get_device_index(RDPCLIENT * This, NTHANDLE handle);
void convert_to_unix_filename(char *filename);
BOOL rdpdr_init(RDPCLIENT * This);
void rdpdr_add_fds(RDPCLIENT * This, int *n, fd_set * rfds, fd_set * wfds, struct timeval *tv, BOOL * timeout);
struct async_iorequest *rdpdr_remove_iorequest(RDPCLIENT * This, struct async_iorequest *prev,
					       struct async_iorequest *iorq);
void rdpdr_check_fds(RDPCLIENT * This, fd_set * rfds, fd_set * wfds, BOOL timed_out);
BOOL rdpdr_abort_io(RDPCLIENT * This, uint32 fd, uint32 major, NTSTATUS status);
#endif
#if 0
/* rdpsnd.c */
void rdpsnd_send_completion(RDPCLIENT * This, uint16 tick, uint8 packet_index);
BOOL rdpsnd_init(RDPCLIENT * This);
/* rdpsnd_oss.c */
BOOL wave_out_open(void);
void wave_out_close(void);
BOOL wave_out_format_supported(WAVEFORMATEX * pwfx);
BOOL wave_out_set_format(WAVEFORMATEX * pwfx);
void wave_out_volume(uint16 left, uint16 right);
void wave_out_write(STREAM s, uint16 tick, uint8 index);
void wave_out_play(void);
#endif
/* secure.c */
void sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt);
void sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2);
void buf_out_uint32(uint8 * buffer, uint32 value);
void sec_sign(uint8 * signature, int siglen, uint8 * session_key, int keylen, uint8 * data,
	      int datalen);
void sec_decrypt(RDPCLIENT * This, uint8 * data, int length);
STREAM sec_init(RDPCLIENT * This, uint32 flags, int maxlen);
BOOL sec_send_to_channel(RDPCLIENT * This, STREAM s, uint32 flags, uint16 channel);
BOOL sec_send(RDPCLIENT * This, STREAM s, uint32 flags);
void sec_process_mcs_data(RDPCLIENT * This, STREAM s);
STREAM sec_recv(RDPCLIENT * This, uint8 * rdpver);
BOOL sec_connect(RDPCLIENT * This, char *server, wchar_t *hostname, char *cookie);
BOOL sec_reconnect(RDPCLIENT * This, char *server, wchar_t *hostname, char *cookie);
void sec_disconnect(RDPCLIENT * This);
void sec_reset_state(RDPCLIENT * This);
#if 0
/* serial.c */
int serial_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg);
BOOL serial_get_event(RDPCLIENT * This, NTHANDLE handle, uint32 * result);
BOOL serial_get_timeout(RDPCLIENT * This, NTHANDLE handle, uint32 length, uint32 * timeout, uint32 * itv_timeout);
#endif
/* tcp.c */
STREAM tcp_init(RDPCLIENT * This, uint32 maxlen);
BOOL tcp_send(RDPCLIENT * This, STREAM s);
STREAM tcp_recv(RDPCLIENT * This, STREAM s, uint32 length);
BOOL tcp_connect(RDPCLIENT * This, char *server);
BOOL tcp_disconnect(RDPCLIENT * This);
wchar_t *tcp_get_address(RDPCLIENT * This);
void tcp_reset_state(RDPCLIENT * This);
/* xclip.c */
void ui_clip_format_announce(RDPCLIENT * This, uint8 * data, uint32 length);
void ui_clip_handle_data(RDPCLIENT * This, uint8 * data, uint32 length);
void ui_clip_request_failed(RDPCLIENT * This);
void ui_clip_request_data(RDPCLIENT * This, uint32 format);
void ui_clip_sync(RDPCLIENT * This);
void ui_clip_set_mode(RDPCLIENT * This, const char *optarg);
void xclip_init(RDPCLIENT * This);
void xclip_deinit(RDPCLIENT * This);
#if 0
/* xkeymap.c */
BOOL xkeymap_from_locale(RDPCLIENT * This, const char *locale);
FILE *xkeymap_open(const char *filename);
void xkeymap_init(RDPCLIENT * This);
BOOL handle_special_keys(RDPCLIENT * This, uint32 keysym, unsigned int state, uint32 ev_time, BOOL pressed);
key_translation xkeymap_translate_key(RDPCLIENT * This, uint32 keysym, unsigned int keycode, unsigned int state);
void xkeymap_send_keys(RDPCLIENT * This, uint32 keysym, unsigned int keycode, unsigned int state, uint32 ev_time,
		       BOOL pressed, uint8 nesting);
uint16 xkeymap_translate_button(unsigned int button);
char *get_ksname(uint32 keysym);
void save_remote_modifiers(RDPCLIENT * This, uint8 scancode);
void restore_remote_modifiers(RDPCLIENT * This, uint32 ev_time, uint8 scancode);
void ensure_remote_modifiers(RDPCLIENT * This, uint32 ev_time, key_translation tr);
unsigned int read_keyboard_state(RDPCLIENT * This);
uint16 ui_get_numlock_state(RDPCLIENT * This, unsigned int state);
void reset_modifier_keys(RDPCLIENT * This);
void rdp_send_scancode(RDPCLIENT * This, uint32 time, uint16 flags, uint8 scancode);
#endif
/* xwin.c */
BOOL get_key_state(RDPCLIENT * This, unsigned int state, uint32 keysym);
BOOL ui_init(RDPCLIENT * This);
void ui_deinit(RDPCLIENT * This);
BOOL ui_create_window(RDPCLIENT * This);
void ui_resize_window(RDPCLIENT * This);
void ui_destroy_window(RDPCLIENT * This);
void xwin_toggle_fullscreen(RDPCLIENT * This);
int ui_select(RDPCLIENT * This, SOCKET rdp_socket);
void ui_move_pointer(RDPCLIENT * This, int x, int y);
HBITMAP ui_create_bitmap(RDPCLIENT * This, int width, int height, uint8 * data);
void ui_paint_bitmap(RDPCLIENT * This, int x, int y, int cx, int cy, int width, int height, uint8 * data);
void ui_destroy_bitmap(RDPCLIENT * This, HBITMAP bmp);
HGLYPH ui_create_glyph(RDPCLIENT * This, int width, int height, const uint8 * data);
void ui_destroy_glyph(RDPCLIENT * This, HGLYPH glyph);
HCURSOR ui_create_cursor(RDPCLIENT * This, unsigned int x, unsigned int y, int width, int height, uint8 * andmask,
			 uint8 * xormask);
void ui_set_cursor(RDPCLIENT * This, HCURSOR cursor);
void ui_destroy_cursor(RDPCLIENT * This, HCURSOR cursor);
void ui_set_null_cursor(RDPCLIENT * This);
HCOLOURMAP ui_create_colourmap(RDPCLIENT * This, COLOURMAP * colours);
void ui_destroy_colourmap(RDPCLIENT * This, HCOLOURMAP map);
void ui_set_colourmap(RDPCLIENT * This, HCOLOURMAP map);
void ui_set_clip(RDPCLIENT * This, int x, int y, int cx, int cy);
void ui_reset_clip(RDPCLIENT * This);
void ui_bell(RDPCLIENT * This);
void ui_destblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy);
void ui_patblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour,
	       int fgcolour);
void ui_screenblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void ui_memblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy);
void ui_triblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy,
	       BRUSH * brush, int bgcolour, int fgcolour);
void ui_line(RDPCLIENT * This, uint8 opcode, int startx, int starty, int endx, int endy, PEN * pen);
void ui_rect(RDPCLIENT * This, int x, int y, int cx, int cy, int colour);
void ui_polygon(RDPCLIENT * This, uint8 opcode, uint8 fillmode, POINT * point, int npoints, BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_polyline(RDPCLIENT * This, uint8 opcode, POINT * points, int npoints, PEN * pen);
void ui_ellipse(RDPCLIENT * This, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_draw_glyph(RDPCLIENT * This, int mixmode, int x, int y, int cx, int cy, HGLYPH glyph, int srcx, int srcy,
		   int bgcolour, int fgcolour);
void ui_draw_text(RDPCLIENT * This, uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y, int clipx,
		  int clipy, int clipcx, int clipcy, int boxx, int boxy, int boxcx, int boxcy,
		  BRUSH * brush, int bgcolour, int fgcolour, uint8 * text, uint8 length);
void ui_desktop_save(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy);
void ui_desktop_restore(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy);
void ui_begin_update(RDPCLIENT * This);
void ui_end_update(RDPCLIENT * This);
void ui_seamless_begin(RDPCLIENT * This, BOOL hidden);
void ui_seamless_hide_desktop(RDPCLIENT * This);
void ui_seamless_unhide_desktop(RDPCLIENT * This);
void ui_seamless_toggle(RDPCLIENT * This);
void ui_seamless_create_window(RDPCLIENT * This, unsigned long id, unsigned long group, unsigned long parent,
			       unsigned long flags);
void ui_seamless_destroy_window(RDPCLIENT * This, unsigned long id, unsigned long flags);
void ui_seamless_destroy_group(RDPCLIENT * This, unsigned long id, unsigned long flags);
void ui_seamless_move_window(RDPCLIENT * This, unsigned long id, int x, int y, int width, int height,
			     unsigned long flags);
void ui_seamless_restack_window(RDPCLIENT * This, unsigned long id, unsigned long behind, unsigned long flags);
void ui_seamless_settitle(RDPCLIENT * This, unsigned long id, const char *title, unsigned long flags);
void ui_seamless_setstate(RDPCLIENT * This, unsigned long id, unsigned int state, unsigned long flags);
void ui_seamless_syncbegin(RDPCLIENT * This, unsigned long flags);
void ui_seamless_ack(RDPCLIENT * This, unsigned int serial);
/* lspci.c */
BOOL lspci_init(RDPCLIENT * This);
/* seamless.c */
BOOL seamless_init(RDPCLIENT * This);
unsigned int seamless_send_sync(RDPCLIENT * This);
unsigned int seamless_send_state(RDPCLIENT * This, unsigned long id, unsigned int state, unsigned long flags);
unsigned int seamless_send_position(RDPCLIENT * This, unsigned long id, int x, int y, int width, int height,
				    unsigned long flags);
void seamless_select_timeout(RDPCLIENT * This, struct timeval *tv);
unsigned int seamless_send_zchange(RDPCLIENT * This, unsigned long id, unsigned long below, unsigned long flags);
unsigned int seamless_send_focus(RDPCLIENT * This, unsigned long id, unsigned long flags);

/* events */
BOOL event_pubkey(RDPCLIENT * This, unsigned char * key, unsigned int key_size);
void event_logon(RDPCLIENT * This);
BOOL event_redirect(RDPCLIENT * This, uint32 flags, uint32 server_len, wchar_t * server, uint32 cookie_len, char * cookie, uint32 username_len, wchar_t * username, uint32 domain_len, wchar_t * domain, uint32 password_len, wchar_t * password);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
