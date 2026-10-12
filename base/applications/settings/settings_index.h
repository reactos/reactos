/*
 * PROJECT:     ReactOS Settings
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     The searchable index of settings
 *
 * Each entry points at the exact place where a setting lives. Pages are opened
 * with "rundll32.exe shell32.dll,Control_RunDLL <file>,<applet>,<page>"; only
 * access, desk, main, mmsys, sysdm and intl support selecting a page (0-based,
 * except access.cpl which is 1-based). Other applets can only be opened whole.
 *
 * Keywords are the words people actually type. When a search fails to find a
 * setting, the fix is almost always to add a synonym here (and a test case in
 * the host unit test, see test_search.c).
 *
 * TODO: move the strings to lang/ resources so they can be translated.
 */

/* CATEGORY(Id, Name, Description, IconFile, IconIndex) */
#define SETTINGS_CATEGORIES \
    CATEGORY(SYSTEM,          L"System",                L"Display, sound, power, about",                 L"shell32.dll", 15) \
    CATEGORY(DEVICES,         L"Devices",               L"Mouse, keyboard, printers, game controllers",  L"main.cpl", 0) \
    CATEGORY(NETWORK,         L"Network & internet",    L"Connections, proxy, internet options",         L"shell32.dll", 18) \
    CATEGORY(PERSONALIZATION, L"Personalization",       L"Background, colors, themes, taskbar, start",   L"desk.cpl", 0) \
    CATEGORY(APPS,            L"Apps",                  L"Installed apps, get apps, file types",         L"appwiz.cpl", 0) \
    CATEGORY(ACCOUNTS,        L"Accounts",              L"Users, passwords, groups",                     L"usrmgr.cpl", 0) \
    CATEGORY(TIME_LANGUAGE,   L"Time & language",       L"Date, time zone, region, languages, keyboard", L"timedate.cpl", 0) \
    CATEGORY(ACCESSIBILITY,   L"Accessibility",         L"Keyboard, mouse keys, contrast, magnifier",    L"access.cpl", 0) \
    CATEGORY(ADVANCED,        L"Advanced",              L"Folder options, system tools, developers",     L"shell32.dll", 21)

#define RD L"shell32.dll,Control_RunDLL "

/* SETTING(Id, Category, Title, Description, Keywords, Program, Parameters) */
#define SETTINGS_INDEX \
    /* ---------------------------------------------------------------- System */ \
    SETTING(DISPLAY, SYSTEM, L"Display", L"Resolution, color depth, multiple monitors", \
        L"screen, monitor, monitors, resolution, screen resolution, screen size, pixels, color depth, colors, bits, " \
        L"second screen, multiple displays, dual monitor, projector, refresh rate, hz, graphics, video card, display adapter, display driver", \
        L"rundll32.exe", RD L"desk.cpl,,3") \
    SETTING(DPI, SYSTEM, L"Scale", L"Make text, apps and other items bigger (100% - 200%)", \
        L"dpi, scale, scaling, zoom, make text bigger, larger text, font size, high dpi, 125%, 150%, magnify, size of text", \
        L"rundll32.exe", RD L"desk.cpl,,3") \
    SETTING(SOUND, SYSTEM, L"Sound", L"Volume, output and input devices", \
        L"audio, volume, speakers, headphones, microphone, mic, mute, loud, quiet, output device, input device, recording, playback, " \
        L"volume icon, speaker setup", \
        L"rundll32.exe", RD L"mmsys.cpl,,0") \
    SETTING(AUDIO_DEVICES, SYSTEM, L"Audio devices", L"Default playback, recording and MIDI devices", \
        L"default device, playback device, recording device, midi, speakers, microphone, sound card, audio output, audio input", \
        L"rundll32.exe", RD L"mmsys.cpl,,2") \
    SETTING(POWER, SYSTEM, L"Power & sleep", L"Turn off screen, standby, hibernate, lid and power button", \
        L"power, sleep, standby, hibernate, hibernation, battery, screen timeout, turn off monitor, turn off display, power plan, " \
        L"power scheme, lid, power button, shutdown button, energy, laptop", \
        L"rundll32.exe", RD L"powercfg.cpl") \
    SETTING(ABOUT, SYSTEM, L"About", L"Version, processor, memory, license", \
        L"about, version, build, system information, specs, specifications, cpu, processor, ram, memory, uptime, license, " \
        L"which version, os version, reactos version", \
        L"rundll32.exe", RD L"sysdm.cpl,,0") \
    SETTING(COMPUTER_NAME, SYSTEM, L"Computer name", L"Rename this PC, workgroup and domain", \
        L"rename, rename pc, computer name, pc name, hostname, host name, workgroup, domain, join domain, network id, dns suffix", \
        L"rundll32.exe", RD L"sysdm.cpl,,1") \
    SETTING(PERFORMANCE, SYSTEM, L"Performance", L"Visual effects, processor scheduling, virtual memory", \
        L"performance, speed up, faster, visual effects, animations, virtual memory, paging file, page file, pagefile, swap, " \
        L"processor scheduling, background services", \
        L"rundll32.exe", RD L"sysdm.cpl,,-1") \
    SETTING(STARTUP_RECOVERY, SYSTEM, L"Startup and recovery", L"Default operating system, crash dumps, automatic restart", \
        L"boot, boot menu, dual boot, default operating system, startup, recovery, crash, blue screen, bsod, memory dump, minidump, " \
        L"automatically restart, timeout", \
        L"rundll32.exe", RD L"sysdm.cpl,,3") \
    SETTING(ENVIRONMENT, SYSTEM, L"Environment variables", L"PATH and other user and system variables", \
        L"environment variables, env, path, temp, variables, system variables, user variables", \
        L"rundll32.exe", RD L"sysdm.cpl,,3") \
    SETTING(USER_PROFILES, SYSTEM, L"User profiles", L"Copy, delete or change the type of profiles", \
        L"user profiles, roaming profile, local profile, delete profile, copy profile", \
        L"rundll32.exe", RD L"sysdm.cpl,,3") \
    SETTING(DEVICE_MANAGER, SYSTEM, L"Device Manager", L"Hardware, drivers and their properties", \
        L"device manager, devices, hardware, drivers, update driver, devmgmt, unknown device, hardware profiles", \
        L"devmgmt.exe", NULL) \
    SETTING(ADD_HARDWARE, SYSTEM, L"Add hardware", L"Install a device that wasn't detected", \
        L"add hardware, new hardware, install device, install driver, legacy device, hardware wizard", \
        L"rundll32.exe", RD L"hdwwiz.cpl") \
    SETTING(SAFE_REMOVE, SYSTEM, L"Safely remove hardware", L"Stop USB drives and other removable devices", \
        L"eject, safely remove, remove usb, usb drive, unplug, stop device, removable, flash drive", \
        L"rundll32.exe", RD L"hotplug.dll") \
    \
    /* --------------------------------------------------------------- Devices */ \
    SETTING(MOUSE, DEVICES, L"Mouse", L"Primary button, double-click speed, ClickLock", \
        L"mouse, buttons, left handed, left-handed, swap buttons, primary button, right click, double click, double-click speed, " \
        L"clicklock, touchpad, trackpad", \
        L"rundll32.exe", RD L"main.cpl,@0,0") \
    SETTING(MOUSE_POINTER_SPEED, DEVICES, L"Pointer speed and options", L"Speed, precision, trails, show location", \
        L"pointer speed, mouse speed, cursor speed, sensitivity, acceleration, enhance pointer precision, snap to, pointer trails, " \
        L"hide pointer while typing, show location of pointer, ctrl key, find mouse", \
        L"rundll32.exe", RD L"main.cpl,@0,2") \
    SETTING(MOUSE_WHEEL, DEVICES, L"Mouse wheel", L"Lines to scroll at a time", \
        L"scroll, scrolling, wheel, scroll speed, lines per scroll, one screen at a time", \
        L"rundll32.exe", RD L"main.cpl,@0,3") \
    SETTING(KEYBOARD, DEVICES, L"Keyboard", L"Repeat delay, repeat rate, cursor blink rate", \
        L"keyboard, typing, key repeat, repeat delay, repeat rate, cursor blink, blink rate, caret", \
        L"rundll32.exe", RD L"main.cpl,@1,0") \
    SETTING(PRINTERS, DEVICES, L"Printers", L"Add, remove and manage printers", \
        L"printer, printers, print, printing, add printer, default printer, print queue, fax", \
        L"control.exe", L"printers") \
    SETTING(SCANNERS, DEVICES, L"Scanners and cameras", L"Imaging devices", \
        L"scanner, scanners, camera, cameras, webcam, imaging, scan", \
        L"control.exe", L"scannercamera") \
    SETTING(GAME_CONTROLLERS, DEVICES, L"Game controllers", L"Joysticks and gamepads", \
        L"game controller, gamepad, joystick, controller, xbox, steering wheel, flight stick, calibrate", \
        L"rundll32.exe", RD L"joy.cpl") \
    \
    /* ----------------------------------------------------- Network & internet */ \
    SETTING(NETWORK_CONNECTIONS, NETWORK, L"Network connections", L"Ethernet, Wi-Fi, adapters, IP address", \
        L"network, internet, ethernet, wifi, wi-fi, wireless, wlan, lan, adapter, ip, ip address, dhcp, dns, tcp/ip, connection, " \
        L"connect, offline, network adapter", \
        L"rundll32.exe", RD L"ncpa.cpl") \
    SETTING(PROXY, NETWORK, L"Proxy", L"Proxy server and automatic configuration", \
        L"proxy, proxy server, wpad, pac, auto config, automatic configuration, port", \
        L"rundll32.exe", RD L"inetcpl.cpl") \
    SETTING(INTERNET_OPTIONS, NETWORK, L"Internet options", L"Home page, browsing history, security zones, certificates", \
        L"internet options, browser, home page, homepage, browsing history, cookies, cache, temporary internet files, " \
        L"security zones, certificates, publishers", \
        L"rundll32.exe", RD L"inetcpl.cpl") \
    \
    /* -------------------------------------------------------- Personalization */ \
    SETTING(BACKGROUND, PERSONALIZATION, L"Background", L"Desktop picture, color and placement", \
        L"wallpaper, background, desktop background, desktop picture, desktop image, picture, photo, image, stretch, center, " \
        L"tile, fill, fit, solid color, backdrop", \
        L"rundll32.exe", RD L"desk.cpl,,0") \
    SETTING(COLORS, PERSONALIZATION, L"Colors and themes", L"Visual style, light or dark mode, font size", \
        L"theme, themes, visual style, colors, colour, color scheme, dark mode, dark theme, light mode, light theme, night mode, " \
        L"accent color, look, appearance, style, skin, eleven, lautus, modern, classic", \
        L"rundll32.exe", RD L"desk.cpl,,2") \
    SETTING(EFFECTS, PERSONALIZATION, L"Visual effects", L"Font smoothing, shadows, animations, large icons", \
        L"effects, font smoothing, smooth fonts, antialiasing, anti-aliasing, cleartype, menu shadows, shadow, animations, fade, " \
        L"slide, show window contents while dragging, underline, alt key, large icons, flat menus", \
        L"rundll32.exe", RD L"desk.cpl,,2") \
    SETTING(ADVANCED_APPEARANCE, PERSONALIZATION, L"Window colors and fonts", L"Colors, sizes and fonts of each window element", \
        L"title bar, caption, window color, window border, border width, menu font, menu size, scrollbar size, scroll bar width, " \
        L"icon spacing, tooltip, system font, menu font size, title bar font", \
        L"rundll32.exe", RD L"desk.cpl,,2") \
    SETTING(SCREEN_SAVER, PERSONALIZATION, L"Screen saver", L"Choose a screen saver and its timeout", \
        L"screen saver, screensaver, lock screen, idle, password on resume, wait minutes", \
        L"rundll32.exe", RD L"desk.cpl,,1") \
    SETTING(DESKTOP_ICONS, PERSONALIZATION, L"Desktop icons", L"Show or hide My Computer, Recycle Bin and other icons", \
        L"desktop icons, my computer, this pc, recycle bin, my documents, network places, show icons, hide icons, change icon", \
        L"rundll32.exe", RD L"desk.cpl,,0") \
    SETTING(MOUSE_POINTERS, PERSONALIZATION, L"Mouse pointers", L"Pointer scheme, cursor shapes and shadow", \
        L"cursor, cursors, mouse pointer, pointer scheme, cursor scheme, big cursor, large pointer, pointer shadow, arrow", \
        L"rundll32.exe", RD L"main.cpl,@0,1") \
    SETTING(SOUND_SCHEME, PERSONALIZATION, L"Sound scheme", L"Sounds played for system events", \
        L"sound scheme, system sounds, notification sound, startup sound, logon sound, beep, alert sound, event sounds", \
        L"rundll32.exe", RD L"mmsys.cpl,,1") \
    SETTING(TASKBAR, PERSONALIZATION, L"Taskbar", L"Alignment, auto-hide, lock, grouping, small icons", \
        L"taskbar, task bar, center taskbar, align left, left aligned, taskbar alignment, centered, auto hide, autohide, " \
        L"hide taskbar, lock taskbar, group buttons, combine, small icons, quick launch, always on top", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 1") \
    SETTING(START_MENU, PERSONALIZATION, L"Start", L"Start menu style and items", \
        L"start, start menu, start button, classic start menu, recent documents, recent items, pinned", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 1") \
    SETTING(NOTIFICATION_AREA, PERSONALIZATION, L"Notification area", L"Clock, tray icons, show desktop button", \
        L"notification area, system tray, tray, tray icons, hide inactive icons, clock, show seconds, show desktop", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 1") \
    SETTING(FONTS, PERSONALIZATION, L"Fonts", L"View and install fonts", \
        L"fonts, font, install font, typeface, ttf, truetype", \
        L"control.exe", L"fonts") \
    \
    /* ------------------------------------------------------------------- Apps */ \
    SETTING(INSTALLED_APPS, APPS, L"Installed apps", L"Uninstall or change programs", \
        L"uninstall, remove program, remove app, add remove programs, add/remove programs, installed programs, programs and features, " \
        L"apps and features, change program, repair, modify, delete program, software", \
        L"rapps.exe", L"/appwiz") \
    SETTING(GET_APPS, APPS, L"Get apps", L"Download and install software", \
        L"download, install, install apps, get apps, store, app store, applications manager, rapps, software, browser, firefox, " \
        L"package, packages", \
        L"rapps.exe", NULL) \
    SETTING(FILE_TYPES, APPS, L"Default apps and file types", L"Which program opens which kind of file", \
        L"default apps, default programs, open with, file association, file associations, file types, extension, extensions, " \
        L"default browser", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 0") \
    SETTING(STARTUP_APPS, APPS, L"Startup apps", L"Programs and services that start with the system", \
        L"startup, startup apps, startup programs, autostart, autorun, run at startup, boot, msconfig, services", \
        L"msconfig.exe", NULL) \
    \
    /* --------------------------------------------------------------- Accounts */ \
    SETTING(USERS, ACCOUNTS, L"User accounts", L"Add, remove and manage users and passwords", \
        L"user, users, account, accounts, add user, new user, remove user, password, change password, reset password, login, " \
        L"logon, sign in, administrator, admin, groups, auto login", \
        L"rundll32.exe", RD L"usrmgr.cpl") \
    \
    /* -------------------------------------------------------- Time & language */ \
    SETTING(DATE_TIME, TIME_LANGUAGE, L"Date & time", L"Set the date, time and clock", \
        L"date, time, clock, calendar, set time, set date, wrong time, change time", \
        L"rundll32.exe", RD L"timedate.cpl") \
    SETTING(TIME_ZONE, TIME_LANGUAGE, L"Time zone", L"Time zone and daylight saving time", \
        L"time zone, timezone, daylight saving, dst, utc, gmt, summer time", \
        L"rundll32.exe", RD L"timedate.cpl") \
    SETTING(INTERNET_TIME, TIME_LANGUAGE, L"Internet time", L"Synchronize the clock with a time server", \
        L"internet time, ntp, time server, synchronize, sync clock, sync time, automatic time", \
        L"rundll32.exe", RD L"timedate.cpl") \
    SETTING(REGION, TIME_LANGUAGE, L"Region and formats", L"Number, currency, date and time formats, location", \
        L"region, regional, locale, country, location, format, formats, date format, time format, 24 hour, 24-hour, currency, " \
        L"decimal, decimal separator, comma, measurement, metric, imperial, first day of week", \
        L"rundll32.exe", RD L"intl.cpl,,0") \
    SETTING(LANGUAGES, TIME_LANGUAGE, L"Language", L"Text input languages and language support", \
        L"language, languages, display language, input language, east asian, right to left, complex script", \
        L"rundll32.exe", RD L"intl.cpl,,1") \
    SETTING(KEYBOARD_LAYOUT, TIME_LANGUAGE, L"Keyboard layouts", L"Input languages, layouts and switching hotkeys", \
        L"keyboard layout, input method, ime, add keyboard, switch language, language bar, qwerty, azerty, dvorak, " \
        L"caps lock, hotkey, alt shift", \
        L"rundll32.exe", RD L"input.dll") \
    SETTING(NON_UNICODE, TIME_LANGUAGE, L"Language for non-Unicode programs", L"Code page used by older programs", \
        L"non-unicode, non unicode, code page, codepage, garbled text, legacy programs, system locale", \
        L"rundll32.exe", RD L"intl.cpl,,2") \
    \
    /* ---------------------------------------------------------- Accessibility */ \
    SETTING(STICKY_KEYS, ACCESSIBILITY, L"Sticky, filter and toggle keys", L"Make the keyboard easier to use", \
        L"sticky keys, stickykeys, filter keys, filterkeys, toggle keys, togglekeys, bounce keys, slow keys, keyboard accessibility, " \
        L"modifier keys, caps lock sound", \
        L"rundll32.exe", RD L"access.cpl,,1") \
    SETTING(SOUND_SENTRY, ACCESSIBILITY, L"Visual notifications for sound", L"SoundSentry and ShowSounds", \
        L"soundsentry, sound sentry, showsounds, show sounds, flash screen, deaf, hard of hearing, captions, visual alerts", \
        L"rundll32.exe", RD L"access.cpl,,2") \
    SETTING(HIGH_CONTRAST, ACCESSIBILITY, L"High contrast and text cursor", L"High contrast colors, cursor width and blink", \
        L"high contrast, contrast, contrast themes, low vision, cursor width, text cursor, caret width, blinking", \
        L"rundll32.exe", RD L"access.cpl,,3") \
    SETTING(MOUSE_KEYS, ACCESSIBILITY, L"Mouse keys", L"Control the pointer with the numeric keypad", \
        L"mouse keys, mousekeys, numeric keypad, numpad, keyboard mouse, move pointer with keyboard", \
        L"rundll32.exe", RD L"access.cpl,,4") \
    SETTING(ACCESSIBILITY_GENERAL, ACCESSIBILITY, L"Accessibility options", L"Automatic reset, warnings, SerialKey devices", \
        L"accessibility, ease of access, serialkeys, serial keys, turn off after idle, notifications", \
        L"rundll32.exe", RD L"access.cpl,,5") \
    SETTING(MAGNIFIER, ACCESSIBILITY, L"Magnifier", L"Zoom in on part of the screen", \
        L"magnifier, magnify, zoom, zoom in, enlarge, bigger, low vision", \
        L"magnify.exe", NULL) \
    SETTING(ON_SCREEN_KEYBOARD, ACCESSIBILITY, L"On-screen keyboard", L"Type with the mouse", \
        L"on-screen keyboard, onscreen keyboard, osk, virtual keyboard, touch keyboard", \
        L"osk.exe", NULL) \
    SETTING(UTILITY_MANAGER, ACCESSIBILITY, L"Utility Manager", L"Start accessibility tools automatically", \
        L"utility manager, utilman, accessibility tools, start magnifier automatically", \
        L"utilman.exe", NULL) \
    \
    /* --------------------------------------------------------------- Advanced */ \
    SETTING(FOLDER_OPTIONS, ADVANCED, L"Folder options", L"Single or double click, open folders in same window", \
        L"folder options, file explorer options, explorer, single click, double click, open folders, same window, new window", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 0") \
    SETTING(HIDDEN_FILES, ADVANCED, L"Hidden files and file extensions", L"Show hidden files, system files and extensions", \
        L"hidden files, show hidden files, hidden folders, file extensions, show extensions, hide extensions, protected files, " \
        L"system files, full path, folder view", \
        L"rundll32.exe", L"shell32.dll,Options_RunDLL 7") \
    SETTING(CONSOLE, ADVANCED, L"Console defaults", L"Default font, colors and size of command prompt windows", \
        L"console, command prompt, cmd, terminal, console font, console colors, quickedit, buffer size", \
        L"rundll32.exe", RD L"console.dll") \
    SETTING(ADMIN_TOOLS, ADVANCED, L"Administrative tools", L"Services, event viewer, computer management", \
        L"administrative tools, admin tools, services, event viewer, event log, computer management, management console, mmc", \
        L"control.exe", L"admintools") \
    SETTING(TASK_MANAGER, ADVANCED, L"Task Manager", L"Running programs, processes and performance", \
        L"task manager, taskmgr, processes, kill, end task, running programs, cpu usage, memory usage", \
        L"taskmgr.exe", NULL) \
    SETTING(OPENGL, ADVANCED, L"OpenGL", L"Choose the OpenGL renderer", \
        L"opengl, gl, renderer, 3d, graphics driver, software rendering", \
        L"rundll32.exe", RD L"openglcfg.cpl") \
    SETTING(WINED3D, ADVANCED, L"Direct3D (WineD3D)", L"Direct3D options for games", \
        L"direct3d, d3d, directx, wined3d, glsl, games, multisampling, video memory", \
        L"rundll32.exe", RD L"wined3dcfg.cpl") \
    SETTING(CONTROL_PANEL, ADVANCED, L"Control Panel", L"All classic Control Panel applets", \
        L"control panel, classic control panel, applets, cpl, all settings", \
        L"control.exe", NULL)
