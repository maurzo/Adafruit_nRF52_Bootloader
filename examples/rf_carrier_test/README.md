# Bounded nRF52840 RF carrier test

Temporary bare-metal application for the `zongle_nrf52840` bootloader and its S140
7.3.0 flash layout. This is a spectrum-analyzer bring-up test, not Bluetooth
advertising or receiver firmware. Do not load it onto an nRF52833.

## Waveform and safety

- Wait 5 seconds after startup.
- Emit a carrier at **2440 MHz**, nominal chip TX power **-20 dBm** (10 microwatts).
- Run **10 seconds ON / 5 seconds OFF** for **five bursts**.
- Turn RF off and return to the USB bootloader after approximately **75 seconds**.
- A power cycle repeats the test. Unplugging the board stops it immediately.

TIMER2 and PPI disable the radio at each 10-second cutoff independently of the CPU.
A 20-second watchdog handles stalls. A watchdog reset skips transmission and returns
to the bootloader rather than repeating a faulty test. No GPIO, external PA/RF switch,
regulator or UICR configuration is changed; controlled front-ends need board-specific
configuration before use.

USB is absent while the test runs; `ZongleBOOT` should return afterward. The original
bootloader and SoftDevice remain installed. The application does not enable the
SoftDevice and accesses the radio directly.

## Build

Use Arm GNU GCC (tested with 12.3.Rel1), Python 3, and initialized repository
submodules. From the repository root:

```sh
make -C examples/rf_carrier_test
```

The output is `_build/rf_carrier_test/rf-carrier-test.uf2`. The application links at
`0x27000`, reserves upper flash from `0xEA000`, and uses UF2 family `0xADA52840`.
It must not be used with a different SoftDevice/application flash layout.

When the analyzer is ready, copy the UF2 onto the mounted `ZongleBOOT` drive.
Transmission starts after the initial 5-second delay. If the final copy flush reports
an I/O error because the board restarted immediately, verify the returned drive's
`CURRENT.UF2` against the built HEX before assuming the upload succeeded. Do not
use J-Link auto-unlock for routine application updates: it can mass-erase the bootloader.

## Measurement

For an over-the-air check, start with analyzer center 2440 MHz, span 5 MHz, RBW
10 kHz, and a separate receive antenna about 0.5-1 m away. Max Hold captures the
peak; Clear/Write or zero-span/time mode shows the burst gaps. Prefer shielded or
conducted testing and follow local RF rules.

Received dBm is not the chip's configured TX power. Distance, polarization,
antenna patterns, cable losses and reflections affect it. This test does not
measure calibrated antenna gain, efficiency or impedance matching.

The carrier sequence follows Nordic's nRF5 SDK
[`examples/peripheral/radio_test/radio_test.c`](https://github.com/greenlsi/nrf5-sdk/blob/master/examples/peripheral/radio_test/radio_test.c).
The source matches the application tested over UF2 on nRF52840 AAF2; application
readback passed and the USB bootloader returned after the test. An over-the-air
carrier was observed by the operator; no calibrated RF performance claim is made.
