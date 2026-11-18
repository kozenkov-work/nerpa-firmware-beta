# Nerpa — Architecture and Code Map

> **Status:** verified against the working tree on 2026-11-18. This document describes what is actually implemented in the source. It is not a specification of planned features.

## 1. Project purpose

Nerpa is a stack-based virtual machine project with a Desktop target and an ESP32 target. The intended embedded workflow is:

1. The ESP32 starts the application.
2. The application initializes buttons and the OLED display.
3. The application attempts to mount the SD card.
4. The user navigates the menu and opens the file list.
5. A selected file is loaded by `Tokenizer` into a `Program`.
6. A `VM` executes that program using the ESP32 implementation of `Host`.
7. Host debug output is sent to UART and forwarded to the application console shown on the OLED.

The flow above is the implemented code path. Not every part has been physically verified on hardware.

## 2. Repository layout

```text
Nerpa/
├── CMakeLists.txt          # Desktop build (CMake, C++17)
├── esp32.sh                # User's ESP-IDF 6.1 build wrapper
├── main.cpp                # Legacy scratch file, not part of any target
├── nerpa.ino               # Original Arduino prototype sketch (reference)
├── vm-contract.md          # Contract between VM programs and Host implementations
├── program_files/          # Sample Nerpa programs for the Desktop target
├── tests/
│   └── vm_tests.cpp        # Desktop VM contract tests (CTest)
├── docs/
│   └── ARCHITECTURE.md     # This document
└── src/                    # Shared VM core + Desktop platform
    ├── main.cpp            # Desktop entry point
    ├── stack.h/.cpp        # Operand stack
    ├── value.h/.cpp        # VM value type
    ├── instruction.h       # OpCode enum and Instruction struct
    ├── program.h           # Program representation
    ├── tokenizer.h/.cpp    # Program text → Program
    ├── vm.h/.cpp           # Execution engine
    ├── host.h              # Host interface (platform abstraction)
    ├── desktop_host.h/.cpp # Desktop implementation of Host
    ├── file_loader.h/.cpp  # File reading helper
    └── esp32/              # ESP32 firmware (ESP-IDF project)
        ├── CMakeLists.txt          # ESP-IDF project root
        ├── main/
        │   ├── CMakeLists.txt      # Component registration, includes shared VM sources
        │   ├── idf_component.yml
        │   └── main.cpp            # app_main()
        ├── app/
        │   ├── App.h/.cpp          # Application state machine and coordination
        ├── display/
        │   ├── Display.h/.cpp      # U8G2 + ESP-IDF I2C OLED driver
        ├── input/
        │   ├── Buttons.h/.cpp      # GPIO buttons with debounce
        ├── storage/
        │   ├── ProgramStorage.h/.cpp # SD card mount and file access
        ├── host/
        │   ├── Esp32Host.h/.cpp    # ESP32 implementation of Host
        └── managed_components/
            └── u8g2/               # U8G2 library (managed component)
```

The ESP32 component CMake file (`src/esp32/main/CMakeLists.txt`) compiles the shared VM sources (`stack.cpp`, `value.cpp`, `tokenizer.cpp`, `vm.cpp`) directly into the firmware component and adds `src/` as an include directory. Platform-specific Desktop files (`main.cpp`, `desktop_host.cpp`, `file_loader.cpp`) are not part of the firmware build.

## 3. Shared VM core (`src/`)

The VM core is platform-independent. It must not include ESP-IDF, Arduino or any hardware headers.

