# zevMobile Architecture

## Initial target

zevMobile is designed as a virtual phone first. The PC emulator provides the hardware model, while the firmware/kernel and Java runtime run as the guest system.

```text
PC emulator
    |
    +-- virtual CPU
    +-- virtual RAM
    +-- virtual flash
    +-- virtual display
    +-- virtual keypad
    +-- virtual audio
           |
        zevMobile
           |
    +------+------+
    |             |
  kernel         JVM
    |             |
    +------ + ----+
           |
       Java APIs
           |
         apps
```

## Development milestones

1. Virtual hardware and emulator window
2. Firmware boot entry
3. Kernel initialization
4. Framebuffer/display output
5. Keypad/input events
6. Virtual flash/filesystem
7. Small Java bytecode interpreter
8. zevMobile Java API layer
9. Launcher and built-in apps
10. Install/run external Java applications

The implementation can evolve as the project grows; these milestones are intentionally small so each stage can be tested independently.
