# BlastmasterOS DWM Compositor Architecture

## Status

**Design / implementation entry point — not an implemented compositor.** This document defines the first safe implementation milestones. Composition must remain disabled until the end-to-end rendering path is operational and tested.

## Goal

Implement a real per-session desktop window compositor for BlastmasterOS, starting with a software composition backend and opaque windows, then adding alpha blending, frame margins, colorization, and optional accelerated presentation. Preserve the existing non-composited window manager as the fallback.

A dwmapi.dll implementation alone is insufficient: application window pixels must be redirected into compositor-readable surfaces, surface updates must be tracked, and the final desktop image must be presented without racing the shell or display driver.

## Existing integration points

- `dll/win32/dwmapi/dwmapi_main.c`: public DWM API compatibility layer. It currently reports composition disabled, and that is correct while no compositor exists.
- `dll/win32/dwmapi/dwmapi.spec`: exported API surface. Internal `Dwmp*` entries are mostly stubs and should not be advertised as working without implementation.
- `win32ss/user/ntuser/window.c` and `win32ss/user/ntuser/winpos.c`: window lifetime, visibility, geometry, and Z-order transitions. These are likely notification/invalidation integration points, but require a careful audit before changes.
- `win32ss/gdi/ntgdi/bitblt.c`: GDI rendering primitives, including alpha blending. This is not itself a window-redirection or presentation mechanism.
- `base/shell/explorer`: shell/taskbar process. Explorer should not own the compositor's lifetime or be used as a fake compositor.
- `dll/win32/uxtheme/nonclient.c` and the theme resource pipeline: themed non-client artwork and fallback behavior.

## Target architecture

### 1. Session compositor process

Create a dedicated per-interactive-session compositor process/service with explicit startup, shutdown, crash recovery, and session isolation. It must not run as an always-on system service that mixes desktops or sessions. The compositor should negotiate capabilities with the window manager/display path and expose a versioned private IPC protocol; do not invent undocumented DWM exports as a substitute for the protocol.

Responsibilities:
- own the composition loop and output surface;
- receive window create/destroy, visibility, geometry, Z-order, opacity, and damage notifications;
- manage per-window redirection surfaces and their lifetimes;
- coalesce invalidations and repaint only damaged regions when feasible;
- handle display resize, mode change, session logoff, and compositor restart;
- report truthful capabilities and transition to the opaque fallback if initialization fails.

### 2. Window redirection contract

Before implementing the renderer, define a kernel/window-manager contract for surface allocation, drawing redirection, synchronization, and damage notifications. Application GDI/User drawing must reach a per-window surface without changing what the application believes its client coordinates or paint lifecycle are.

Required invariants:
- never expose a stale surface after HWND destruction or handle reuse;
- synchronize resize and surface replacement with in-flight paints;
- preserve normal non-client hit testing, input routing, focus, and Z-order;
- prevent one session from reading another session's surfaces;
- cap surface dimensions and total memory, validate all dimensions, and handle allocation failure;
- preserve the existing direct/opaque drawing path when redirection is unavailable.

Do not implement surface capture by periodically calling PrintWindow or copying the visible desktop: that loses occluded content, can deadlock/re-enter applications, and is not a sound foundation for real composition.

### 3. Initial software renderer

The first backend should be a deterministic CPU/software renderer. For each frame:
1. Snapshot a consistent ordered list of visible, non-minimized windows and their surface references.
2. Clear or restore the damaged output region.
3. Composite window surfaces in correct Z-order with clipping and per-pixel alpha where supported.
4. Apply frame margins and colorization only when those capabilities are enabled.
5. Present through a documented display-driver/graphics path, not by drawing a screenshot into an ordinary top-level window.
6. Signal frame completion to DwmFlush callers and wake the loop on damage or state changes.

Use premultiplied alpha consistently. Validate integer overflow, stride, pixel format, clipping, and allocation size. A software renderer should prioritize correctness and opaque fallback before blur, shadows, rounded corners, or animation.

### 4. Public DWM API contracts