- **`Value`** — a tagged value: `std::variant<double, std::string, bool, std::monostate>` implementing contract `number`, `string`, `bool` and `null`, with checked typed getters.
- **`Stack`** — operand stack over `std::vector<Value>` with contract `MATH` dispatch (numeric, string and boolean operations), comparisons and legacy arithmetic wrappers. Underflow, invalid types and division by zero throw instead of silently leaving the stack unchanged.
- **`Instruction`** — `{ OpCode op; int arg1; double arg2; }`; the floating-point second argument allows fractional `CONST number` values. Contract opcodes: `CONST, POP, DUP, MATH, GET, SET, EQ, LT, GT, JMP, JMPF, CALL, RET, HOST`. Legacy arithmetic and `HALT` remain supported.
- **`Program`** — instruction vector, typed `config` values, typed default-initialized `variables`, string table (`strings`) and source line mapping (`lineToIndex`). Config defines the setup line (ID 0, default 0), required loop line (ID 1) and a boolean parameter (ID 2, default true).
- **`Tokenizer`** — `load(filename)` and `loadFromString(text)` parse sections only in contract order (`*config`, `*variables`, `*strings`, `*code`), validate arguments and duplicate IDs, require a loop line, apply config defaults and verify setup/loop targets. Errors throw `std::exception` (callers handle them).
- **`VM`** — `run(const Program&, Host*)` calls setup once, then invokes loop repeatedly until `Host::shouldStop()` requests cancellation. It uses an operand stack, call frames and a per-run mutable copy of variables. Every function, including setup/loop, must leave exactly one new value on `RET`; entry-function results are discarded. Reaching the end of a function without `RET` is an error. `SET` enforces the declared variable type.
- **`Host`** — abstract platform interface: `debugPrint`, `pinMode`, `digitalWrite`, `digitalRead`, `analogWrite`, `analogRead`, `shouldStop`. Contract Host IDs 0–5 map to debug string output, GPIO and analog operations; ID 6 is a backward-compatible numeric debug extension. `shouldStop()` defaults to `false`; the VM polls it every 32 executed instructions and after each loop call.

The Desktop target (`src/main.cpp`) loads a hardcoded program path (`./program_files/file.txt`) and runs it with `DesktopHost`.

## 4. ESP32 components (`src/esp32/`)

### `main/` — firmware entry point

`main.cpp` defines `app_main()`.

- Normal mode: constructs `App`, calls `App::setup()`, then repeatedly calls `App::loop()`.
- If setup fails, the task remains alive and delays in a loop.
- Diagnostic mode: when `NERPA_DISPLAY_DIAGNOSTIC` is enabled, constructs `Display` and runs the display diagnostic instead of starting `App`.

Keep this file as a thin entry point. Application behavior belongs in `App`; hardware drivers belong in their respective components.

### `app/` — application state and coordination

`App` coordinates the UI, input, storage and VM execution.

Application states: `Start`, `Settings`, `FileList`, `Console`.

Main responsibilities:

- Initialize buttons and display.
- Attempt SD-card mounting.
- Read button events and dispatch them according to the current state.
- Load the SD file list when the user opens the file list.
- Load/tokenize a selected program and execute it.
- Maintain console lines (at most 15) and scroll position; up to 3 lines shown at a time.
- Render the current state through `Display`.

Important implementation facts:

- `runProgram()` executes the VM synchronously in the application task. While a program runs, the normal UI is not processed, but the VM checks the interrupt-driven RESET flag through `Esp32Host` every 32 instructions and after each loop call. RESET stops execution and leaves the console visible; a second RESET or BACK returns to the file list. UP, DOWN, SELECT and BACK remain available to the running program through `digitalRead`.
- Every `debugPrint` from the VM triggers a full screen redraw via the `Esp32Host` callback (`printFromHost` → `printToConsole` + `updateScreen`).
- The Settings state currently only handles the Back event; the screen shows a placeholder.
- SD mounting failure is reported on the display for 2 seconds, but setup continues and returns `true`. There is no remount attempt later; a card inserted after boot is not usable until restart.
- `Buttons::begin()` failure is logged but does not fail setup.

Do not change the execution model or state machine without first explaining the consequences for responsiveness, button handling and output rendering.

### `display/` — OLED driver and rendering

`Display` owns the U8G2 display object and the ESP-IDF I2C bus/device handles.

Responsibilities:

- Configure the I2C master bus (400 kHz) and add the OLED device; probe the address before use.
- Initialize U8G2 using the SSD1306 128x64 I2C setup function.
- Transport U8G2 data over ESP-IDF I2C via byte/gpio-delay callbacks.
- Draw the start menu, settings screen, file list, console and simple messages.
- Optionally run OLED diagnostic patterns in diagnostic mode.

Hardware settings:

| Setting | Value |
|---|---|
| Controller | SSD1306 |
| Resolution | 128x64 |
| Interface | I2C |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| Address | 0x3C |
| Font | `u8g2_font_6x13_t_cyrillic` |

