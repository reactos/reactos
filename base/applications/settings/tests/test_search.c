/*
 * PROJECT:     ReactOS Settings
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Host unit test: does searching find the right setting?
 *
 * Build and run on any host (not part of the ReactOS build):
 *   gcc -fshort-wchar -I.. -o test_search test_search.c ../search.c && ./test_search
 *
 * Every query here is something a person would plausibly type, including
 * typos, partial words and names from Windows. When a search misses, add the
 * synonym to settings_index.h rather than special-casing the engine.
 */

#include <stdio.h>
#include <string.h>
#include "search.h"
#include "../settings_index.h"

/* Build the index table from the same X-macros the app uses */
enum
{
#define CATEGORY(Id, Name, Desc, IconFile, IconIndex) CAT_##Id,
    SETTINGS_CATEGORIES
#undef CATEGORY
};

static const SCHAR *CategoryNames[] = {
#define CATEGORY(Id, Name, Desc, IconFile, IconIndex) Name,
    SETTINGS_CATEGORIES
#undef CATEGORY
};

enum
{
#define SETTING(Id, Cat, Title, Desc, Keywords, Program, Params) S_##Id,
    SETTINGS_INDEX
#undef SETTING
    S_COUNT
};

static const char *SettingIds[] = {
#define SETTING(Id, Cat, Title, Desc, Keywords, Program, Params) #Id,
    SETTINGS_INDEX
#undef SETTING
};

static SEARCH_ITEM Items[S_COUNT];

