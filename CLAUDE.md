# CLAUDE.md — Nerpa Project Rules

## 1. Project overview

Nerpa is a C++ stack-based virtual machine.

The project has two target environments:

1. **Desktop** — a standalone version used for development, debugging and testing.
2. **ESP32** — an embedded version running on an ESP32 board.

The VM core should be shared between both environments whenever possible.

The ESP32 version is intended to:
- Display a menu on a 128x64 OLED screen.
- Read Nerpa programs from an SD card.
- Allow the user to select a program using physical buttons.
- Execute the selected program using the shared VM core.
- Display program output on the OLED and send it to UART.
- Allow programs to be changed on the SD card without reflashing the ESP32.

The project is actively developed. Do not assume that the architecture, implementation or documentation is always up to date. Verify the actual source code before making changes.

## 2. Most important rules

These rules have priority over convenience or speed.

1. **Inspect before modifying.** Read the relevant source files, headers, CMake files and existing documentation before proposing or implementing changes.
2. **Do not guess.** If a function, class, API, build command or hardware configuration is unclear, inspect the project or ask for clarification.
3. **Do not rewrite working code without a reason.** Prefer the smallest change that solves the actual problem.
4. **Do not perform large refactors without approval.** First explain the proposed changes and wait for confirmation.
5. **Preserve existing functionality.** A fix for ESP32 must not silently break Desktop, and a Desktop change must not break ESP32.
6. **Do not hide errors.** Do not silently ignore return values, exceptions, failed initialization or hardware errors.
7. **Do not claim success without verification.** Distinguish between code changes, successful compilation, successful execution and successful physical hardware testing.
8. **Never invent test results.** If a build or test was not run, explicitly say so.
9. **Do not modify unrelated files.** Keep changes focused on the requested task.
10. **Do not delete or overwrite user data, programs or configuration without explicit permission.**

## 3. Required workflow

For every non-trivial task, follow this sequence.

### Step 1 — Understand

- Inspect the relevant files and their dependencies.
- Identify which target is affected: Desktop, ESP32 or shared VM core.
- Check the current implementation before suggesting a replacement.
- Check whether the same functionality already exists elsewhere.

### Step 2 — Explain the plan

Before making substantial changes, briefly report:

- What is currently wrong or missing.
- What the likely cause is.
- Which files need to be changed.
- What exactly will change.
- What could potentially break.
- How the result will be tested.

For a small, local and unambiguous fix, you may proceed without asking for approval.

### Step 3 — Implement

- Make focused changes.
- Preserve existing naming conventions and project architecture.
- Avoid unnecessary abstractions, wrappers, classes and dependencies.
- Do not duplicate functionality that already exists.
- Do not replace an entire file when a small patch is sufficient.
- Keep Desktop-specific and ESP32-specific code separate from the shared VM core.

### Step 4 — Verify

After modifications:

- Build the affected target.
- Build the other target too if shared code or common headers were changed.
- Run relevant tests if available.
- Inspect the final diff.
- Check for accidental changes to unrelated files.

If a test cannot be run, explain exactly why.

### Step 5 — Report

At the end, provide:

1. Root cause or task summary.
2. List of modified files.
3. Explanation of important changes.
4. Build and test results.
5. Remaining problems or unverified assumptions.
6. Any manual steps required from the user.

Do not write "everything works" if you have only verified compilation.

## 4. Architecture rules

The project contains a shared virtual machine and platform-specific code.

Keep these responsibilities separate:

- **VM core:** execution, instructions, stack, values and program representation.
- **Desktop platform:** desktop-specific input/output and host functionality.
- **ESP32 platform:** GPIO, display, SD card, UART and other hardware-specific functionality.
- **Application layer:** menu, user interaction, program selection and coordination of components.

Do not introduce ESP-IDF dependencies into the shared VM core.

Do not introduce Arduino dependencies into the ESP-IDF implementation.

Do not make the VM depend directly on OLED, GPIO, SD card or other hardware drivers.

Platform-specific functionality should be provided through the existing Host abstraction or another established project interface. Inspect the current implementation before changing this design.

## 5. ESP32 hardware configuration

The following pin configuration comes from the original hardware prototype. Preserve it unless the user explicitly requests a change.

### OLED

- Controller: SSD1306
- Resolution: 128x64
- Interface: I2C
- SDA: GPIO 21
- SCL: GPIO 22
- I2C address: 0x3C
- Original Arduino library: U8G2
- Original display constructor: `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`
- Original font: `u8g2_font_6x13_t_cyrillic`

Do not switch the display controller to SH1106 without verifying the actual hardware.

### SD card

SPI configuration:

- CS: GPIO 5
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK: GPIO 18

### Buttons

All buttons use INPUT_PULLUP and are active-low.

- UP: GPIO 13
- DOWN: GPIO 14
- SELECT: GPIO 27
- BACK: GPIO 26

Do not change pin assignments without an explicit reason and user approval.

## 6. U8G2 and ESP-IDF precautions

The project uses U8G2 with ESP-IDF. Be especially careful with callback contexts and C/C++ ABI compatibility.

A previous implementation used `U8X8_WITH_USER_PTR` only in the application component while U8G2 itself was compiled separately without that definition.

This can cause different translation units to see different layouts of `u8x8_t`, resulting in ABI mismatch and memory corruption.

Therefore:

