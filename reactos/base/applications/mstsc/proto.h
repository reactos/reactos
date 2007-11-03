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
void cache_rebuild_bmpcache_linked_list(uint8 id, sint16 * idx, int count);
void cache_bump_bitmap(uint8 id, uint16 idx, int bump);
void cache_evict_bitmap(uint8 id);
RD_HBITMAP cache_get_bitmap(uint8 id, uint16 idx);
void cache_put_bitmap(uint8 id, uint16 idx, RD_HBITMAP bitmap);
void cache_save_state(void);
FONTGLYPH *cache_get_font(uint8 font, uint16 character);
void cache_put_font(uint8 font, uint16 character, uint16 offset, uint16 baseline, uint16 width,
		    uint16 height, RD_HGLYPH pixmap);
DATABLOB *cache_get_text(uint8 cache_id);
void cache_put_text(uint8 cache_id, void *data, int length);
uint8 *cache_get_desktop(uint32 offset, int cx, int cy, int bytes_per_pixel);
void cache_put_desktop(uint32 offset, int cx, int cy, int scanline, int bytes_per_pixel,
		       uint8 * data);
RD_HCURSOR cache_get_cursor(uint16 cache_idx);
void cache_put_cursor(uint16 cache_idx, RD_HCURSOR cursor);
/* channels.c */
VCHANNEL *channel_register(char *name, uint32 flags, void (*callback) (STREAM));
STREAM channel_init(VCHANNEL * channel, uint32 length);
void channel_send(STREAM s, VCHANNEL * channel);
void channel_process(STREAM s, uint16 mcs_channel);
/* cliprdr.c */
void cliprdr_send_simple_native_format_announce(uint32 format);
void cliprdr_send_native_format_announce(uint8 * formats_data, uint32 formats_data_length);
void cliprdr_send_data_request(uint32 format);
void cliprdr_send_data(uint8 * data, uint32 length);
void cliprdr_set_mode(const char *optarg);
BOOL cliprdr_init(void);
/* disk.c */
int disk_enum_devices(uint32 * id, char *optarg);
RD_NTSTATUS disk_query_information(RD_NTHANDLE handle, uint32 info_class, STREAM out);
RD_NTSTATUS disk_set_information(RD_NTHANDLE handle, uint32 info_class, STREAM in, STREAM out);
RD_NTSTATUS disk_check_notify(RD_NTHANDLE handle);
RD_NTSTATUS disk_create_notify(RD_NTHANDLE handle, uint32 info_class);
RD_NTSTATUS disk_query_volume_information(RD_NTHANDLE handle, uint32 info_class, STREAM out);
RD_NTSTATUS disk_query_directory(RD_NTHANDLE handle, uint32 info_class, char *pattern, STREAM out);
/* mppc.c */
int mppc_expand(uint8 * data, uint32 clen, uint8 ctype, uint32 * roff, uint32 * rlen);
/* ewmhints.c */
int get_current_workarea(uint32 * x, uint32 * y, uint32 * width, uint32 * height);
void ewmh_init(void);
/* iso.c */
STREAM iso_init(int length);
void iso_send(STREAM s);
STREAM iso_recv(uint8 * rdpver);
BOOL iso_connect(char *server, char *username);
BOOL iso_reconnect(char *server);
void iso_disconnect(void);
void iso_reset_state(void);
/* licence.c */
void licence_process(STREAM s);
/* mcs.c */
STREAM mcs_init(int length);
void mcs_send_to_channel(STREAM s, uint16 channel);
void mcs_send(STREAM s);
STREAM mcs_recv(uint16 * channel, uint8 * rdpver);
BOOL mcs_connect(char *server, STREAM mcs_data, char *username);
BOOL mcs_reconnect(char *server, STREAM mcs_data);
void mcs_disconnect(void);
void mcs_reset_state(void);
/* orders.c */
void process_orders(STREAM s, uint16 num_orders);
void reset_order_state(void);
/* parallel.c */
int parallel_enum_devices(uint32 * id, char *optarg);
/* printer.c */
int printer_enum_devices(uint32 * id, char *optarg);
/* printercache.c */
int printercache_load_blob(char *printer_name, uint8 ** data);
void printercache_process(STREAM s);
/* pstcache.c */
void pstcache_touch_bitmap(uint8 cache_id, uint16 cache_idx, uint32 stamp);
BOOL pstcache_load_bitmap(uint8 cache_id, uint16 cache_idx);
BOOL pstcache_save_bitmap(uint8 cache_id, uint16 cache_idx, uint8 * key, uint8 width,
			  uint8 height, uint16 length, uint8 * data);
