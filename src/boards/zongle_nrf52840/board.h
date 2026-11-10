#ifndef ZONGLE_NRF52840_H
#define ZONGLE_NRF52840_H

// Initial USB bring-up: do not assume the nRF52833 Zongle GPIO wiring.
#define BOARD_HAS_CRYSTAL_LFCLK 0
#define LEDS_NUMBER 0
#define BUTTONS_NUMBER 0

// Deliberately leave UICR_REGOUT0_VALUE undefined: preserve regulator voltage.
// DC/DC converters remain disabled because their inductors are unconfirmed.

#define BLEDIS_MANUFACTURER "vuoz"
#define BLEDIS_MODEL "Zongle nRF52840"

#define USB_DESC_VID 0x239A
#define USB_DESC_UF2_PID 0x00D8
#define USB_DESC_CDC_ONLY_PID 0x00D8

#define UF2_PRODUCT_NAME "Zongle nRF52840"
#define UF2_VOLUME_LABEL "ZongleBOOT"
#define UF2_BOARD_ID "nRF52840-zongle-bringup-v1"
#define UF2_INDEX_URL "https://github.com/maurzo/Adafruit_nRF52_Bootloader"

#endif // ZONGLE_NRF52840_H
