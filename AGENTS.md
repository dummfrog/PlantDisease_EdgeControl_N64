# Codex Instructions for PlantDisease_EdgeControl_N647

## Project Identity

This repository is the STM32N647 edge-control project for:

```text
STM32N647 端云协同病虫害视觉诊断与闭环干预系统
```

Current project root:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647
```

Actual STM32CubeIDE project:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647\STM32CubeIDE\Appli
```

Correct STM32CubeIDE build target:

```text
01_LED_Appli
```

## Current Baseline

The current baseline is:

- LED example base is preserved and runs.
- `app_main.c` / `app_main.h` application framework exists.
- USART1 printf works.
- Serial assistant has shown:

```text
[BOOT] PlantDisease Edge Control Start
[APP] heartbeat
```

- Official `05_Serial` has verified USART1 on PE5/PE6.
- Current UART configuration:
  - `USART1`
  - TX `PE5`
  - RX `PE6`
  - `GPIO_AF7_USART1`
  - `115200 8N1`

## Required Workflow

At the start of each task:

1. Read the current project structure.
2. Read the files relevant to the requested change.
3. Determine the exact files that need to change.
4. State the plan before editing if the task is non-trivial.
5. Keep the change scoped to the requested feature.
6. After editing, list changed files and STM32CubeIDE verification steps.

Use `rg` / `rg --files` first for searches.

## Modification Rules

`main.c` must remain thin. It should only handle generated initialization and call:

```c
App_Init();

while (1)
{
    App_Loop();
}
```

Only edit `main.c` inside USER CODE regions.

Application modules belong under:

```text
Appli/Core/Inc
Appli/Core/Src
```

In CubeIDE these appear as:

```text
Application/User/Core
```

Prefer one independent `.c/.h` module per feature.

## Forbidden Without Explicit Approval

Do not modify:

- `.ioc`
- startup files, including `startup_stm32n647x0hxq.s`
- linker scripts, including `*.ld`
- FSBL
- external storage configuration
- TrustZone / Secure / Non-Secure configuration
- HAL driver source files
- generated code outside USER CODE regions
- unrelated CubeIDE metadata
- Debug / Release build output
- `elf`, `hex`, `bin`, `o`, `d`, `map`, or other build products

Do not remove:

- existing LED behavior
- current UART printf support
- `App_Init()` / `App_Loop()` structure

Do not implement unrelated features in a focused task.

## No Guessing Rule

If a GPIO, active level, HAL handle, clock source, peripheral instance, or BSP function name is uncertain:

1. Search the current project.
2. Search official examples.
3. Read the relevant header/source file.
4. Ask the user if it still cannot be confirmed.

Do not guess pin names, HAL handles, or active levels.

## Current Development Order

Next task should be the board buzzer module:

1. Confirm the buzzer GPIO using official `02_Buzzer` and/or the schematic.
2. Add `buzzer.h`.
3. Add `buzzer.c`.
4. Initialize the buzzer without breaking LED or UART.
5. Keep heartbeat printing.
6. Make the buzzer short-beep once every 5 seconds.
7. Build and verify.

After buzzer:

1. `board_config.h`
2. `relay.c` / `relay.h`
3. `pump.c` / `pump.h`
4. `fan.c` / `fan.h`
5. `ai_result.c` / `ai_result.h`
6. `prescription.c` / `prescription.h`
7. minimal closed-loop demo
8. sensors
9. LCD UI
10. JSON logging
11. L610 upload

Do not rush LCD, AI model, camera, or L610 before the minimal GPIO/control path is stable.

## Verification Required After Code Changes

The user must verify in STM32CubeIDE:

1. Open/import:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647\STM32CubeIDE\Appli
```

2. Confirm project name:

```text
01_LED_Appli
```

3. Run `Clean Project`.
4. Run `Build Project`.
5. Target: 0 errors.
6. Download/debug on the STM32N647 board.
7. Confirm:
   - boot print appears
   - heartbeat prints
   - LED still alternates
   - the requested new feature behaves correctly

## Hardware Safety

Pump and fan must not be driven directly from STM32 GPIO. Use relays or MOS driver modules. Confirm external power, common ground, active level, and load current before enabling sustained outputs.

For future relay/pump/fan work, require the user to confirm:

- relay input pins
- active-high or active-low trigger
- pump relay channel
- fan relay channel
- external supply voltage/current
- common ground
- any pin conflicts with UART, LED, LCD, camera, XSPI, I2C, ADC, or L610
