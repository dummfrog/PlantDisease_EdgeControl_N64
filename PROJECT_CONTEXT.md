# STM32N647 PlantDisease Edge Control Project Context

## Project

Project name: STM32N647 端云协同病虫害视觉诊断与闭环干预系统。

Goal: build the STM32N647 edge-control side of an embedded design competition project. The edge device should eventually collect environment data, receive or simulate AI disease recognition results, apply prescription rules, control pump/fan through relays, alarm through a buzzer, display state on LCD, and produce JSON logs for later L610 cloud upload.

Current repository path:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647
```

Actual STM32CubeIDE Appli project path:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647\STM32CubeIDE\Appli
```

Correct STM32CubeIDE build target:

```text
01_LED_Appli
```

Current Git branch:

```text
codex/buzzer
```

Recent commits:

```text
0d5662b test: verify UART heartbeat on N647
2b84681 feat(uart): add USART printf support for N647 project
d1fc5c9 chore: add CubeMX project metadata
1115261 feat(app): add app_main framework based on N647 LED template
```

## Current Baseline

Current verified state:

- The project has built with 0 errors in STM32CubeIDE.
- The board can be programmed and debugged with STM32CubeProgrammer / ST-LINK / Development boot flow.
- The original LED example behavior is preserved: the board LEDs still alternate.
- Application framework is present:
  - `Appli/Core/Inc/app_main.h`
  - `Appli/Core/Src/app_main.c`
- UART printf support is present:
  - `Appli/Core/Inc/app_uart.h`
  - `Appli/Core/Src/app_uart.c`
- `App_Init()` prints:

```text
[BOOT] PlantDisease Edge Control Start
```

- `App_Loop()` prints:

```text
[APP] heartbeat
```

- Official `05_Serial` has been verified on the board and proves USART1 PE5/PE6 can output to the serial assistant.

## Current Application Structure

The physical source layout is:

```text
Appli/Core/Inc
Appli/Core/Src
```

Inside STM32CubeIDE, these are linked under:

```text
Application/User/Core
```

Current application files:

- `app_main.h`: declares `App_Init()` and `App_Loop()`.
- `app_main.c`: initializes LED, initializes UART printf, prints boot message, then handles heartbeat and LED blinking.
- `app_uart.h`: declares `App_UART_Init(uint32_t baudrate)`.
- `app_uart.c`: implements USART1 initialization and `__io_putchar()` printf redirection.
- `main.c`: only calls the application layer from USER CODE regions.

Important `main.c` integration points:

- `USER CODE BEGIN Includes`: includes `app_main.h`.
- `USER CODE BEGIN 2`: calls `App_Init()`.
- `USER CODE BEGIN 3`: calls `App_Loop()`.
- `USER CODE BEGIN RIF_Init 1`: configures PE5/PE6 RIF attributes for USART1.

## UART Configuration

The current UART printf configuration follows official `05_Serial`:

- UART instance: `USART1`
- TX pin: `PE5`
- RX pin: `PE6`
- Alternate function: `GPIO_AF7_USART1`
- Baud rate: `115200`
- Format: `8N1`
- Hardware flow control: none
- Clock source: `RCC_USART1CLKSOURCE_CLKP`
- UART HAL module: `HAL_UART_MODULE_ENABLED`
- Transmit path: `printf()` -> `__io_putchar()` -> `HAL_UART_Transmit()`

Current caveat: `__io_putchar()` uses blocking `HAL_UART_Transmit()` with `HAL_MAX_DELAY`. This is acceptable for short boot and heartbeat logs, but later JSON logs or heavy debug output should move to a bounded timeout or buffered logger.

## Things Not To Do Now

Do not do these unless a future task explicitly requires and explains the impact:

- Do not modify `.ioc`.
- Do not enable or restructure Non-Secure.
- Do not modify startup files.
- Do not modify linker scripts.
- Do not modify FSBL.
- Do not modify TrustZone / Secure / Non-Secure configuration.
- Do not modify external storage configuration.
- Do not rush into LCD, AI model, camera, or L610.
- Do not rewrite the generated STM32CubeMX structure.
- Do not remove the LED behavior.
- Do not add broad refactors.

## Development Rules

- Keep `main.c` as a thin generated-code entry point.
- Only modify `main.c` inside USER CODE regions.
- Put new application modules under `Appli/Core/Inc` and `Appli/Core/Src` so they appear in CubeIDE as `Application/User/Core`.
- Add one small feature at a time.
- Read official examples and current project code before naming HAL handles, GPIOs, or peripheral instances.
- If a pin, active level, HAL handle, or external module wiring is uncertain, do not guess. Search the project or ask the user.
- Build in STM32CubeIDE after each code change before moving on.
- Do not commit `Debug`, `Release`, `elf`, `hex`, `bin`, `o`, `d`, `map`, or other build outputs.

## Next Task

Next recommended branch:

```text
codex/buzzer
```

The current repository is already on `codex/buzzer`. If another Codex session starts elsewhere, confirm the branch before editing.

Next implementation target: board buzzer module.

Planned steps:

1. Search current project and official examples before coding.
2. Check official `02_Buzzer` and/or board schematic to confirm the buzzer GPIO and active level.
3. Add:
   - `Appli/Core/Inc/buzzer.h`
   - `Appli/Core/Src/buzzer.c`
4. Keep UART heartbeat working.
5. Keep LED blinking behavior unchanged.
6. Make the buzzer short-beep once every 5 seconds for the first validation.
7. Build in STM32CubeIDE and verify 0 errors.
8. Download/debug and verify:
   - boot message prints
   - heartbeat prints
   - LED still alternates
   - buzzer beeps at the expected interval

Do not implement relay, pump, fan, LCD, AI, sensors, or L610 in the buzzer task.

## Later Development Order

Recommended order after buzzer:

1. `board_config.h` for central pin and active-level definitions.
2. `relay.c` / `relay.h` for external relay channels.
3. `pump.c` / `pump.h` wrapping relay channel for pump.
4. `fan.c` / `fan.h` wrapping relay channel for fan.
5. `ai_result.c` / `ai_result.h` for mock AI result.
6. `prescription.c` / `prescription.h` for expert prescription rules.
7. Minimal closed-loop demo using mock AI result.
8. Sensors.
9. LCD UI.
10. JSON log generation.
11. L610 upload.

## Hardware Notes

Before implementing relay, pump, fan, or buzzer control, confirm:

- Exact STM32 GPIO port/pin.
- Active-high or active-low behavior.
- Whether the module needs external power.
- Whether the external module and STM32 board share ground.
- Load voltage and current.
- Whether the pin conflicts with LED, USART1 PE5/PE6, XSPI, LCD, camera, I2C, ADC, or future L610 pins.

Pump and fan must not be driven directly by STM32 GPIO. Use relay or MOS driver hardware and verify power stability.