The I2C transport uses a 129-byte transfer buffer with bounds checks; I2C error codes are preserved in `lastError_`. Preserve this when changing the callback.

**Callback context:** U8G2 requires a C-style callback without user data. The implementation uses a static `Display* activeInstance_` set before display initialization. This does not modify U8G2 structure layout and is safe for this single-display design. `U8X8_WITH_USER_PTR` is **not** defined anywhere in the build; do not reintroduce it for only one component (it changes `u8x8_t` layout and previously caused ABI mismatch with the separately compiled U8G2 component).

### `input/` — physical buttons

`Buttons` configures five active-low buttons using GPIO pull-ups. UP, DOWN, SELECT and BACK use polling; RESET uses a falling-edge GPIO interrupt.

| Button | GPIO |
|---|---:|
| UP | 13 |
| DOWN | 14 |
| SELECT | 27 |
| BACK | 26 |
| RESET | 25 |

Polled buttons use a 40 ms debounce interval. `read()` returns an `Events` structure with one-shot press events. The application polls it from `App::loop()` with a 10 ms delay. The RESET ISR only sets a volatile flag; `readReset()` consumes it in task context and applies a 40 ms interval between accepted events. During VM execution, `readReset()` checks only this flag, so the other buttons remain available to the program.

Do not change the active-low behavior, pin assignments or debounce logic without checking the hardware and the application interaction model.

### `storage/` — SD-card mounting and file access

`ProgramStorage` mounts an SD card using the ESP-IDF SDSPI/FATFS path.

| Setting | Value |
|---|---|
| CS | GPIO 5 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK | GPIO 18 |
| Mount point | `/sdcard` |
| Initial SPI frequency | 400 kHz |
| Format on mount failure | Disabled |

Public operations:

- `mount()` — initializes SPI and mounts the SD card. On failure, releases the SPI bus so a later retry is possible.
- `loadFileList()` — enumerates regular files in the mount root and sorts names. There is no extension or hidden-file filter; every regular file is listed.
- `isProgramAvailable()` — checks that a named path is a regular file.
- `programPath()` — creates a path under `/sdcard`.
- `isMounted()` — reports the internal mounted flag.

Do not enable automatic formatting on mount failure unless the user explicitly requests it. Formatting can destroy user data.

### `host/` — ESP32 implementation of VM host operations

`Esp32Host` derives from the shared `Host` interface (`src/host.h`).

- `debugPrint()` writes text to UART using `printf` and forwards it through a callback to `App` (console line + screen redraw).
- `pinMode()` maps VM mode values (0=input, 1=output, 2=input+pullup, 3=input+pulldown) to ESP-IDF GPIO configuration. Unknown modes return -1. See `vm-contract.md`.
- `digitalWrite()` controls a GPIO output level.
- `digitalRead()` reads a GPIO level.
- `analogWrite()` validates a 0–1 value and drives LEDC PWM at 5 kHz with 8-bit resolution. One timer is initialized lazily; GPIOs are assigned to eight channels by `pin % 8`, and a collision between different GPIOs is reported as an error.
- `analogRead()` uses the ESP-IDF ADC oneshot API on ADC1/ADC2, 12-bit resolution and 12 dB attenuation. Raw values are normalized to 0–1 without voltage calibration; initialization and read failures are logged and reported to the VM.
- `shouldStop()` calls back into `App` and reports a debounced RESET press.

`Esp32Host` receives a context pointer plus console and stop-check callbacks in its constructor; the callbacks point back to the owning `App` instance.

Do not put OLED or SD-card implementation details into the shared VM core. Platform-specific operations belong behind the Host interface.

## 5. Main runtime relationships

```text
app_main()
  └── App
      ├── Buttons
      │   └── ESP-IDF GPIO input
      ├── Display
      │   ├── U8G2
      │   └── ESP-IDF I2C master
      ├── ProgramStorage
      │   ├── ESP-IDF SPI/SDSPI
      │   └── FATFS / VFS
      └── Esp32Host
          ├── ESP-IDF GPIO
          └── callback → App console

App::runProgram()
  ├── ProgramStorage::isProgramAvailable()
  ├── ProgramStorage::programPath()
  ├── Tokenizer::load(path) → Program
  └── VM::run(program, &host_)
       └── Host operations → Esp32Host
```

## 6. Shared VM and Desktop boundary

