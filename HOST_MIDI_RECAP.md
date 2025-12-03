# USB MIDI Host Integration Recap

## Hardware path
- **MCU**: STM32H743 running ChibiOS/RT with D-Cache enabled.
- **USB port**: OTG FS on PA11 (DM) / PA12 (DP), alternate function AF10, no VBUS sensing or switching. VBUS is hardwired to 5 V.
- **Clocking**: RCC enables USB_OTG_FS, GPIOA, and SYSCFG. USB voltage detector is enabled for correct PHY power-up.
- **Interrupts**: OTG_FS_IRQn is configured with priority 6 and handled by `OTG_FS_IRQHandler` calling the HAL HCD ISR.

## Low-level host bring-up
1. `USBH_LL_Init` links the ST Host stack to the HAL HCD driver, configures the OTG FS core for full-speed embedded PHY, disables VBUS sensing, and sets FIFO sizes (RX 0x80 words, TX0 0x40, TX1 0x80).
2. `HAL_HCD_MspInit` configures PA11/PA12 pins, enables clocks, and sets up NVIC.
3. `USBH_LL_Start` starts the real controller; `USBH_LL_Stop` halts it cleanly. `USBH_LL_ResetPort` issues a hardware reset and increments a reset counter for diagnostics.
4. VBUS control is a no-op: `USBH_LL_DriverVBUS` intentionally does nothing because power is always present.

### Cache / DMA safety on STM32H743
- All URB submissions now clean or invalidate the D-Cache in `USBH_LL_SubmitURB` based on transfer direction, and completed IN transfers are invalidated in `USBH_LL_GetURBState` before the stack reads data.
- MIDI bulk buffers (`in_packet`, `out_packet`, RX/TX rings) are 32-byte aligned to match the cache line size.
- These measures prevent stale or dirty cache lines when the HCD DMA engine reads or writes MIDI payloads.

## Enumeration and class binding
1. USB Host memory is **fully static**: `USBH_malloc`/`USBH_free` map to a 32-byte aligned pool (`USBH_STATIC_MEM_SIZE`, default 16384 bytes) reset on every host restart to avoid heap fragmentation. An explicit OOM counter tracks allocation failures and is cleared with each allocator reset; devices presenting oversized descriptors may therefore be rejected but the event remains observable via this counter.
2. `usb_host_midi_init()` clears the allocator and MIDI FIFO, starts the host core, and spawns the ChibiOS thread `usb_host_thread`.
3. The thread calls `USBH_Process` every **250 µs** to drive enumeration and class state machines with lower latency.
4. The MIDI class searches the configuration descriptor for the first Audio/MIDIStreaming interface (Class 0x01 / SubClass 0x03) and requires exactly one Bulk IN and one Bulk OUT endpoint.
5. Pipes are allocated and opened with the ST core APIs using the discovered endpoint addresses and packet sizes; data toggles are initialized to zero.

## MIDI data path
- **Reception**: The class continuously posts bulk IN URBs. Completed transfers are split into 4-byte USB-MIDI packets and pushed by the USB thread into a static, lock-free SPSC FIFO dedicated to the application. Overflow drops packets, increments a counter, and triggers a guarded reset after too many consecutive drops.
- **Transmission**: Application writes 4-byte packets into a non-blocking TX ring. The class batches as many packets as fit in the endpoint’s max packet size and submits a bulk OUT URB. URB completion returns the pipe to IDLE for the next batch. TX overflow is still tracked in the USB thread via `USBH_MIDI_GetTxOverflow`.
- **Supported messages**: 4-byte USB-MIDI events for Note On/Off, CC, Program Change, Pitch Bend, Channel Pressure, MIDI Clock/Start/Stop/Continue, Active Sensing, and Reset. SysEx and multi-cable are intentionally not supported.

## ChibiOS wrapper API
- `usb_host_midi_is_device_attached()` indicates physical connection events from the USBH user callback.
- `usb_host_midi_is_ready()` reflects class activation (`HOST_CLASS` state).
- `usb_host_midi_receive(packet)` is non-blocking and reads only from the MIDI FIFO (no direct access to the ST class). `usb_host_midi_send(packet)` returns `false` if not ready or during a pending recovery.
- `usb_host_midi_rx_overflow()` / `usb_host_midi_tx_overflow()` expose dropped packet counts; RX reflects FIFO drops.
- `usb_host_midi_reset_count()` reports the sum of hardware port resets and software host restarts. `usb_host_midi_error_count()` tracks accumulated USB errors that triggered recoveries.

## Hotplug and recovery
- Connect and disconnect events are delivered by HAL HCD callbacks into the ST host core, which drives enumeration or cleanup automatically.
- The wrapper clears readiness flags on disconnect, resets the MIDI FIFO, and keeps processing URBs on reconnect without rebooting.
- Unrecovered USB errors, too many consecutive FIFO overflows, or **device inactivity over ~5 s** schedule an internal host restart: `USBH_Stop` → state cleanup + allocator reset → `USBH_Init` → class registration → `USBH_Start`. Audio is untouched.

## Test procedure
1. Power the STM32H743 board with the firmware built from this tree.
2. Plug a class-compliant USB MIDI keyboard into the OTG FS port (VBUS already at 5 V).
3. Observe through logs or LEDs that enumeration completes (device attached then MIDI ready).
4. Press keys or move controls; `usb_host_midi_receive()` should return USB-MIDI 4-byte packets for Note/CC/Clock without blocking. Check overflow counters remain zero.
5. Send MIDI events back via `usb_host_midi_send()`; the keyboard should play/flash as notes/CCs arrive.
6. Unplug and replug the keyboard; connection flags should update and port reset count should increment while the system continues running.
