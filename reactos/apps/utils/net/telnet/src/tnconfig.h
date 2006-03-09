// Tnconfig.h
// Written by Paul Brannan <pbranna@clemson.edu>
//
// This is a class designed for use with Brad Johnson's Console Telnet
// It reads an ini file and keeps the settings for later retrieval.
// It does not store any information about the current settings, only default
// or recommended settings.

#ifndef __TNCONFIG_H
#define __TNCONFIG_H

// Ioannou 2 June 98:  Borland needs them - quick hack
#ifdef __BORLANDC__
#define bool BOOL
#define true TRUE
#define false FALSE
#endif //  __BORLANDC__

#include "tnerror.h"

#define ENV_TELNET_CFG "TELNET_CFG"
#define ENV_TELNET_REDIR "TELNET_REDIR"
#define ENV_INPUT_REDIR "TELNET_INPUT_REDIR"
#define ENV_OUTPUT_REDIR "TENLET_OUTPUT_REDIR"
#define ENV_TELNET_INI "TELNET_INI"

class TConfig {
public:
	TConfig();
	~TConfig();

	// Miscellaneous strings
	const char *get_startdir() const {return startdir;}
	const char *get_exename() const {return exename;}
	const char *get_keyfile() const {return keyfile;}
	const char *get_inifile() const {return inifile;}
	const char *get_dumpfile() const {return dumpfile;}
	const char *get_term() const {return term;}
	const char *get_printer_name() const {return printer_name;}
	const char *get_default_config() const {return default_config;}

	// Terminal settings
	int get_input_redir() const {return input_redir;}
	int get_output_redir() const {return output_redir;}
	bool get_strip_redir() const {return strip_redir;}
	bool get_dstrbksp() const {return dstrbksp;}
	bool get_eightbit_ansi() const {return eightbit_ansi;}
	bool get_vt100_mode() const {return vt100_mode;}
	bool get_disable_break() const {return disable_break;}
	bool get_speaker_beep() const {return speaker_beep;}
	bool get_do_beep() const {return do_beep;}
	bool get_preserve_colors() const {return preserve_colors;}
	bool get_wrapline() const {return wrapline;}
	bool get_fast_write() const {return fast_write;}
	bool get_lock_linewrap() const {return lock_linewrap;}
	bool get_set_title() const { return set_title;}
	int get_term_width() const {return term_width;}
	int get_term_height() const {return term_height;}
	int get_window_width() const {return window_width;}
	int get_window_height() const {return window_height;}
	bool get_wide_enable() const {return wide_enable;}
	bool get_control_break_as_c() const {return ctrlbreak_as_ctrlc;}
	int get_buffer_size() const {return buffer_size;}

	// Colors
	int get_blink_bg() const {return blink_bg;}
	int get_blink_fg() const {return blink_fg;}
	int get_underline_bg() const {return underline_bg;}
	int get_underline_fg() const {return underline_fg;}
	int get_ulblink_bg() const {return ulblink_bg;}
	int get_ulblink_fg() const {return ulblink_fg;}
	int get_normal_bg() const {return normal_bg;}
	int get_normal_fg() const {return normal_fg;}
	int get_scroll_bg() const {return scroll_bg;}
	int get_scroll_fg() const {return scroll_fg;}
	int get_status_bg() const {return status_bg;}
	int get_status_fg() const {return status_fg;}

	// Mouse
	bool get_enable_mouse() const {return enable_mouse;}

	// Keyboard
	char get_escape_key() const {return escape_key[0];}
	char get_scrollback_key() const {return scrollback_key[0];}
	char get_dial_key() const {return dial_key[0];}
	bool get_alt_erase() const {return alt_erase;}
	bool get_keyboard_paste() const {return keyboard_paste;}

	// Scrollback
	const char *get_scroll_mode() const {return scroll_mode;}
	bool get_scroll_enable() const {return scroll_enable;}
	int get_scroll_size() const {return scroll_size;}

	// Scripting
	const char *get_scriptname() const {return scriptname;}
	bool get_script_enable() const {return script_enable;}

	// Pipes
	const char *get_netpipe() const {return netpipe;}
	const char *get_iopipe() const {return iopipe;}

	// Host configuration
	const char *get_host() const {return host;}
	const char *get_port() const {return port;}

	// Initialization
	void init(char *dirname, char *exename);
	bool Process_Params(int argc, char *argv[]);

	// Ini variables
	void print_vars();
	void print_vars(char *s);
	void print_groups();
	bool set_value(const char *var, const char *value);
	int print_value(const char *var);

	// Aliases
	void print_aliases();
	bool find_alias(const char *alias_name);

private:

	void inifile_init();
	void keyfile_init();
	void redir_init();
	void init_varlist();
	void init_vars();
	void init_aliases();
	void set_string(char *dest, const char *src, const int length);
	void set_bool(bool *boolval, const char *str);

	// Miscellaneous strings
	char startdir[MAX_PATH];
	char exename[MAX_PATH];
	char keyfile[MAX_PATH*2];
	char inifile[MAX_PATH*2];
	char dumpfile[MAX_PATH*2];
	char printer_name[MAX_PATH*2];
	char term[128];
	char default_config[128];

	// Terminal
	int input_redir, output_redir;
	bool strip_redir;
	bool dstrbksp;
	bool eightbit_ansi;
	bool vt100_mode;
	bool disable_break;
	bool speaker_beep;
	bool do_beep;
	bool preserve_colors;
	bool wrapline;
	bool lock_linewrap;
	bool fast_write;
	bool set_title;
	int  term_width, term_height;
	int  window_width, window_height;
	bool wide_enable;
	bool ctrlbreak_as_ctrlc;
	int  buffer_size;

	// Colors
	int blink_bg;
	int blink_fg;
	int underline_bg;
	int underline_fg;
	int ulblink_bg;
	int ulblink_fg;
	int normal_bg;
	int normal_fg;
	int scroll_bg;
	int scroll_fg;
	int status_bg;
	int status_fg;

	// Mouse
	bool enable_mouse;

	// Keyboard
	char escape_key[2];
	char scrollback_key[2];
	char dial_key[2];
	bool alt_erase;
	bool keyboard_paste;

	// Scrollback
	char scroll_mode[8];
	bool scroll_enable;
	int scroll_size;

	// Scripting
	char scriptname[MAX_PATH*2];
	bool script_enable;

	// Pipes
	char netpipe[MAX_PATH*2];
	char iopipe[MAX_PATH*2];

	// Host configration
	char host[128];
	char *port;

	// Aliases
	char **aliases;
	int alias_total;

};

extern TConfig ini;

#endif
