# Zongle nRF52840 initial bring-up

This target is for custom hardware using **nRF52840**, not the original nRF52833
`zongle` target. It was tested on nRF52840 AAF2 with 1 MiB flash and 256 KiB RAM.

## Conservative hardware configuration

- USB UF2/CDC bootloader, volume label `ZongleBOOT`.
- S140 7.3.0; bootloader at `0xF4000`, MBR parameters at `0xFE000`.
- nRF52840 UF2 application family `0xADA52840`.
- Internal low-frequency RC oscillator; a working 32 MHz crystal is still required
  by USB and radio operation.
- No LED or button GPIOs configured.
- No reset/NFC pin reassignment or DC/DC enablement.
- No `REGOUT0` modification: the regulator output stays at its existing setting.

These defaults avoid applying the original nRF52833 board's GPIO or 3.3 V regulator
assumptions to unconfirmed hardware. Configure those features only after checking
the actual schematic and power supply. The bootloader cannot make incorrect power
wiring safe.

## Build

Install Arm GNU GCC and the Python tools described in the repository README,
initialize the submodules, then run from the repository root:

```sh
# This fork may not contain the upstream release tags needed by version macros.
git fetch --tags https://github.com/adafruit/Adafruit_nRF52_Bootloader.git
make BOARD=zongle_nrf52840 all
```

The merged `_build/build-zongle_nrf52840/*_s140_7.3.0.hex` contains the bootloader,
SoftDevice and bootloader UICR addresses for initial programming with a debugger.
Do not use an nRF52833 image for this hardware.

With no valid application installed, the bootloader exposes USB update mode. Since
no physical reset/button wiring is configured, an application should provide a
software return path, for example `GPREGRET = 0x57` followed by a system reset.
Application firmware must respect the SoftDevice, bootloader, settings and reserved
data regions; the included RF example links at `0x27000`.

## Validation

- Initial programming and full flash/UICR image-byte readback passed via J-Link.
- Native USB enumerated as `239a:00d8`, `Zongle nRF52840`.
- UF2 application upload and application readback through `CURRENT.UF2` passed.
- The RF example returned to the USB bootloader after its five-burst sequence;
  an over-the-air carrier was observed with a spectrum analyzer.

The initial protected firmware could not be backed up. On improved-APPROTECT parts,
the older bundled Nordic startup code may leave debug access protected again after
a power cycle. J-Link's automatic unsecure operation can mass-erase the target,
including this bootloader. Prefer UF2 application updates and do not accept an
unlock when preserving the installed image.