static void
BuildItems(void)
{
    int i = 0;
#define SETTING(Id, Cat, T, D, K, Program, Params) \
    Items[i].Title = T; Items[i].Description = D; Items[i].Keywords = K; \
    Items[i].Category = CategoryNames[CAT_##Cat]; i++;
    SETTINGS_INDEX
#undef SETTING
}

typedef struct
{
    const char *Query;
    int Expected;
    int MaxRank;    /* 1 = must be the top result */
} TEST;

static const TEST Tests[] = {
    /* The obvious ones */
    { "wallpaper", S_BACKGROUND, 1 },
    { "background", S_BACKGROUND, 1 },
    { "change my wallpaper", S_BACKGROUND, 1 },
    { "how do I change the desktop background", S_BACKGROUND, 1 },
    { "desktop picture", S_BACKGROUND, 1 },
    { "dark mode", S_COLORS, 1 },
    { "dark theme", S_COLORS, 1 },
    { "theme", S_COLORS, 1 },
    { "resolution", S_DISPLAY, 1 },
    { "screen resolution", S_DISPLAY, 1 },
    { "second monitor", S_DISPLAY, 1 },
    { "refresh rate", S_DISPLAY, 1 },
    { "dpi", S_DPI, 1 },
    { "make text bigger", S_DPI, 2 },
    { "volume", S_SOUND, 1 },
    { "microphone", S_SOUND, 2 },
    { "headphones", S_SOUND, 1 },
    { "mouse speed", S_MOUSE_POINTER_SPEED, 1 },
    { "pointer speed", S_MOUSE_POINTER_SPEED, 1 },
    { "left handed mouse", S_MOUSE, 1 },
    { "double click speed", S_MOUSE, 1 },
    { "scroll speed", S_MOUSE_WHEEL, 1 },
    { "cursor", S_MOUSE_POINTERS, 2 },
    { "big cursor", S_MOUSE_POINTERS, 1 },
    { "keyboard repeat rate", S_KEYBOARD, 1 },
    { "keyboard layout", S_KEYBOARD_LAYOUT, 1 },
    { "azerty", S_KEYBOARD_LAYOUT, 1 },
    { "printer", S_PRINTERS, 1 },
    { "add printer", S_PRINTERS, 1 },
    { "wifi", S_NETWORK_CONNECTIONS, 1 },
    { "wi-fi", S_NETWORK_CONNECTIONS, 1 },
    { "ip address", S_NETWORK_CONNECTIONS, 1 },
    { "dns", S_NETWORK_CONNECTIONS, 1 },
    { "proxy", S_PROXY, 1 },
    { "uninstall", S_INSTALLED_APPS, 1 },
    { "uninstall a program", S_INSTALLED_APPS, 1 },
    { "add remove programs", S_INSTALLED_APPS, 1 },
    { "install software", S_GET_APPS, 2 },
    { "default browser", S_FILE_TYPES, 1 },
    { "open with", S_FILE_TYPES, 1 },
    { "password", S_USERS, 1 },
    { "change password", S_USERS, 1 },
    { "add user", S_USERS, 1 },
    { "time", S_DATE_TIME, 2 },
    { "change the time", S_DATE_TIME, 1 },
    { "clock", S_DATE_TIME, 2 },
    { "time zone", S_TIME_ZONE, 1 },
    { "timezone", S_TIME_ZONE, 1 },
    { "date format", S_REGION, 1 },
    { "24 hour clock", S_REGION, 1 },
    { "currency", S_REGION, 1 },
    { "language", S_LANGUAGES, 1 },
    { "sticky keys", S_STICKY_KEYS, 1 },
    { "high contrast", S_HIGH_CONTRAST, 1 },
    { "magnifier", S_MAGNIFIER, 1 },
    { "zoom in", S_MAGNIFIER, 1 },
    { "on screen keyboard", S_ON_SCREEN_KEYBOARD, 1 },
    { "show hidden files", S_HIDDEN_FILES, 1 },
    { "file extensions", S_HIDDEN_FILES, 1 },
    { "sleep", S_POWER, 1 },
    { "hibernate", S_POWER, 1 },
    { "battery", S_POWER, 1 },
    { "turn off screen", S_POWER, 2 },
    { "rename computer", S_COMPUTER_NAME, 1 },
    { "computer name", S_COMPUTER_NAME, 1 },
    { "about", S_ABOUT, 1 },
    { "ram", S_ABOUT, 1 },
    { "version", S_ABOUT, 1 },
    { "virtual memory", S_PERFORMANCE, 1 },
    { "pagefile", S_PERFORMANCE, 1 },
    { "path", S_ENVIRONMENT, 1 },
    { "environment variables", S_ENVIRONMENT, 1 },
    { "drivers", S_DEVICE_MANAGER, 1 },
    { "device manager", S_DEVICE_MANAGER, 1 },
    { "eject usb", S_SAFE_REMOVE, 1 },
    { "screensaver", S_SCREEN_SAVER, 1 },
    { "screen saver", S_SCREEN_SAVER, 1 },
    { "taskbar", S_TASKBAR, 1 },
    { "align taskbar left", S_TASKBAR, 1 },
    { "center taskbar", S_TASKBAR, 1 },
    { "auto hide taskbar", S_TASKBAR, 1 },
    { "start menu", S_START_MENU, 1 },
    { "system tray", S_NOTIFICATION_AREA, 1 },
    { "fonts", S_FONTS, 1 },
    { "install font", S_FONTS, 1 },
    { "font smoothing", S_EFFECTS, 1 },
    { "cleartype", S_EFFECTS, 1 },
    { "startup programs", S_STARTUP_APPS, 1 },
    { "game controller", S_GAME_CONTROLLERS, 1 },
    { "joystick", S_GAME_CONTROLLERS, 1 },
    { "task manager", S_TASK_MANAGER, 1 },
    { "services", S_ADMIN_TOOLS, 2 },
    { "control panel", S_CONTROL_PANEL, 1 },
    { "command prompt font", S_CONSOLE, 1 },
    { "system sounds", S_SOUND_SCHEME, 1 },
    { "desktop icons", S_DESKTOP_ICONS, 1 },
    { "recycle bin icon", S_DESKTOP_ICONS, 1 },
    { "blue screen", S_STARTUP_RECOVERY, 1 },
    { "dual boot", S_STARTUP_RECOVERY, 1 },

    /* Extra words must not hide the right result, nor drag in near-misses */
    { "how do I uninstall firefox", S_INSTALLED_APPS, 2 },
    { "uninstall firefox", S_INSTALLED_APPS, 2 },

    /* Typos */
    { "walpaper", S_BACKGROUND, 1 },
    { "wallpapr", S_BACKGROUND, 1 },
    { "backgroud", S_BACKGROUND, 1 },
    { "resolutoin", S_DISPLAY, 1 },
    { "keybaord", S_KEYBOARD, 2 },
    { "pasword", S_USERS, 1 },
    { "screensavr", S_SCREEN_SAVER, 1 },
    { "taskbra", S_TASKBAR, 1 },
    { "hiberante", S_POWER, 1 },

    /* As-you-type prefixes */
    { "wall", S_BACKGROUND, 1 },
    { "resol", S_DISPLAY, 1 },
    { "uninst", S_INSTALLED_APPS, 1 },
    { "blueto", -1, 0 },  /* Bluetooth isn't implemented: must return nothing rather than junk */

    /* Upper case and punctuation */
    { "WALLPAPER", S_BACKGROUND, 1 },
    { "Screen Resolution?", S_DISPLAY, 1 },
};

/* Queries whose result list must stay short (no junk near-misses) */
static const struct { const char *Query; int MaxCount; } CountTests[] = {
    { "dark mode", 1 },
    { "uninstall firefox", 2 },
    { "wallpaper", 2 },
};

static void
ToWide(const char *s, SCHAR *out, size_t n)
{
    size_t i;
    for (i = 0; i + 1 < n && s[i]; i++)
        out[i] = (unsigned char)s[i];
    out[i] = 0;
}

static void
PrintTitle(const SCHAR *t)
{
    for (; *t; t++)
        putchar((char)*t);
}

int
main(int argc, char **argv)
{
    size_t t;
    int Failures = 0;
    int Results[16];

    BuildItems();

    /* Interactive mode: ./test_search "some query" */
    if (argc > 1)
    {
        SCHAR Query[256];
        int n, i;
        ToWide(argv[1], Query, 256);
        n = SearchItems(Items, S_COUNT, Query, Results, 16);
        for (i = 0; i < n; i++)
        {
            printf("%2d. ", i + 1);
            PrintTitle(Items[Results[i]].Title);
            printf("  [%s]\n", SettingIds[Results[i]]);
        }
        return 0;
    }

    for (t = 0; t < sizeof(Tests) / sizeof(Tests[0]); t++)
    {
        const TEST *Test = &Tests[t];
        SCHAR Query[256];
        int n, Rank = 0, i;

        ToWide(Test->Query, Query, 256);
        n = SearchItems(Items, S_COUNT, Query, Results, 16);

        if (Test->Expected < 0)
        {
            if (n != 0)
            {
                printf("FAIL  %-36s expected no results, got %d (first: %s)\n", Test->Query, n, SettingIds[Results[0]]);
                Failures++;
            }
            continue;
        }

        for (i = 0; i < n; i++)
        {
            if (Results[i] == Test->Expected)
            {
                Rank = i + 1;
                break;
            }
        }
        if (Rank == 0 || Rank > Test->MaxRank)
        {
            printf("FAIL  %-36s expected %s in top %d, got rank %d; top: %s\n", Test->Query,
                   SettingIds[Test->Expected], Test->MaxRank, Rank, n ? SettingIds[Results[0]] : "(nothing)");
            Failures++;
        }
    }

    for (t = 0; t < sizeof(CountTests) / sizeof(CountTests[0]); t++)
    {
        SCHAR Query[256];
        int n;
        ToWide(CountTests[t].Query, Query, 256);
        n = SearchItems(Items, S_COUNT, Query, Results, 16);
        if (n > CountTests[t].MaxCount)
        {
            printf("FAIL  %-36s expected at most %d results, got %d\n", CountTests[t].Query, CountTests[t].MaxCount, n);
            Failures++;
        }
    }

    printf("%d/%d search tests passed\n",
           (int)(sizeof(Tests) / sizeof(Tests[0]) + sizeof(CountTests) / sizeof(CountTests[0])) - Failures,
           (int)(sizeof(Tests) / sizeof(Tests[0]) + sizeof(CountTests) / sizeof(CountTests[0])));
    return Failures ? 1 : 0;
}
