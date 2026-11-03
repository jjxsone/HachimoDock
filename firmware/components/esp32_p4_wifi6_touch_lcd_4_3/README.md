# ESP32-P4 WLK2802 Display BSP

This local Waveshare BSP adaptation drives the 480x640 P028TRM160-IPS
(ST7701S, 20-pin FPC, one D0 data lane) over one 800Mbps MIPI DSI data lane.
The firmware renders a 640x480 logical framebuffer and rotates it onto the
portrait panel. The panel module has no touch controller (FPC pins 15-19 are
NC), so touch stays unavailable and buttons remain the only input.

`vendor_specific_init_default[]` is copied byte-for-byte from the panel
supplier's "ST7701S MIPI 1Lane" script. The GIP/power registers (C1/C2,
E1..EF) are panel-specific; substituting another module's table leaves the
gate driver off (backlight lit, black image) even though the DSI link reports
ESP_OK. Re-check the table whenever the panel vendor changes.

DSI PHY clock selection is left to the SDK (`phy_clk_src = 0`) so each
chip family uses its supported PLL reference. The old explicit PHY default
is invalid on P4 v3. Lane rate, rotation, RGB565 format and resolution are unchanged.

P4 v3 uses an exact 24MHz DPI clock (240MHz / 10). Requesting 25MHz with
IDF 5.5.4 produces 24MHz and a horizontal timing correction of -21 pixels,
which exceeds this panel's 10-pixel front porch and prevents normal scanout.
The v1 build retains its existing 25MHz request and SDK timing behavior.

The panel command return path is treated as write-only. Startup skips the
optional ST7701S ID read and disables command ACK requests before sending the
vendor initialization table. Waiting for either response can otherwise fill
the ESP32-P4 command FIFO and block startup before the backlight is enabled.

## Backlight

```c
bsp_display_brightness_init();
bsp_display_backlight_on();
bsp_display_backlight_off();
bsp_display_brightness_set(100);
```

## Dependencies

| Capability | Implementation |
|---|---|
| Display | Local `esp_lcd_st7701` 2.0.2 override |
| Touch | `espressif/esp_lcd_touch_gt911` |
| Audio | `espressif/esp_codec_dev` |
| SD card | ESP-IDF |
