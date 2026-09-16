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

1. Virtual hardware and emulator window — done
2. Firmware boot entry — foundation in place
3. Kernel initialization — done
4. Framebuffer/display output — done
5. Keypad/input events — done
6. Virtual flash/filesystem — **implemented**
7. Kernel processes/scheduler + syscall boundary — **implemented**
8. Small Java bytecode interpreter — next
9. zevMobile Java API layer
10. Launcher and built-in apps
11. Install/run external Java applications

The kernel now mounts a small fixed-slot filesystem in virtual flash, starts `zinit` as PID 1, advances a cooperative scheduler, and exposes a small syscall interface for uptime, input, filesystem access, and process information. The next major boundary is the JVM: Java bytecode should execute through kernel services rather than being hard-coded into the emulator.
