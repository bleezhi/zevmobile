# zevMobile

A small Java-oriented mobile operating system and PC emulator.

## Goals

- Build a tiny phone-style operating system from scratch.
- Run Java bytecode through a deliberately small JVM/runtime.
- Provide a simple phone UI with display and keypad/input support.
- Emulate the virtual phone on a desktop PC.
- Keep the project understandable and hackable.

## Project layout

```text
zevMobile/
├── boot/       # Boot and firmware code
├── kernel/     # Core OS functionality
├── jvm/        # Java bytecode runtime
├── lib/        # zevMobile Java APIs
├── apps/       # Built-in applications
├── emulator/   # PC emulator
├── tools/      # Build/development tools
└── docs/       # Design documentation
```

## Status

🚧 Early development.

The first milestone is a minimal virtual phone that boots in the PC emulator and displays a basic zevMobile screen.
