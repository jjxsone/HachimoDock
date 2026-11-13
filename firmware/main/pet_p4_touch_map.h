#pragma once

/*
 * [Input] Geometry of the LCD and of the installed touch cover.
 * [Output] One place describing how controller samples map into UI space.
 * [Pos] Display/touch geometry contract for the P4 runtime.
 * [Sync] Update pet_p4_touch_core.c and tests/p4_touch_slice_test.c together.
 *
 * The UI is composed as 640x480 landscape and scanned out to the 480x640
 * panel with the original mapping (PPA angle 270, UI(x,y) -> panel(479-y, x)).
 *
 * The replacement FT6336U cover reports a 240x320 grid whose axes run
 * perpendicular to the UI: raw X follows the UI's vertical axis and raw Y the
 * horizontal one, at a uniform 2x scale. Measured on the panel corners:
 *
 *   UI top-left     -> raw (234,   8)
 *   UI bottom-left  -> raw (  0,   5)
 *   UI top-right    -> raw (239, 319)
 *   UI bottom-right -> raw (  0, 319)
 *
 * i.e. ui_x = raw_y * 639/319 and ui_y = 479 - raw_x * 479/239.
 * A cover that reports full 480x640 pixel coordinates would need 480/640 here
 * and the plain panel inverse instead.
 */

#define PET_P4_UI_WIDTH 640
#define PET_P4_UI_HEIGHT 480

#define PET_P4_FT6336_RAW_WIDTH 240
#define PET_P4_FT6336_RAW_HEIGHT 320
