# Tomorrow Relay Test Checklist

Purpose: verify relay CH1 and CH2 absorb/release behavior only. Do not connect pump or fan loads yet.

## 1. Before Power-On

- Connect only the relay control side.
- Do not connect water pump load.
- Do not connect fan load.
- Connect `PC8` to relay `IN1`.
- Connect `PC11` to relay `IN2`.
- Connect relay `VCC` to `5V`.
- Connect relay `GND` to STM32 `GND`.
- Confirm STM32 and relay module share ground.
- Confirm the relay module input supports `3.3V` logic trigger.
- Confirm relay module is expected to be low-level trigger for this test.
- Keep hands and metal tools away from relay load terminals.

## 2. STM32CubeIDE Steps

1. Open:

```text
C:\Users\Administrator\Documents\Embedded\PlantDisease_EdgeControl_N647\STM32CubeIDE\Appli
```

2. Confirm project:

```text
01_LED_Appli
```

3. Run `Clean Project`.
4. Run `Build Project`.
5. Confirm build result is `0 errors`.
6. Run `Debug As -> STM32 Cortex-M C/C++ Application`.
7. Press `F8` / `Resume`.

## 3. Serial Assistant Configuration

- Port: `COM6`
- Baud rate: `115200`
- Data bits: `8`
- Parity: `None`
- Stop bits: `1`
- Display mode: `ASCII`

## 4. Expected Serial Output

Startup and heartbeat:

```text
[BOOT] PlantDisease Edge Control Start
[BUZZER] init ok
[RELAY] init ok
[APP] heartbeat
```

Relay test sequence:

```text
[RELAY] CH1 ON
[RELAY] CH1 OFF
[RELAY] CH2 ON
[RELAY] CH2 OFF
```

The relay sequence should repeat about every 10 seconds.

## 5. Expected Hardware Behavior

- LED keeps blinking normally.
- Buzzer short-beeps about once every 5 seconds.
- Relay CH1 absorbs for about 1 second, then releases.
- About 1 second later, relay CH2 absorbs for about 1 second, then releases.
- No pump or fan should move because no load should be connected.

## 6. Abnormal Behavior Checklist

If a relay is always absorbed:

- Check whether the relay module is low-level trigger or high-level trigger.
- Check `RELAY_ACTIVE_LOW`.
- Check whether IN1/IN2 wires are swapped or shorted.

If serial output is normal but relay does not move:

- Check relay `VCC`.
- Check relay `GND`.
- Check common ground between relay module and STM32.
- Check `PC8 -> IN1`.
- Check `PC11 -> IN2`.
- Confirm the relay input supports `3.3V` logic.

If heartbeat disappears or becomes very slow:

- Check whether code was changed after this checklist.
- Check whether the relay test state machine was replaced by blocking delay.
- Do not add pump/fan code before the relay test is stable.

If Debug fails:

- Check ST-LINK connection first.
- Check board power and boot mode.
- Do not modify application code as the first response.

If relay action causes reset or serial interruption:

- Disconnect relay module and retest board-only behavior.
- Check power stability.
- Check common ground.
- Do not connect pump/fan loads until control-side relay behavior is stable.

## 7. Test Result Notes

Record tomorrow:

- Build result:
- Serial port used:
- CH1 behavior:
- CH2 behavior:
- Whether `RELAY_ACTIVE_LOW = 1` is correct:
- Any abnormal reset or serial interruption:
