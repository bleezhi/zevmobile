# zevMobile PC emulator

The first emulator target is Windows using the Win32 API directly, so the GUI has no SDL dependency.

## Virtual hardware

The emulator currently exposes:

- 64 KiB virtual RAM
- 256 KiB virtual flash
- 240x320 32-bit framebuffer
- virtual keypad: Up, Down, Left, Right, Enter, Escape
- a virtual tick counter

The hardware lives in `emulator/phone.c` and is intentionally independent from the GUI. This lets us replace the host frontend later without changing the guest-side hardware model.

## Windows build

With MinGW-w64 installed:

```text
cd emulator
mingw32-make windows
```

This produces `zevmobile.exe`.

A native Visual Studio build can also compile `main.c`, `phone.c`, and `../kernel/kernel.c` as one Windows GUI executable and link against `user32.lib` and `gdi32.lib`.

## Controls

- Arrow keys -> directional keypad
- Enter -> OK
- Escape -> Back

The display is currently the virtual framebuffer. The kernel will start drawing the real boot/UI contents into it as the next milestones are implemented.
