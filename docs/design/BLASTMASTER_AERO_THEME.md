# BlastmasterOS Aero Theme

## Goal

Implement a cohesive, Vista-era Aero-inspired desktop for BlastmasterOS. The target is a close visual recreation of the Windows Vista Aero experience, but the theme's artwork must be created for BlastmasterOS or otherwise be appropriately licensed. Do not copy Microsoft's proprietary binaries, fonts, wallpapers, or extracted theme resources into the repository.

This is a desktop-shell and rendering project, not only a wallpaper/color change. The theme must degrade gracefully on systems or virtual machines where composition is unavailable.

## Current repository findings

- The repository already contains the ReactOS theme engine under `dll/win32/uxtheme`.
- DWM API exports are under `dll/win32/dwmapi`; several composition-related functions are currently stubs, including `DwmEnableComposition` and `DwmExtendFrameIntoClientArea`.
- Theme assets are built from `media/themes/CMakeLists.txt`; current entries include Lautus and Modern. Blackshade and Luna are commented out there.
- Explorer's shell and taskbar implementation lives under `base/shell/explorer`, including Start-menu and appbar code.

These are the primary integration points. A new `.theme` configuration by itself will not provide real Aero Glass while the compositor and non-client rendering path do not support it.

## Visual acceptance target

### 1. Window frames and composition
- Blue-tinted translucent frame treatment with a subtle glass highlight and readable caption text.
- Consistent active/inactive frame states, title-bar buttons, rounded top corners where supported, and soft window shadows.
- Correct hit-testing and accessibility for caption buttons, resizing borders, and system menus.
- Opaque fallback when composition is disabled or unsupported; no invisible controls or unreadable captions.

### DWM integration boundary and default fallback

The current `dwmapi` module is an API shim, not a compositor. It has no composition service, redirected-window surfaces, or frame-composition loop to integrate with yet. Until those pieces exist:

- `DwmIsCompositionEnabled` must report `FALSE`.
- `DwmExtendFrameIntoClientArea` must validate its arguments and return `DWM_E_COMPOSITIONDISABLED` rather than claiming a glass frame was drawn.
- `DwmEnableComposition` must reject unknown actions and must not report that enabling composition succeeded.
- A request to disable composition remains a successful no-op, since composition is already unavailable.
- New user profiles start with `ThemeActive=0`, selecting the built-in Classic/non-themed rendering path. This is the safe default while Aero is unfinished; it does not prevent users from enabling a supported visual theme later.

These are truthful compatibility/fallback behaviors, not the completed Aero renderer. The next DWM milestone must implement the compositor itself and connect redirected surfaces, frame margins, colorization, invalidation/flush, and composition state to it. Only after that path is working should DWM report composition as enabled.

## Caption-button visual specification (Windows 7 Aero with Basic geometry)

The Minimize, Maximize/Restore, and Close buttons should use the compact, square-edged Windows 7 Basic-style layout and glyph proportions, while retaining the Windows 7 Aero palette and state treatment. Do not use Vista's oversized, pill-shaped caption controls.

- **Minimize:** a crisp horizontal glyph centered in a compact rectangular button.
- **Maximize:** a thin, square outline; **Restore:** two overlapping square outlines, with consistent stroke weight and centering.
- **Close:** a clearly legible X glyph inside the wider close-button hit target.
- **Normal:** subtle blue glass gradient/highlight with a restrained border; buttons should visually blend into the Aero frame.
- **Hover:** brighter cyan/blue glass highlight with a clear outline.
- **Pressed:** darker blue inset treatment that visibly reads as pressed.
- **Inactive window:** muted, lower-contrast frame and button colors while retaining readable glyphs.
- **Close hover/pressed:** use the distinct amber-to-red Windows 7 Aero close-button treatment; preserve a high-contrast X.
- Preserve native caption hit-testing and system-command behavior. The visual implementation must not replace the existing `HTMINBUTTON`, `HTMAXBUTTON`, and `HTCLOSE` interactions.
- Support maximized windows, disabled caption buttons, small tool windows, high contrast, and composition-off fallback without clipping or changing button hit boxes.