Implement APIs only as their backing features become real:
- `DwmIsCompositionEnabled`: true only after the compositor is running and the current desktop is redirected/composited.
- `DwmEnableComposition`: validate action; enable only after successful initialization; disabling must drain frames and return to the direct/opaque path safely.
- `DwmExtendFrameIntoClientArea`: validate HWND and margins, store margins in window composition state, and schedule frame damage.
- `DwmGetColorizationColor`: return the actual configured colorization value and opacity state.
- `DwmFlush`: wait for the compositor's submitted work to reach the documented completion point; do not return immediate success while pretending a frame was presented.
- `DwmSetWindowAttribute` / `DwmGetWindowAttribute`: validate attribute, size, and window, and implement only attributes backed by real state.
- Thumbnail, iconic bitmap, blur-behind, and transport APIs remain unsupported until their own behavior is implemented. Do not return success for no-op behavior that callers reasonably interpret as successful rendering.

API validation must follow the SDK contracts and the compatibility expectations of this ReactOS-derived tree. Add tests for invalid pointers, invalid HWNDs, unsupported attributes, composition-off behavior, and transitions.

### 5. Theme and fallback

The renderer and theme are separate layers. UxTheme supplies frame/control artwork; DWM supplies window surfaces and composition. The first working compositor does not automatically provide Aero Glass. Start with opaque composition and a clear Classic fallback, then add a colorized alpha frame only after the surface pipeline is stable.

### 6. Performance and resource limits

- Bound total redirected surface memory and per-window dimensions.
- Reuse surfaces where possible and avoid full-screen copies for every small invalidation.
- Coalesce bursty damage notifications and avoid unbounded queues.
- Keep UI-thread work out of blocking compositor waits.
- Ensure compositor failure returns the desktop to a usable non-composited state.
- Instrument frame time, missed presents, surface allocation failures, and fallback transitions in debug builds.

## Implementation milestones

### M0 — Baseline and contract
- [ ] Build the current tree and boot a baseline VM.
- [ ] Trace the display presentation path and GDI/User window paint path.
- [ ] Identify safe extension points for surface redirection and damage notifications.
- [ ] Define IPC messages, ownership, locking, session boundaries, and failure recovery.
- [ ] Add a design-level state machine for Disabled, Starting, Enabled, Stopping, and Failed.

### M1 — Opaque software composition
- [ ] Add the session compositor executable and startup/shutdown integration.
- [ ] Implement window-manager notifications and per-window surface lifetime.
- [ ] Redirect window drawing to surfaces and implement a software Z-order renderer.
- [ ] Present the composed output through the actual display path.
- [ ] Test overlap, occlusion, resize, minimize/restore, destruction/recreation, multiple windows, and logoff.
- [ ] Keep `DwmIsCompositionEnabled=FALSE` until all M1 gates pass.

### M2 — API state and alpha
- [ ] Implement composition state transitions and synchronization.
- [ ] Implement frame margins and colorization against real composed pixels.
- [ ] Add alpha correctness, clipping, invalidation, and resize stress tests.
- [ ] Make `DwmFlush` reflect real submitted-frame completion.

### M3 — Aero effects
- [ ] Implement blur/backdrop sampling, frame tint/highlight, shadows, and rounded frame clipping as supported by the renderer.
- [ ] Add optional hardware acceleration only behind capability detection.
- [ ] Measure performance at 1024x768 and 1280x720 on the documented VM profile.
- [ ] Preserve opaque fallback for unsupported adapters and allocation failures.

### M4 — Compatibility and release gate
- [ ] Test common applications, shell/taskbar, menus, dialogs, fullscreen windows, remote sessions where supported, and display mode changes.
- [ ] Verify compositor crash recovery and no black/transparent desktop on failure.
- [ ] Run regression tests and boot/install a disposable VM from a rebuilt ISO.
- [ ] Enable composition by default only after repeatable pass results.

## Immediate next engineering task

Perform the M0 code-path audit first. Do not add a user-mode overlay or claim the DWM is implemented until the tree has a supported way to redirect every relevant window's drawing and present the final frame. The critical unknown is the safest integration point between win32k/GDI surface ownership and the active display driver.
