#ifndef __COLOR_H
#define __COLOR_H

void init_colors (void);
void toggle_color_mode (void);
void configure_colors_string (char *color_string);

extern int hascolors;
extern int use_colors;
extern int disable_colors;

extern int attr_pairs [];
#ifdef HAVE_GNOME
#   define MY_COLOR_PAIR(x) x
#   define PORT_COLOR(co,bw) co
#else
#   ifdef HAVE_SLANG
#       define MY_COLOR_PAIR(x) COLOR_PAIR(x)
#   else
#       define MY_COLOR_PAIR(x) (COLOR_PAIR(x) | attr_pairs [x])
#   endif
#define PORT_COLOR(co,bw) (use_colors?co:bw)
#endif

#define NORMAL_COLOR          (PORT_COLOR (MY_COLOR_PAIR (1), 0))
#define SELECTED_COLOR        (PORT_COLOR (MY_COLOR_PAIR (2),A_REVERSE))
#define MARKED_COLOR          (PORT_COLOR (MY_COLOR_PAIR (3),A_BOLD))
#ifdef HAVE_SLANG
#define MARKED_SELECTED_COLOR (PORT_COLOR (MY_COLOR_PAIR (4),(SLtt_Use_Ansi_Colors ? A_BOLD_REVERSE : A_REVERSE | A_BOLD)))
#else
#define MARKED_SELECTED_COLOR (PORT_COLOR (MY_COLOR_PAIR (4),A_REVERSE | A_BOLD))
#endif
#define ERROR_COLOR           (PORT_COLOR (MY_COLOR_PAIR (5),0))
#define MENU_ENTRY_COLOR      (PORT_COLOR (MY_COLOR_PAIR (6),A_REVERSE))
#define REVERSE_COLOR         (PORT_COLOR (MY_COLOR_PAIR(7),A_REVERSE))
#define Q_SELECTED_COLOR      (PORT_COLOR (SELECTED_COLOR, 0))
#define Q_UNSELECTED_COLOR    REVERSE_COLOR
#define VIEW_UNDERLINED_COLOR (PORT_COLOR (MY_COLOR_PAIR(12),A_UNDERLINE))
#define MENU_SELECTED_COLOR   (PORT_COLOR (MY_COLOR_PAIR(13),A_BOLD))
#define MENU_HOT_COLOR        (PORT_COLOR (MY_COLOR_PAIR(14),0))
#define MENU_HOTSEL_COLOR     (PORT_COLOR (MY_COLOR_PAIR(15),0))

/*
 * This should be selectable independently. Default has to be black background
 * foreground does not matter at all.
 */
#define GAUGE_COLOR        (PORT_COLOR (MY_COLOR_PAIR(21),0))
#define INPUT_COLOR        (PORT_COLOR (MY_COLOR_PAIR(22),0))

#ifdef HAVE_SLANG
#    define DEFAULT_COLOR  (PORT_COLOR (MY_COLOR_PAIR(31),0))
#   else
#     define DEFAULT_COLOR A_NORMAL
#endif
#define HELP_NORMAL_COLOR  (PORT_COLOR (MY_COLOR_PAIR(16),A_REVERSE))
#define HELP_ITALIC_COLOR  (PORT_COLOR (MY_COLOR_PAIR(17),A_REVERSE))
#define HELP_BOLD_COLOR    (PORT_COLOR (MY_COLOR_PAIR(18),A_REVERSE))
#define HELP_LINK_COLOR    (PORT_COLOR (MY_COLOR_PAIR(19),0))
#define HELP_SLINK_COLOR   (PORT_COLOR (MY_COLOR_PAIR(20),A_BOLD))
			   
extern int sel_mark_color  [4];
extern int dialog_colors   [4];
			   
#define COLOR_NORMAL       (PORT_COLOR (MY_COLOR_PAIR (8),A_REVERSE))
#define COLOR_FOCUS        (PORT_COLOR (MY_COLOR_PAIR (9),A_BOLD))
#define COLOR_HOT_NORMAL   (PORT_COLOR (MY_COLOR_PAIR (10),0))
#define COLOR_HOT_FOCUS    (PORT_COLOR (MY_COLOR_PAIR (11),0))
			   
/* Add this to color panel, on BW all pairs are normal */
#define STALLED_COLOR      (PORT_COLOR (MY_COLOR_PAIR (12),0))
			   
#define DIRECTORY_COLOR    (PORT_COLOR (MY_COLOR_PAIR (23),0))
#define EXECUTABLE_COLOR   (PORT_COLOR (MY_COLOR_PAIR (24),0))
#define LINK_COLOR         (PORT_COLOR (MY_COLOR_PAIR (25),0))
#define DEVICE_COLOR       (PORT_COLOR (MY_COLOR_PAIR (26),0))
#define SPECIAL_COLOR      (PORT_COLOR (MY_COLOR_PAIR (27),0))
#define CORE_COLOR         (PORT_COLOR (MY_COLOR_PAIR (28),0))

#endif /* __COLOR_H */