The existing `dll/win32/uxtheme/nonclient.c` already calculates caption-button hit rectangles and delegates visuals to `DrawThemeBackground`. The preferred implementation is to supply and validate matching theme resources/state artwork through the existing theme pipeline first. Only add custom non-client drawing if the theme format cannot express the required state; avoid globally overriding caption buttons for every installed theme.

## 2. Shell
- Vista-style taskbar proportions and glass treatment, Start orb, notification area, clock, and task buttons.
- Vista-style two-column Start menu with program list, common locations, search entry point where supported, and shutdown/lock actions that map to existing shell functionality.
- Window grouping and previews only where the shell APIs can support them reliably; do not fake working previews with static images.
- Explorer toolbar, navigation pane, breadcrumb/address UI, and selection colors should match the same visual system.

### 3. Controls and typography
- Consistent themed buttons, check boxes, radio buttons, progress bars, tabs, scrollbars, menus, tooltips, and common dialogs.
- Use legally redistributable fonts available to the OS; provide sane font fallback.
- Preserve keyboard navigation, focus indication, high-contrast/accessibility behavior, and readable text at common VM resolutions.

### 4. Branding and assets
- Create original BlastmasterOS Aero-inspired assets: Start orb, wallpaper, icons, frame textures, button states, and shell artwork.
- Include asset sources and licensing notes alongside generated assets.
- Use a coherent blue/black glass palette and keep BlastmasterOS identity in About/system surfaces without replacing core shell functionality.

## Implementation sequence

1. **Baseline and build integration**
   - Build the existing theme and shell targets before changing them.
   - Confirm the current default theme, theme resource packaging, and registry/default-theme selection path.
   - Record a baseline VM screenshot at a known resolution.

2. **Theme resource**
   - Add a dedicated `media/themes/BlastmasterAero` theme target and its resource subdirectory.
   - Add it to `media/themes/CMakeLists.txt` only once its required resources and resource compiler inputs exist.
   - Add a theme configuration selecting the new style, colors, metrics, and wallpaper using supported ReactOS mechanisms.

3. **Core visual style**
   - Implement the blue glass-like non-client frame and control states in the supported theme format.
   - Validate theme parsing and drawing using existing `uxtheme` tests; add regression coverage for any new behavior.

4. **Composition capability**
   - Trace callers and contracts for DWM APIs before changing their behavior.
   - Implement composition incrementally with explicit capability detection and an opaque fallback. Do not report composition enabled until the rendering path actually works.
   - Verify repaint, resize, minimize/restore, multiple windows, remote/VM graphics, and session shutdown.

5. **Explorer shell**
   - Integrate the taskbar, Start menu, and Explorer artwork without changing command behavior or breaking shell startup.
   - Preserve keyboard access, shell extensions, notifications, task switching, and user settings.

6. **Branding and release validation**
   - Set the theme as the BlastmasterOS default only after it passes the smoke-test matrix.
   - Test at 800x600, 1024x768, and 1280x720; test both composition-on and fallback paths.
   - Capture screenshots of desktop, Start menu, Explorer, dialogs, active/inactive windows, and taskbar states.
   - Rebuild the ISO and boot/install it in a disposable VM before calling the theme beta-ready.

## Release gates

- [ ] Theme resources compile and are included in the ISO.
- [ ] Theme loads after a clean install and persists after reboot.
- [ ] Window frames, buttons, menus, and controls render without missing resources.
- [ ] Composition capability is reported accurately and fallback works.
- [ ] Start menu, taskbar, Explorer, notifications, and window management remain functional.
- [ ] Keyboard-only navigation and high-contrast/fallback mode remain usable.
- [ ] No unlicensed Microsoft theme binaries or extracted assets are included.
- [ ] VM screenshots and test results are attached to the change for review.

## Scope note

“1:1” is the visual target, not a claim that unsupported Windows Vista internals already exist in ReactOS. Exact behavior such as live thumbnails, advanced window animations, and full DWM composition must be implemented and verified individually; otherwise it must be marked as a known limitation.