int pstcache_enumerate(uint8 id, HASH_KEY * keylist);
BOOL pstcache_init(uint8 cache_id);
/* rdesktop.c */
int main(int argc, char *argv[]);
void generate_random(uint8 * random);
void *xmalloc(int size);
char *xstrdup(const char *s);
void *xrealloc(void *oldmem, int size);
void xfree(void *mem);
void error(char *format, ...);
void warning(char *format, ...);
void unimpl(char *format, ...);
void hexdump(unsigned char *p, unsigned int len);
char *next_arg(char *src, char needle);
void toupper_str(char *p);
BOOL str_startswith(const char *s, const char *prefix);
BOOL str_handle_lines(const char *input, char **rest, str_handle_lines_t linehandler, void *data);
BOOL subprocess(char *const argv[], str_handle_lines_t linehandler, void *data);
char *l_to_a(long N, int base);
int load_licence(unsigned char **data);
void save_licence(unsigned char *data, int length);
BOOL rd_pstcache_mkdir(void);
int rd_open_file(char *filename);
void rd_close_file(int fd);
int rd_read_file(int fd, void *ptr, int len);
int rd_write_file(int fd, void *ptr, int len);
int rd_lseek_file(int fd, int offset);
BOOL rd_lock_file(int fd, int start, int len);
/* rdp5.c */
void rdp5_process(STREAM s);
/* rdp.c */
void rdp_out_unistr(STREAM s, char *string, int len);
int rdp_in_unistr(STREAM s, char *string, int uni_len);
void rdp_send_input(uint32 time, uint16 message_type, uint16 device_flags, uint16 param1,
		    uint16 param2);
void rdp_send_client_window_status(int status);
void process_colour_pointer_pdu(STREAM s);
void process_cached_pointer_pdu(STREAM s);
void process_system_pointer_pdu(STREAM s);
void process_bitmap_updates(STREAM s);
void process_palette(STREAM s);
void process_disconnect_pdu(STREAM s, uint32 * ext_disc_reason);
void rdp_main_loop(BOOL * deactivated, uint32 * ext_disc_reason);
BOOL rdp_loop(BOOL * deactivated, uint32 * ext_disc_reason);
BOOL rdp_connect(char *server, uint32 flags, char *domain, char *password, char *command,
		 char *directory);
BOOL rdp_reconnect(char *server, uint32 flags, char *domain, char *password, char *command,
		   char *directory, char *cookie);
void rdp_reset_state(void);
void rdp_disconnect(void);
/* rdpdr.c */
int get_device_index(RD_NTHANDLE handle);
void convert_to_unix_filename(char *filename);
BOOL rdpdr_init(void);
void rdpdr_add_fds(int *n, fd_set * rfds, fd_set * wfds, struct timeval *tv, BOOL * timeout);
struct async_iorequest *rdpdr_remove_iorequest(struct async_iorequest *prev,
					       struct async_iorequest *iorq);
void rdpdr_check_fds(fd_set * rfds, fd_set * wfds, BOOL timed_out);
BOOL rdpdr_abort_io(uint32 fd, uint32 major, RD_NTSTATUS status);
/* rdpsnd.c */
void rdpsnd_send_completion(uint16 tick, uint8 packet_index);
BOOL rdpsnd_init(void);
/* rdpsnd_oss.c */
BOOL wave_out_open(void);
void wave_out_close(void);
BOOL wave_out_format_supported(WAVEFORMATEX * pwfx);
BOOL wave_out_set_format(WAVEFORMATEX * pwfx);
void wave_out_volume(uint16 left, uint16 right);
void wave_out_write(STREAM s, uint16 tick, uint8 index);
void wave_out_play(void);
/* secure.c */
void sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt);
void sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2);
void buf_out_uint32(uint8 * buffer, uint32 value);
void sec_sign(uint8 * signature, int siglen, uint8 * session_key, int keylen, uint8 * data,
	      int datalen);
void sec_decrypt(uint8 * data, int length);
STREAM sec_init(uint32 flags, int maxlen);
void sec_send_to_channel(STREAM s, uint32 flags, uint16 channel);
void sec_send(STREAM s, uint32 flags);
void sec_process_mcs_data(STREAM s);
STREAM sec_recv(uint8 * rdpver);
BOOL sec_connect(char *server, char *username);
BOOL sec_reconnect(char *server);
void sec_disconnect(void);
void sec_reset_state(void);
/* serial.c */
int serial_enum_devices(uint32 * id, char *optarg);
BOOL serial_get_event(RD_NTHANDLE handle, uint32 * result);
BOOL serial_get_timeout(RD_NTHANDLE handle, uint32 length, uint32 * timeout, uint32 * itv_timeout);
/* tcp.c */
STREAM tcp_init(uint32 maxlen);
void tcp_send(STREAM s);
STREAM tcp_recv(STREAM s, uint32 length);
BOOL tcp_connect(char *server);
void tcp_disconnect(void);
char *tcp_get_address(void);
void tcp_reset_state(void);
/* xclip.c */
void ui_clip_format_announce(uint8 * data, uint32 length);
void ui_clip_handle_data(uint8 * data, uint32 length);
void ui_clip_request_failed(void);
void ui_clip_request_data(uint32 format);
void ui_clip_sync(void);
void ui_clip_set_mode(const char *optarg);
void xclip_init(void);
/* xkeymap.c */
BOOL xkeymap_from_locale(const char *locale);
FILE *xkeymap_open(const char *filename);
void xkeymap_init(void);
BOOL handle_special_keys(uint32 keysym, unsigned int state, uint32 ev_time, BOOL pressed);
key_translation xkeymap_translate_key(uint32 keysym, unsigned int keycode, unsigned int state);
void xkeymap_send_keys(uint32 keysym, unsigned int keycode, unsigned int state, uint32 ev_time,
		       BOOL pressed, uint8 nesting);
