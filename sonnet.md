# Code Review — stm_lights_driver
**Target:** STM32F303K8 · CAN-driven LED light controller (PERLA)
**Scope:** Application code under `ligths_driver/App/` only (vendored HAL/CMSIS excluded)

---

## Overall Rating: 4 / 10

| Area | Score | Reason |
|------|-------|--------|
| Driver quality (`led`, `can`, `error_handler`, `adc`, `pwm`) | 7/10 | Clean, null-checked, well-documented, reusable |
| Application integration | 2/10 | Three files define the same symbols → won't link |
| Functional correctness | 3/10 | Hardcoded board side, broken ADC guard, blocking safe-state |
| ISR / concurrency safety | 4/10 | Flags are `volatile`; byte arrays are not protected |
| Maintainability | 4/10 | Dead code, divergent protocol defs, mixed style |
| Documentation | 6/10 | Good Doxygen; one-line README |

---

## CRITICAL — Will not build / link

### 1. Three files define the same symbols

`app.c`, `can_app.c`, and `global_variables.c` all define identical non-static globals and functions. CubeIDE compiles all three, producing multiple-definition linker errors.

| Symbol | Defined in |
|--------|-----------|
| `APP_InterpretFrames()` | `app.c` + `can_app.c` |
| `HAL_CAN_RxFifo0MsgPendingCallback()` | `app.c` + `can_app.c` |
| `brakeStatus`, `reverseStatus`, `safeStateStatus`, … | all three files |
| `positionStatus`, `lowBeamStatus`, `leftTurnStatus`, … | `app.c` + `global_variables.c` |

**Fix:** `main.c` includes `can_app.h` and calls `RearService()` directly, so the intended live path is `can_app.c` + `rear_service.c`. Delete `app.c`, `app.h`, `global_variables.c`, and `global_variables.h` — git history preserves them.

### 2. Board selection is dead; right-rear always behaves as left-rear

`main.c` calls `RearService()` directly, bypassing `APP_Main()` and `BOARD_ChooseBoard()`. Inside `rear_service.c` the side is a compile-time constant:

```c
const bool boardIsLeft = true;   // rear_service.c line 18
```

This means:
- `BOARD_ChooseBoard()` (ADC voltage read) is **never executed**.
- `LF_Service()` and `RF_Service()` are **never reached**.
- The right-rear board silently runs left-rear logic (`ledLongLight` stays off on brake, wrong direction-blink side in `can_app.c`).

**Fix:** Call `BOARD_ChooseBoard()` early in `main.c` and pass the result into `RearService(bool isLeft)`.

### 3. ADC bounds checks are always true

```c
// adc_driver.c — voltage check
if (tempValue >= STM32_GND || tempValue <= STM32_VCC)   // always true for any float
```

The operator should be `&&`, not `||`. As written the error path is unreachable.

```c
// adc_driver.c — raw value check
if (endValue >= 0 && endValue <= ADC_Resolution(hadc))  // endValue is uint16_t → always >= 0
```

The lower bound is vacuous for an unsigned type. Both checks fail to catch out-of-range readings.

---

## HIGH — Correctness / robustness

### 4. Tick arithmetic wraps at ~49 days

```c
// can_driver.c line 119
if (currentTick > msg->lastTick + msg->periodMs)
```

`lastTick + periodMs` overflows `uint32_t` near the rollover boundary, causing all scheduled messages to stop sending until the tick counter catches up. The correct, wrap-safe form is:

```c
if ((uint32_t)(currentTick - msg->lastTick) >= msg->periodMs)
```

Additionally, `msg->lastTick = HAL_GetTick()` after each send accumulates call latency, causing slow period drift. Use `msg->lastTick += msg->periodMs` instead.

### 5. `CheckForSafeState()` blocks the main loop for 1.2 seconds

```c
// rear_service.c
if (safeStateStatus)
{
    while (true)          // spins for SAFE_STATE_DURATION_MS = 1200 ms
    {
        LED_Handle(&ledSafeState);
        LED_Handle(&ledDirection);
        ...
    }
}
```

During this spin, `CAN_HandleScheduled()` and `APP_InterpretFrames()` are frozen. Heartbeat/status frames stop transmitting, and new CAN commands are silently dropped — the worst possible behavior during a safety event.

**Fix:** Track a `uint32_t safeStateEnteredAt` timestamp; check `HAL_GetTick() - safeStateEnteredAt >= SAFE_STATE_DURATION_MS` each main-loop pass without blocking.

### 6. Multi-byte CAN payload is not atomically exchanged with the ISR

```c
// can_app.c — ISR
memcpy(canDashboardLightsData, tempData, CAN_MAX_DLC);   // 8 bytes
dashboardLightsDataCheck = true;
```

If the ISR fires again while the main loop is mid-`memcpy` of the same buffer, the array is silently corrupted. The `volatile bool` flag is not a memory barrier. Options: double-buffer the payload, or briefly mask the FIFO interrupt around the read in `APP_InterpretFrames()`.

Also: the return value of `HAL_CAN_GetRxMessage()` is never checked.

### 7. `ADC_Init` hardcodes ADC1 clock regardless of which ADC is passed

```c
// adc_driver.c line 267
__HAL_RCC_ADC1_CLK_ENABLE();   // called for both hadc1 and hadc2
```

Works on F3 only because ADC1/ADC2 share a clock block. Gate on `hadc->Instance`:

```c
if (hadc->Instance == ADC1)      __HAL_RCC_ADC1_CLK_ENABLE();
else if (hadc->Instance == ADC2) __HAL_RCC_ADC2_CLK_ENABLE();
```

### 8. `fabs` used instead of `fabsf` on float operands

```c
// board_selector.c
if (fabs(LEFT_BACK_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
```

`fabs` promotes to `double`, which is software-emulated on Cortex-M4. Use `fabsf` throughout for both correctness and performance.

---

## MEDIUM — Maintainability

### Two conflicting protocol decoders

`app.h` treats turn signals as single bits (bit 5 / bit 6). `can_app.c` treats them as 2-bit fields at bits 3–4 / 5–6 per `CAN_DB.dbc`. PRND decoding also differs: `app.c` spans two bytes; `can_app.c` reads a 2-bit field from byte 1. Both cannot be correct.

**Fix:** Generate a single `can_signals.h` from the `.dbc` and use it everywhere.

### `global_variables.*` is dead boilerplate

~70 lines of trivial get/set wrappers (`setBrakeStatusTrue()`, `getBrakeStatus()`, …) that nobody in the current code path calls. `global_variables.h` has **no include guard**, making it unsafe to include twice.

### Inconsistent `volatile` on the same objects

`volatile bool brakeStatus` in `can_app.h` vs plain `bool brakeStatus` in `global_variables.h`. Using the same object with different qualifiers in different translation units is **undefined behavior** in C.

### Magic numbers in `error_handler.c`

```c
int32_t newPeriod = 300 - ((hehandler->activeErrorCount - 1) * 30);
if (newPeriod < 100) newPeriod = 100;
```

`300`, `30`, and `100` should be named `#define`s.

### Stub/empty files

`dma_driver.c` is empty. `LF_service.c` and `RF_service.c` contain only an empty function body. Add a `/* TODO */` comment or a `#warning` so they don't silently slip through integration.

### Spelling and naming

- Folder, project, and `.ioc` file: `ligths_driver` → `lights_driver`
- `getData_HeightbeatOK` → `getData_HeartbeatOK`
- `severals`, `mentiooned`, `Reseolution` in comments
- Stray `~` at end of `SWAP_ENDIANNESS` doc comment
- Four naming conventions coexist: `PascalCase` (`RearService`), `camelCase` (`brakeStatus`), `snake_case` (`board_selector`), module-prefix (`EH_`, `ADC_`, `LED_`, `CAN_`) vs no prefix. Adopt one convention per category and enforce with `.clang-format`.

---

## What is done well

- **`led_driver`** is a clean non-blocking state machine. The network-synchronized `syncTick` approach is well-suited for multi-board blink coordination, and the header clearly documents the `HAL_IncTick` hook.
- **`can_driver`** and **`error_handler`** consistently null-check every pointer and return `HAL_StatusTypeDef`, matching HAL conventions.
- **`error_handler`** error multiplexing (cycling through active errors on a single CAN ID, scaling period with error count) is thoughtful design.
- **`adc_utils` / `stm32_family.h`** — reading ADC configuration directly from registers rather than requiring the user to re-declare it is a genuinely reusable pattern.
- **`can_app.c`** bit-field decoding is clearly commented with the `.dbc` layout inline.
- Doxygen headers are present on most public functions.
- The `SWAP_ENDIANNESS` / `GET_BYTE` macros in `can_driver.h` are practical and safe (with the documented side-effect warning).

---

## Good-practices guide — embedded C

1. **One owner per symbol.** Define every global/function in exactly one `.c` file. Everything else uses `extern` from one canonical header. File-local symbols get `static`. This resolves the critical linker issues entirely.

2. **Single source of truth for the CAN protocol.** Keep all frame IDs and signal bit-layouts in one header (ideally auto-generated from the `.dbc`). Two decoders that agree today will diverge silently.

3. **Never block the cooperative loop.** Any timed behavior (safe-state hold, blink phase) is a state + timestamp, checked non-blockingly each pass. Reserve the only `while(1)` for genuine fatal-halt paths such as `haltNode()`.

4. **Wrap-safe tick arithmetic.** Write `(uint32_t)(now - then) >= period`, never `now > then + period`. The unsigned subtraction wraps correctly on rollover; addition does not.

5. **ISR ↔ task data exchange.** A `volatile bool` flag is sufficient for a single-byte signal. For a multi-byte payload, use a short critical section (`__disable_irq()` / `__enable_irq()`) or double-buffering. Always check HAL return values, including inside callbacks.

6. **Use float functions for floats.** `fabsf`, `sqrtf`, `roundf` — not their `double` counterparts. On a Cortex-M4 with FPU, single-precision is hardware; double is not.

7. **Correct range checks.** `x >= 0` on an unsigned type is always true. `a || b` when you mean `a && b` silently disables safety. Enable `-Wtype-limits` and `-Wlogical-op` (GCC) to catch these at compile time.

8. **Treat warnings as errors.** Compile with `-Wall -Wextra -Wshadow -Wtype-limits -Wlogical-op`. Most of the issues above would surface before the project even links. Add `cppcheck` or a MISRA-C subset check in CI for safety-critical paths.

9. **Delete dead code; do not comment it out or leave parallel versions.** Git preserves history. Parallel implementations of the same module confuse new contributors and cause exactly the duplicate-symbol failures seen here.

10. **Consistent formatting.** Add a `.clang-format` file and run it in CI. Mixed tabs/spaces and four naming conventions in one project are a maintenance tax on every future reader.
