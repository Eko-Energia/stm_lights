# Code Review — stm_lights_driver

Review of the application code (`App/`). The vendored HAL/CMSIS code is ST-generated and out of scope.

## Summary

The firmware is a per-board light controller for an STM32F303K8 that reacts to CAN frames and drives LED GPIOs. The individual drivers (`led_driver`, `can_driver`, `error_handler`, `adc_driver`, `pwm_driver`) are mostly well-structured and documented. **However, the application layer is in a half-finished refactor: three parallel implementations of the same feature coexist and define the same symbols, so the project almost certainly does not link cleanly, and the active code path has a real correctness bug.**

---

## Critical — build / won't-work

### 1. Duplicate symbol definitions across three files

`app.c`, `can_app.c`, and `global_variables.c` each define the *same non-static globals and functions*. Since nothing is excluded in `.cproject`, CubeIDE compiles all three, giving multiple-definition linker errors.

- `APP_InterpretFrames()` — defined in `app.c` and `can_app.c`
- `HAL_CAN_RxFifo0MsgPendingCallback()` — defined in `app.c` and `can_app.c`
- `brakeStatus`, `reverseStatus`, `safeStateStatus`, `brakeChangeFlag`, `reverseChangeFlag` — defined in all three
- `positionStatus`, `lowBeamStatus`, `highBeamStatus`, `leftTurnStatus`, ... — in `app.c` and `global_variables.c`

Pick **one** owner. Given `main.c` includes `can_app.h`/`rear_service.h` and calls `RearService()`, the intended path looks like `can_app.c` + `rear_service.c`. `app.c`, `app.h`, and the whole `global_variables.*` pair look like dead predecessors that should be deleted.

### 2. `RearService` signature/dispatch is inconsistent, and board selection is dead

- `main.c` calls `RearService();`
- but `app.c` (the other implementation) calls it *with an argument*: `RearService(true)` / `RearService(false)`.
- `rear_service.c` defines `void RearService(void)` and hardcodes the side: `const bool boardIsLeft = true;`

Consequences: the ADC `BOARD_ChooseBoard()` logic and the `LF_Service`/`RF_Service` front boards are never reached (`main` bypasses `APP_Main`), and every rear board behaves as **left** because `boardIsLeft` is a compile-time constant. The left/right distinction (`ledLongLight` handling, direction-blink side in `can_app.c`) is therefore wrong for the RB board. Board side needs to be resolved at runtime (from `BOARD_ChooseBoard()`) and passed in.

## High — correctness / robustness

### 5. `CheckForSafeState()` blocks the whole main loop

```c
static void CheckForSafeState(void)
{
	if (safeStateStatus)
	{
		while (true)
		{
			...
		}
	}
}
```

This spins for `SAFE_STATE_DURATION_MS` (1.2 s) handling only two LEDs. During that window `CAN_HandleScheduled()` and `APP_InterpretFrames()` don't run, so status/heartbeat frames stop and new CAN commands are ignored — the opposite of what you want in a safety state. Fold this into the cooperative loop as a non-blocking state (track a timer, keep ticking all handlers each pass).

### 6. ISR ↔ main-loop data sharing isn't atomic

The RX callback `memcpy`s 8-byte payloads into buffers that `APP_InterpretFrames()` reads later. The `volatile bool ...Check` flags are fine, but the multi-byte arrays can be torn if a new frame arrives mid-read. Snapshot the buffer with the matching FIFO IRQ briefly masked (`__disable_irq()`/`HAL_CAN_DeactivateNotification`) or use double-buffering. Also, the return value of `HAL_CAN_GetRxMessage` isn't checked.

### 7. `ADC_Init` always enables the ADC1 clock

```c
// Enabling clock for ADC1
__HAL_RCC_ADC1_CLK_ENABLE();
```

This is called for both `hadc1` and `hadc2` (`APP_Main`), so ADC2 gets the wrong clock enable. On F3 the ADC12 clock is shared so it happens to work, but hardcoding a peripheral in a "universal" driver is a latent bug — gate it on `hadc->Instance`.

### 8. `fabs` (double) on Cortex-M4 for a float compare

```c
if (fabs(LEFT_BACK_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
```

`fabs` forces double-precision (software emulated, larger/slower). Use `fabsf`. Minor, but pervasive if copied.