uint16 xkeymap_translate_button(unsigned int button);
char *get_ksname(uint32 keysym);
void save_remote_modifiers(uint8 scancode);
void restore_remote_modifiers(uint32 ev_time, uint8 scancode);
void ensure_remote_modifiers(uint32 ev_time, key_translation tr);
unsigned int read_keyboard_state(void);
uint16 ui_get_numlock_state(unsigned int state);
void reset_modifier_keys(void);
void rdp_send_scancode(uint32 time, uint16 flags, uint8 scancode);
/* xwin.c */
BOOL get_key_state(unsigned int state, uint32 keysym);
BOOL ui_init(void);
void ui_deinit(void);
BOOL ui_create_window(void);
void ui_resize_window(void);
void ui_destroy_window(void);
void xwin_toggle_fullscreen(void);
int ui_select(int rdp_socket);
void ui_move_pointer(int x, int y);
RD_HBITMAP ui_create_bitmap(int width, int height, uint8 * data);
void ui_paint_bitmap(int x, int y, int cx, int cy, int width, int height, uint8 * data);
void ui_destroy_bitmap(RD_HBITMAP bmp);
RD_HGLYPH ui_create_glyph(int width, int height, uint8 * data);
void ui_destroy_glyph(RD_HGLYPH glyph);
RD_HCURSOR ui_create_cursor(unsigned int x, unsigned int y, int width, int height, uint8 * andmask,
			 uint8 * xormask);
void ui_set_cursor(RD_HCURSOR cursor);
void ui_destroy_cursor(RD_HCURSOR cursor);
void ui_set_null_cursor(void);
RD_HCOLOURMAP ui_create_colourmap(COLOURMAP * colours);
void ui_destroy_colourmap(RD_HCOLOURMAP map);
void ui_set_colourmap(RD_HCOLOURMAP map);
void ui_set_clip(int x, int y, int cx, int cy);
void ui_reset_clip(void);
void ui_bell(void);
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy);
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour,
	       int fgcolour);
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy);
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy,
	       BRUSH * brush, int bgcolour, int fgcolour);
void ui_line(uint8 opcode, int startx, int starty, int endx, int endy, PEN * pen);
void ui_rect(int x, int y, int cx, int cy, int colour);
void ui_polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints, BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_polyline(uint8 opcode, POINT * points, int npoints, PEN * pen);
void ui_ellipse(uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_draw_glyph(int mixmode, int x, int y, int cx, int cy, RD_HGLYPH glyph, int srcx, int srcy,
		   int bgcolour, int fgcolour);
void ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y, int clipx,
		  int clipy, int clipcx, int clipcy, int boxx, int boxy, int boxcx, int boxcy,
		  BRUSH * brush, int bgcolour, int fgcolour, uint8 * text, uint8 length);
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy);
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy);
void ui_begin_update(void);
void ui_end_update(void);
void ui_seamless_begin(BOOL hidden);
void ui_seamless_hide_desktop(void);
void ui_seamless_unhide_desktop(void);
void ui_seamless_toggle(void);
void ui_seamless_create_window(unsigned long id, unsigned long group, unsigned long parent,
			       unsigned long flags);
void ui_seamless_destroy_window(unsigned long id, unsigned long flags);
void ui_seamless_move_window(unsigned long id, int x, int y, int width, int height,
			     unsigned long flags);
void ui_seamless_restack_window(unsigned long id, unsigned long behind, unsigned long flags);
void ui_seamless_settitle(unsigned long id, const char *title, unsigned long flags);
void ui_seamless_setstate(unsigned long id, unsigned int state, unsigned long flags);
void ui_seamless_syncbegin(unsigned long flags);
void ui_seamless_ack(unsigned int serial);
/* lspci.c */
BOOL lspci_init(void);
/* seamless.c */
BOOL seamless_init(void);
unsigned int seamless_send_sync(void);
unsigned int seamless_send_state(unsigned long id, unsigned int state, unsigned long flags);
unsigned int seamless_send_position(unsigned long id, int x, int y, int width, int height,
				    unsigned long flags);
void seamless_select_timeout(struct timeval *tv);
unsigned int seamless_send_zchange(unsigned long id, unsigned long below, unsigned long flags);
unsigned int seamless_send_focus(unsigned long id, unsigned long flags);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