- Never define `U8X8_WITH_USER_PTR` for only one component.
- Never assume that a compile definition applied to the application is also applied to managed components.
- If a macro changes a public structure layout, verify that it is applied consistently to every translation unit that uses that structure.
- Prefer a callback-context solution that does not modify the U8G2 structure layout, such as a carefully managed static instance pointer when only one display instance is used.
- Set callback context before initializing or using the display.
- Check callback buffer boundaries and I2C transfer lengths.
- Preserve the actual I2C error code instead of silently discarding it.
- Do not blindly increase task stack size to hide memory corruption.
- When diagnosing a crash, use the ELF file, backtrace and `addr2line` where possible.

Treat the previous ABI mismatch as a known historical issue, not proof that the current source still contains it. Inspect the current code before deciding what to fix.

## 7. Build and development environment

The user develops on Omarchy (Arch-based Linux).

ESP32 development uses ESP-IDF 6.1 through the user's `esp32.sh` wrapper.

Before building:

- Inspect the repository's CMake files and existing build scripts.
- Check the actual ESP-IDF environment and project configuration.
- Do not assume that a command from a different ESP-IDF version is valid.
- Do not replace the user's build configuration just to make a command work.
- Do not edit generated build files or `sdkconfig` unless the task requires it.

For Desktop builds, inspect the existing build configuration and use the project's established build procedure.

If a build fails, distinguish between:
- A source-code error.
- A configuration error.
- A missing dependency.
- An environment or toolchain error.

Do not randomly modify source code to fix an environment problem.

## 8. Code quality

- Use clear C++ and meaningful names.
- Prefer simple solutions over unnecessary abstractions.
- Follow the existing C++ standard and project conventions.
- Avoid unnecessary dynamic allocation in embedded code.
- Be careful with object lifetimes, pointers, references and callback contexts.
- Check array and buffer boundaries.
- Handle hardware initialization failures explicitly.
- Keep resource ownership clear.
- Do not introduce third-party dependencies without explaining why they are needed.

Comments should explain non-obvious decisions, hardware constraints and tricky logic. Do not add comments that merely repeat what the code already says.

## 9. Debugging rules

When investigating a bug:

1. Reproduce or identify the exact failure point.
2. Read the relevant logs and source code.
3. Form a hypothesis based on evidence.
4. Check the hypothesis against the actual implementation.
5. Change one logical thing at a time when possible.
6. Rebuild and compare the result.
7. Do not mix speculative fixes with unrelated refactoring.

When several possible causes exist, rank them by evidence and explain what would confirm or reject each one.

Do not present a hypothesis as a confirmed root cause.

## 10. Communication

The user is learning and developing the project, so explanations must be understandable and technically accurate.

- Explain important architectural decisions.
- Explain why a change is necessary, not just what was changed.
- Avoid excessive jargon without explanation.
- Do not bury important warnings in long reports.
- If the user's proposed approach has a serious flaw, point it out directly and explain why.
- If requirements are ambiguous and the ambiguity could cause destructive or architectural changes, ask before proceeding.

The goal is not to produce the largest amount of code. The goal is to make Nerpa work reliably while keeping its architecture understandable and maintainable.

## 12. Pending improvements backlog (as of 2026-11-09)

These items were reviewed and agreed as candidates, but NOT yet implemented or approved. They are ideas, not current behavior. Do not implement them without explicit user approval; do not document them as done.

### ESP32 — behavior

1. **Async VM execution with abort.** Currently `App::runProgram()` runs the VM synchronously in the application task; a hanging or long program freezes the UI and BACK does nothing. Proposal: run the VM in a separate FreeRTOS task with a cancellation flag checked in Host operations. Largest change, needs discussion first.
2. **Throttle console redraws.** Every VM `debugPrint` triggers a full 1 KB framebuffer redraw over I2C (`App::printFromHost`). Proposal: redraw at a limited rate or on program completion.
3. **Act on `Buttons::begin()` failure.** Result is only logged; the device starts without working buttons. Show a warning on screen.
4. **SD remount at runtime.** If the card was missing at boot, it stays unusable until restart. Retry `storage_.mount()` when opening the file list while unmounted.
5. **Filter the SD file list.** Currently lists all regular files including hidden/service ones (`System Volume Information`). Filter hidden files and/or by program extension.

### ESP32 — code style

6. **Single `config.h`/`pins.h`** for pins, I2C address, `MAX_LINES`, `LINE_HEIGHT`, buffer sizes. `MAX_LINES = 3` is currently duplicated in `App.cpp` and `Display.cpp`.
7. **Remove transient `std::string` from render paths** (`Display::showStartMenu`/`showFileList`, `"Запуск: " + fileName` in `App::runProgram`): dynamic allocation per line per frame on an MCU. Use `snprintf` into a stack buffer instead.
8. **Type-safe console callback in `Esp32Host`** instead of `void*` + function pointer (small `ConsoleSink` interface). Optional, current code works.
9. **Minor consistency:** `pdMS_TO_TICKS(10)` in `App::loop` → named constant; rename `MAX_LINES` → `VISIBLE_LINES`; add `.clang-format` to the repo root.

### Repository

10. **Root `main.cpp` and `nerpa.ino`** are legacy (scratch file / original Arduino prototype), not part of any build target. Candidate for removal or moving to an archive folder — only with user permission.

## 11. Project documentation

Before making architectural changes, read `docs/ARCHITECTURE.md`.

When modifying component responsibilities, interfaces, dependencies, hardware configuration or runtime flow:
- Update `docs/ARCHITECTURE.md` to reflect the verified implementation.
- Do not document planned functionality as already implemented.
- Do not remove known issues until they have been verified as resolved.
- If source code and documentation disagree, inspect the source, report the discrepancy and update the documentation after verification.