---

## Medium — maintainability

- **Two disagreeing definitions of the CAN protocol.** `app.h` decodes turn signals as single bits (`TURN_SIGNAL_LEFT_BITPOS=5`, one bit each) while `can_app.c` decodes them as 2-bit fields (bits 3-4 / 5-6, per `CAN_DB.dbc`), and PRND decoding differs too (`app.c` shifts across two bytes; `can_app.c` uses `(byte1>>6)&0x3`). Keep **one** source of truth for frame IDs and bit layouts (ideally generated from the `.dbc`).
- **`global_variables.*` is dead boilerplate** — ~70 lines of trivial get/set wrappers that duplicate state nobody calls. `global_variables.h` also has **no include guard**. Delete it.
- **Inconsistent `volatile` qualifiers for the same objects:** `volatile bool brakeStatus` in `app.h`/`can_app.h` vs plain `bool brakeStatus` in `global_variables.h`. Declaring the same object with differing qualifiers is undefined behavior. Another reason to collapse to one declaration.
- **Empty/stub files:** `dma_driver.c` is empty, `LF_service.c`/`RF_service.c` are empty stubs, `README.md` is a single line. Either implement or mark clearly as TODO placeholders.
- **Magic numbers** in `error_handler.c`:

```c
int32_t newPeriod = 300 - ((hehandler->activeErrorCount - 1) * 30);
if (newPeriod < 100) newPeriod = 100;
```

  Promote `300 / 30 / 100` to named `#define`s.
- **Spelling** throughout hurts grep-ability and looks unpolished: the whole folder/`.ioc` is `ligths_driver` (should be `lights`), plus `getData_HeightbeatOK`, `severals`, `mentiooned`, `Reseolution`, and a stray `~` in the `SWAP_ENDIANNESS` doc comment.
- **Inconsistent style**: mix of tabs and spaces, and four naming conventions across modules (`PascalCase` `RearService`, `camelCase` `brakeStatus`, `snake_case` `board_selector`, `SCREAMING` macros, plus `EH_`/`ADC_`/`LED_`/`CAN_` prefixes vs none). Pick one per category and apply a `.clang-format`.

---

## What's good (keep doing this)

- `led_driver` is a clean, non-blocking state machine with a shared `syncTick` — nice approach for network-synchronized blinking, and the header documents the `HAL_IncTick` hook clearly.
- `error_handler` and `can_driver` consistently null-check pointers and return `HAL_StatusTypeDef`.
- Doxygen headers on most public functions.
- The `adc_utils`/`stm32_family` abstraction for reading ADC config straight from registers is a genuinely reusable idea.
- Bit decoding in `can_app.c` is well-commented with the `.dbc` layout inline.

---

## Good-practices guide (embedded C, this project)

1. **One owner per symbol.** Every global/function defined in exactly one `.c`; everything else uses `extern` from a single header. Anything file-local gets `static`. This alone resolves the critical linker issues.
2. **Single source of truth for the protocol.** Generate frame IDs and signal bit layouts from the `.dbc` (or one shared header) so decoders can't drift apart.
3. **Never block the main loop.** Model long behaviors (safe-state hold, blink timing) as states with timestamps checked every pass. Reserve `while(1)` blocking only for genuine fatal-halt paths.
4. **Wrap-safe time math.** Always compare ticks as `(uint32_t)(now - then) >= period`, never `now > then + period`.
5. **ISR-safe exchange.** Share single-word `volatile` flags; for multi-byte payloads snapshot under a short critical section or double-buffer. Keep ISRs short and check HAL return codes.
6. **Prefer integers; use float carefully.** For ADC thresholds compare raw counts, or if using volts use `fabsf` and correct (`&&`) range checks.
7. **Consistent naming + formatting.** Add a `.clang-format` and a short conventions note; fix the `ligths`→`lights` spelling while renaming is cheap.
8. **Build hygiene.** Enable `-Wall -Wextra -Wshadow` and treat warnings as errors — most issues above (unsigned `>=0`, always-true `||`, unused `global_variables`) would surface immediately. Consider `cppcheck`/clang-tidy or a MISRA subset in CI.
9. **Delete dead code** rather than leaving parallel versions; git history preserves it.
