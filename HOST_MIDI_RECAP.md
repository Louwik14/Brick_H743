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

## Enumeration and class binding
1. `usb_host_midi_init()` creates the USB Host core with `USBH_Init`, registers the MIDI class, starts the host, and spawns the ChibiOS thread `usb_host_thread`.
2. The thread calls `USBH_Process` every millisecond to drive enumeration and class state machines.
3. The MIDI class searches the configuration descriptor for the first Audio/MIDIStreaming interface (Class 0x01 / SubClass 0x03) and requires exactly one Bulk IN and one Bulk OUT endpoint.
4. Pipes are allocated and opened with the ST core APIs using the discovered endpoint addresses and packet sizes; data toggles are initialized to zero.

## MIDI data path
- **Reception**: The class continuously posts bulk IN URBs. Completed transfers are split into 4-byte USB-MIDI packets and queued in a circular buffer. Overflow is counted without blocking the stack.
- **Transmission**: Application writes 4-byte packets into a non-blocking TX ring. The class batches as many packets as fit in the endpoint’s max packet size and submits a bulk OUT URB. URB completion returns the pipe to IDLE for the next batch.
- **Supported messages**: 4-byte USB-MIDI events for Note On/Off, CC, Program Change, Pitch Bend, Channel Pressure, MIDI Clock/Start/Stop/Continue, Active Sensing, and Reset. SysEx and multi-cable are intentionally not supported.

## ChibiOS wrapper API
- `usb_host_midi_is_device_attached()` indicates physical connection events from the USBH user callback.
- `usb_host_midi_is_ready()` reflects class activation (`HOST_CLASS` state).
- `usb_host_midi_receive(packet)` and `usb_host_midi_send(packet)` are non-blocking; they return `false` if no data or if the host is not ready.
- `usb_host_midi_rx_overflow()` / `usb_host_midi_tx_overflow()` expose dropped packet counts.
- `usb_host_midi_reset_count()` exposes hardware port reset count collected in `USBH_LL_ResetPort`.

## Hotplug and recovery
- Connect and disconnect events are delivered by HAL HCD callbacks into the ST host core, which drives enumeration or cleanup automatically.
- The wrapper clears readiness flags on disconnect and keeps processing URBs on reconnect without rebooting.

## Test procedure
1. Power the STM32H743 board with the firmware built from this tree.
2. Plug a class-compliant USB MIDI keyboard into the OTG FS port (VBUS already at 5 V).
3. Observe through logs or LEDs that enumeration completes (device attached then MIDI ready).
4. Press keys or move controls; `usb_host_midi_receive()` should return USB-MIDI 4-byte packets for Note/CC/Clock without blocking. Check overflow counters remain zero.
5. Send MIDI events back via `usb_host_midi_send()`; the keyboard should play/flash as notes/CCs arrive.
6. Unplug and replug the keyboard; connection flags should update and port reset count should increment while the system continues running.
