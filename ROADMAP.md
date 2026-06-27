# Project Roadmap

This roadmap describes the planned development order for the STM32N647 PlantDisease edge-control project.

## Stage 0: Environment And Baseline

- Goal: Prepare tools, official packages, board access, and project path.
- Input: STM32CubeIDE, STM32CubeProgrammer, ST-LINK, official N647 package, migrated English path.
- Output: Project opens and baseline LED example can run.
- New files: None required.
- Acceptance standard: CubeIDE can open `01_LED_Appli`; board can be downloaded/debugged.
- Risks: Wrong workspace path, Chinese path issues, missing driver/package, boot/debug misconfiguration.
- Physical test needed: Yes.

Status: completed.

## Stage 1: UART Heartbeat

- Goal: Establish reliable serial printf debugging.
- Input: Official serial example, USART1 pin evidence, current LED project.
- Output: Boot log and heartbeat log over USART1.
- New files: `app_uart.h`, `app_uart.c`, `app_main.h`, `app_main.c`.
- Acceptance standard: Serial shows `[BOOT] PlantDisease Edge Control Start` and `[APP] heartbeat`; LED still blinks.
- Risks: Wrong UART pins, wrong baud rate, missing RIF pin attributes, blocking transmit overuse.
- Physical test needed: Yes.

Status: completed.

## Stage 2: Buzzer

- Goal: Port minimal board buzzer control.
- Input: Official `02_Buzzer` example and BSP `BEEP` driver.
- Output: Buzzer driver and periodic short-beep test.
- New files: `buzzer.h`, `buzzer.c`.
- Acceptance standard: Build 0 errors; buzzer short-beeps about every 5 seconds; UART heartbeat and LED remain normal.
- Risks: Wrong active level, wrong GPIO, GPIO conflict, too much blocking delay.
- Physical test needed: Yes.

Status: completed.

## Stage 3: Relay

- Goal: Implement external relay CH1/CH2 control and minimal state-machine test.
- Input: Confirmed wiring `PC8 -> IN1`, `PC11 -> IN2`, `RELAY_ACTIVE_LOW = 1`, 5V relay module, common GND.
- Output: Relay driver and non-blocking relay test sequence.
- New files: `board_config.h`, `relay.h`, `relay.c`.
- Acceptance standard: Build 0 errors; serial logs CH1/CH2 on/off; relay CH1 absorbs/releases; relay CH2 absorbs/releases.
- Risks: Relay module not compatible with 3.3V logic, missing common ground, active level inverted, load connected too early.
- Physical test needed: Yes.

Status: build passed; physical relay test pending.

## Stage 4: Pump / Fan Wrappers

- Goal: Add pump and fan application wrappers around relay channels.
- Input: Verified relay behavior, confirmed pump relay channel, confirmed fan relay channel.
- Output: `Pump_On/Off/RunMs` and `Fan_On/Off/RunMs` or non-blocking equivalents.
- New files: `pump.h`, `pump.c`, `fan.h`, `fan.c`.
- Acceptance standard: Pump/fan wrapper compiles and can control relay channels without breaking heartbeat, LED, buzzer, or relay base API.
- Risks: Driving real loads too early, power instability, relay contact rating, external supply wiring.
- Physical test needed: Yes, but start with relay indicators before connecting real loads.

## Stage 5: Simulated AI Recognition + Prescription Closed Loop

- Goal: Use mock AI result to trigger expert prescription and actuator decisions.
- Input: Mock disease result, relay/pump/fan wrappers, buzzer module.
- Output: Simulated disease -> prescription -> alarm/action flow.
- New files: `ai_result.h`, `ai_result.c`, `prescription.h`, `prescription.c`.
- Acceptance standard: Mock `Leaf Spot` with confidence prints; prescription is selected; expected alarm/action logs appear.
- Risks: Long blocking pump/fan runs, repeated triggering too frequently, unclear state transitions.
- Physical test needed: Initially no; later yes with actuator modules.

## Stage 6: JSON Logs

- Goal: Generate structured logs for later L610 upload.
- Input: AI result, prescription, action state, optional sensor data.
- Output: JSON text printed over debug UART.
- New files: `log_upload.h`, `log_upload.c`.
- Acceptance standard: Valid JSON string includes node id, disease, confidence, action, timestamp placeholder, and sensor placeholders.
- Risks: Buffer overflow, too much serial output, malformed JSON.
- Physical test needed: No, serial validation is enough.

## Stage 7: ADC Sensors

- Goal: Read analog sensor values.
- Input: Reserved ADC pin `PF6`, selected analog sensor module.
- Output: Raw ADC value and converted percentage/status.
- New files: `sensor.h`, `sensor.c`, optionally `sensor_rain.h`, `sensor_rain.c`, `sensor_soil.h`, `sensor_soil.c`.
- Acceptance standard: Serial prints stable raw ADC values that change with sensor condition.
- Risks: Wrong ADC channel, noisy readings, wrong voltage range, no common ground.
- Physical test needed: Yes.

## Stage 8: I2C Sensors

- Goal: Add shared I2C2 sensor bus.
- Input: Reserved `PD4 = SDA`, `PD14 = SCL`, DS3231, INA219, BMP280, BH1750.
- Output: Sensor drivers or minimal read probes.
- New files: `sensor_bh1750.h/.c`, `sensor_bmp280.h/.c`, `sensor_ina219.h/.c`, `sensor_ds3231.h/.c`.
- Acceptance standard: I2C bus scans or reads expected device IDs/data; serial prints parsed values.
- Risks: Pull-up resistors missing, address conflict, wrong I2C instance/pins, 3.3V/5V level mismatch.
- Physical test needed: Yes.

## Stage 9: LCD UI

- Goal: Display environment data, disease result, and action status.
- Input: Official `15_RGBLCD` example, existing BSP LCD drivers, app data structures.
- Output: Basic LCD pages or status screen.
- New files: `lcd_ui.h`, `lcd_ui.c`.
- Acceptance standard: LCD displays readable text without breaking UART, LED, buzzer, relay, and sensor tasks.
- Risks: Large driver integration, framebuffer memory, pin conflicts, refresh flicker.
- Physical test needed: Yes.

## Stage 10: L610 Upload

- Goal: Send JSON logs through L610 Cat.1 module.
- Input: JSON log module, selected UART for L610, module power wiring, SIM/network readiness.
- Output: AT command flow and upload prototype.
- New files: `l610.h`, `l610.c`, or extend `log_upload.h/.c` with transport abstraction.
- Acceptance standard: AT responds, network attaches, JSON data can be sent to test endpoint or printed through L610 path.
- Risks: Power spikes, UART level mismatch, SIM/network issues, blocking AT command flow.
- Physical test needed: Yes.

## Stage 11: Camera / AI / NPU

- Goal: Integrate camera capture and real AI inference on STM32N647.
- Input: Official camera examples, AI application examples, trained/converted disease model.
- Output: Real disease recognition result feeding the existing closed-loop control interface.
- New files: likely `camera.h/.c`, `ai_model.h/.c`, `ai_result` adapter updates.
- Acceptance standard: Camera frame captured; AI inference returns disease id/confidence; existing prescription pipeline consumes it.
- Risks: High integration complexity, memory placement, FSBL/external memory, NPU toolchain, LCD/camera pin conflicts.
- Physical test needed: Yes.

## Current Priority

Tomorrow: finish Stage 3 physical relay validation before adding Stage 4 pump/fan wrappers.