The VM implementation is shared between Desktop and ESP32. Both targets compile the same shared sources; the ESP32 build adds ESP32-specific components, the Desktop build adds `desktop_host.cpp` and `file_loader.cpp`.

Rules for changes:

- Keep platform-independent execution logic in the shared VM core.
- Keep ESP-IDF, GPIO, I2C, SPI, FATFS and U8G2 dependencies out of the shared VM core.
- Use the existing `Host` abstraction for platform operations.
- Before changing shared headers or VM interfaces, inspect both build configurations.
- If a shared interface changes, build both targets where possible.

## 7. Hardware pin reference

| Peripheral | Signal | GPIO |
|---|---|---:|
| OLED I2C | SDA | 21 |
| OLED I2C | SCL | 22 |
| SD SPI | CS | 5 |
| SD SPI | MOSI | 23 |
| SD SPI | MISO | 19 |
| SD SPI | SCK | 18 |
| Buttons | UP | 13 |
| Buttons | DOWN | 14 |
| Buttons | SELECT | 27 |
| Buttons | BACK | 26 |
| Buttons | RESET | 25 |

Treat these as the current expected wiring, not as permission to change it.

## 8. Build configuration notes

**Desktop:** root `CMakeLists.txt`, C++17, `nerpa` executable plus a `vm_tests` CTest target when `BUILD_TESTING` is enabled (the CMake default). Build and test with `cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Warnings: `-Wall -Wextra -pedantic` (GCC/Clang) or `/W4` (MSVC).

**ESP32:** `src/esp32/` is an ESP-IDF project (`IDF_TARGET=esp32`), application component set to C++17. The user develops on Omarchy (Arch-based Linux) with ESP-IDF 6.1 invoked through the local `esp32.sh` wrapper. Inspect the wrapper before suggesting build commands.

The firmware component declares dependencies on: `u8g2`, `fatfs`, `sdmmc`, `esp_driver_gpio`, `esp_driver_i2c`, `esp_driver_spi`, `esp_driver_ledc`, `esp_adc`.

`NERPA_DISPLAY_DIAGNOSTIC` is a CMake option that switches the firmware to an OLED-only diagnostic loop.

Do not edit generated build output as a substitute for editing source or configuration files.

## 9. Known limitations and items to verify

These are observations or verification gaps, not necessarily defects:

1. The ESP32 application executes the VM synchronously inside `App::runProgram()`; normal UI input is unresponsive while a program runs. RESET provides cooperative cancellation through its interrupt flag, but the rest of the menu remains inactive until execution stops.
2. Every VM `debugPrint` triggers a full 1 KB framebuffer redraw over I2C.
3. The Settings screen has no settings actions; it shows a placeholder text.
4. The SD file list includes all regular files in the root directory, including hidden and service files (e.g. `System Volume Information`).
5. `App::setup()` logs the boolean result of `Buttons::begin()` but does not act on it; the device starts without working buttons if initialization fails.
6. SD mounting failure does not fail setup, and there is no remount attempt at runtime.
7. Repository root contains `main.cpp` (scratch file) and `nerpa.ino` (original Arduino prototype) which are not part of either build target.
8. Successful compilation does not prove that OLED, SD, buttons or VM execution work on physical hardware. Hardware verification must be reported separately.

## 10. Rules for AI agents editing this project

- Read this document and the relevant source files before changing code.
- Treat source code and build configuration as authoritative when this document is stale; report discrepancies and update the document only after verifying the implementation.
- Do not silently turn planned behavior into claims about existing behavior.
- Do not perform broad refactors without presenting a plan and receiving approval.
- Prefer the smallest change that fixes the demonstrated problem.
- Preserve the Desktop target and shared VM boundary when changing ESP32 code.
- Never claim physical hardware success based only on a successful build.
- After changes, report modified files, tests/builds actually run, results and anything still unverified.

## 11. Keeping this document current

Update this document when a component's responsibility, public interface, dependency direction, hardware configuration or runtime flow changes.

When updating it:

1. Inspect the implementation and build configuration.
2. Describe only verified behavior.
3. Mark planned or unverified behavior explicitly.
4. Do not remove known limitations until they have been checked and resolved.
5. Keep detailed implementation notes near the relevant source code; this document should remain a map of the whole project.
