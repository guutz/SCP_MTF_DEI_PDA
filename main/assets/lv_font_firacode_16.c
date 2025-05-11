/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --no-compress --font FiraCode-Regular.ttf --symbols ~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:"ZXCVBNM<>?`1234567890-=qwertyuiop[]\asdfghjkl;'zxcvbnm,./█  --format lvgl -o lv_font_firacode_16.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_FONT_FIRACODE_16
#define LV_FONT_FIRACODE_16 1
#endif

#if LV_FONT_FIRACODE_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x54, 0xf,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x13, 0x12, 0x12, 0x7f, 0x22, 0x22, 0x22, 0x22,
    0xff, 0x24, 0x24, 0x64,

    /* U+0024 "$" */
    0x10, 0x20, 0x43, 0xed, 0x72, 0x24, 0x68, 0x38,
    0x38, 0x48, 0x91, 0x3a, 0x9e, 0x8, 0x10, 0x20,

    /* U+0025 "%" */
    0x71, 0x44, 0xa2, 0x91, 0xc8, 0xc3, 0xc0, 0x60,
    0x26, 0x24, 0xb2, 0x51, 0x30, 0x60,

    /* U+0026 "&" */
    0x3c, 0x22, 0x10, 0x8, 0x6, 0x0, 0xfd, 0x89,
    0x4, 0x82, 0x41, 0x10, 0x87, 0x80,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x11, 0x88, 0x84, 0x42, 0x10, 0x84, 0x21, 0x84,
    0x30, 0x82,

    /* U+0029 ")" */
    0x43, 0x8, 0x21, 0x4, 0x21, 0x8, 0x42, 0x31,
    0x18, 0x88,

    /* U+002A "*" */
    0x10, 0x22, 0x4f, 0xf1, 0x5, 0x1b, 0x22,

    /* U+002B "+" */
    0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10,

    /* U+002C "," */
    0xfd, 0xed, 0x0,

    /* U+002D "-" */
    0xfe,

    /* U+002E "." */
    0xff, 0x80,

    /* U+002F "/" */
    0x0, 0x3, 0x2, 0x6, 0x4, 0xc, 0x8, 0x8,
    0x10, 0x10, 0x20, 0x20, 0x60, 0x40, 0xc0, 0x80,

    /* U+0030 "0" */
    0x38, 0x8a, 0x3c, 0x59, 0xb2, 0x64, 0xd9, 0xa3,
    0xc5, 0x11, 0xc0,

    /* U+0031 "1" */
    0x30, 0xe3, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x47, 0xf0,

    /* U+0032 "2" */
    0x7a, 0x30, 0x41, 0x4, 0x30, 0x84, 0x31, 0x8c,
    0x3f,

    /* U+0033 "3" */
    0x3c, 0x46, 0x2, 0x2, 0x4, 0x18, 0x2, 0x1,
    0x1, 0x1, 0xc2, 0x3c,

    /* U+0034 "4" */
    0x0, 0x30, 0x41, 0x82, 0xc, 0x91, 0x62, 0xfe,
    0x8, 0x10, 0x20,

    /* U+0035 "5" */
    0x7e, 0x81, 0x2, 0x4, 0xf, 0x81, 0x81, 0x2,
    0x5, 0x13, 0xc0,

    /* U+0036 "6" */
    0x3c, 0x81, 0x4, 0xb, 0xd8, 0xa0, 0xc1, 0x83,
    0x5, 0x11, 0xc0,

    /* U+0037 "7" */
    0xfe, 0xc, 0x10, 0x60, 0x83, 0x6, 0x8, 0x30,
    0x41, 0x80, 0x0,

    /* U+0038 "8" */
    0x3c, 0xcd, 0xa, 0x16, 0x67, 0x9b, 0x41, 0x83,
    0x7, 0x1b, 0xc0,

    /* U+0039 "9" */
    0x38, 0x8a, 0xc, 0x18, 0x38, 0xde, 0x81, 0x4,
    0x11, 0xc0, 0x0,

    /* U+003A ":" */
    0xfc, 0x0, 0x7, 0xe0,

    /* U+003B ";" */
    0xfc, 0x1, 0xfb, 0xda, 0x0,

    /* U+003C "<" */
    0x2, 0x18, 0xe3, 0xc, 0x18, 0xc, 0xe, 0x6,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x80, 0xc0, 0xe0, 0x60, 0x60, 0xc6, 0x38, 0xc0,

    /* U+003F "?" */
    0x7b, 0x10, 0x41, 0xc, 0x61, 0x4, 0x0, 0x3,
    0xc,

    /* U+0040 "@" */
    0x3c, 0x61, 0x80, 0x40, 0x17, 0x8e, 0x46, 0x23,
    0x11, 0x88, 0xc4, 0x66, 0x2f, 0x20, 0x70,

    /* U+0041 "A" */
    0xc, 0x3, 0x1, 0xe0, 0x58, 0x12, 0xc, 0xc2,
    0x30, 0x84, 0x7f, 0x10, 0x64, 0xb, 0x2,

    /* U+0042 "B" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0x21, 0xc1, 0x83,
    0x6, 0x17, 0xc0,

    /* U+0043 "C" */
    0x1e, 0x62, 0x40, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x40, 0x62, 0x1e,

    /* U+0044 "D" */
    0xf9, 0x1a, 0x14, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0xa, 0x37, 0xc0,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x82, 0xf, 0xa0, 0x82, 0x8,
    0x3f,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x82, 0xf, 0xa0, 0x82, 0x8,
    0x20,

    /* U+0047 "G" */
    0x1e, 0x63, 0x40, 0x80, 0x80, 0x80, 0x8f, 0x81,
    0x81, 0x41, 0x61, 0x3e,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x18, 0x3f, 0xe0, 0xc1, 0x83,
    0x6, 0xc, 0x10,

    /* U+0049 "I" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+004A "J" */
    0x1e, 0x4, 0x8, 0x10, 0x20, 0x40, 0x81, 0x2,
    0x5, 0x13, 0xc0,

    /* U+004B "K" */
    0x87, 0x1a, 0x24, 0x8b, 0x1c, 0x28, 0x58, 0x99,
    0x1a, 0x14, 0x30,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+004D "M" */
    0xc3, 0xe3, 0xe7, 0xa5, 0xa5, 0xa5, 0xbd, 0x99,
    0x99, 0x99, 0x81, 0x81,

    /* U+004E "N" */
    0xc3, 0x87, 0x8d, 0x1b, 0x32, 0x64, 0xcd, 0x8b,
    0x1e, 0x1c, 0x30,

    /* U+004F "O" */
    0x3c, 0x42, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x42, 0x3c,

    /* U+0050 "P" */
    0xf9, 0xa, 0xc, 0x18, 0x30, 0x61, 0x7c, 0x81,
    0x2, 0x4, 0x0,

    /* U+0051 "Q" */
    0x3c, 0x21, 0x10, 0x90, 0x28, 0x14, 0xa, 0x5,
    0x2, 0x81, 0x21, 0x90, 0x87, 0x80, 0x30, 0x8,
    0x4,

    /* U+0052 "R" */
    0xf8, 0x84, 0x82, 0x82, 0x82, 0x84, 0xf8, 0x98,
    0x8c, 0x84, 0x86, 0x82,

    /* U+0053 "S" */
    0x7d, 0x8e, 0x4, 0xe, 0xf, 0x3, 0x1, 0x2,
    0x6, 0x13, 0xc0,

    /* U+0054 "T" */
    0xff, 0x84, 0x2, 0x1, 0x0, 0x80, 0x40, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x1, 0x0,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x5, 0x11, 0xc0,

    /* U+0056 "V" */
    0x40, 0xa0, 0x58, 0x64, 0x22, 0x11, 0x98, 0xc8,
    0x24, 0x1e, 0xe, 0x3, 0x1, 0x80,

    /* U+0057 "W" */
    0xc0, 0xf3, 0x3c, 0xc9, 0x32, 0x4c, 0x97, 0x25,
    0x69, 0x4e, 0x53, 0x9c, 0xc7, 0x30, 0xcc,

    /* U+0058 "X" */
    0xc3, 0x42, 0x66, 0x24, 0x38, 0x18, 0x18, 0x2c,
    0x64, 0x46, 0xc3, 0x83,

    /* U+0059 "Y" */
    0xc1, 0xa0, 0x98, 0xc4, 0x41, 0x40, 0xa0, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x1, 0x0,

    /* U+005A "Z" */
    0x7f, 0x3, 0x2, 0x4, 0xc, 0x8, 0x18, 0x10,
    0x20, 0x60, 0x40, 0xff,

    /* U+005B "[" */
    0xf8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xf0,

    /* U+005C "\\" */
    0x0, 0x80, 0x40, 0x40, 0x20, 0x20, 0x30, 0x10,
    0x18, 0x8, 0xc, 0x4, 0x6, 0x2, 0x3, 0x0,

    /* U+005D "]" */
    0xf1, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0xf0,

    /* U+005E "^" */
    0x18, 0x38, 0x24, 0x66, 0x42,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0xc6, 0x0,

    /* U+0061 "a" */
    0xfc, 0x2, 0x2, 0x3e, 0xc2, 0x82, 0x82, 0xc6,
    0x7b,

    /* U+0062 "b" */
    0x1, 0x2, 0x4, 0xb, 0x98, 0xa0, 0xc1, 0x83,
    0x6, 0xe, 0x2f, 0x80,

    /* U+0063 "c" */
    0x3d, 0x18, 0x20, 0x82, 0x8, 0x11, 0x3c,

    /* U+0064 "d" */
    0x0, 0x4, 0x8, 0x13, 0xa8, 0xe0, 0xc1, 0x83,
    0x6, 0xa, 0x33, 0xa0,

    /* U+0065 "e" */
    0x38, 0x8a, 0xc, 0x1f, 0xf0, 0x20, 0x20, 0x3c,

    /* U+0066 "f" */
    0x1e, 0x40, 0x81, 0x2, 0x1f, 0x88, 0x10, 0x20,
    0x40, 0x81, 0x2, 0x0,

    /* U+0067 "g" */
    0x1, 0x3f, 0x46, 0x82, 0x82, 0x82, 0xc6, 0x7c,
    0x40, 0x7e, 0x1, 0x81, 0x83, 0x7e,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x61,

    /* U+0069 "i" */
    0x30, 0xc0, 0x0, 0xf0, 0x41, 0x4, 0x10, 0x41,
    0x4, 0xfc,

    /* U+006A "j" */
    0x18, 0xc0, 0xf, 0x84, 0x21, 0x8, 0x42, 0x10,
    0x8c, 0xd8,

    /* U+006B "k" */
    0x81, 0x2, 0x4, 0x8, 0xd1, 0x24, 0x50, 0xa1,
    0x62, 0x64, 0x68, 0x60,

    /* U+006C "l" */
    0xf0, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x80, 0xe0,

    /* U+006D "m" */
    0xbb, 0x66, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x23,
    0x11, 0x88, 0x80,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61, 0x84,

    /* U+006F "o" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa2, 0x38,

    /* U+0070 "p" */
    0xb9, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xe2, 0xf9,
    0x2, 0x4, 0x0,

    /* U+0071 "q" */
    0x3a, 0x8e, 0xc, 0x18, 0x30, 0x60, 0xa3, 0x3a,
    0x4, 0x8, 0x10,

    /* U+0072 "r" */
    0xee, 0x74, 0xc9, 0x2, 0x4, 0x8, 0x10, 0xf0,

    /* U+0073 "s" */
    0x3c, 0x85, 0x3, 0x81, 0xc0, 0xc0, 0xa1, 0x7c,

    /* U+0074 "t" */
    0x20, 0x43, 0xf1, 0x2, 0x4, 0x8, 0x10, 0x20,
    0x40, 0x78,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x63, 0x74,

    /* U+0076 "v" */
    0xc3, 0x42, 0x42, 0x66, 0x24, 0x24, 0x3c, 0x18,
    0x18,

    /* U+0077 "w" */
    0xc0, 0x93, 0x24, 0xc9, 0x32, 0x5c, 0x94, 0xe7,
    0x30, 0xcc, 0x33, 0x0,

    /* U+0078 "x" */
    0x42, 0x66, 0x2c, 0x18, 0x18, 0x3c, 0x24, 0x46,
    0xc3,

    /* U+0079 "y" */
    0xc3, 0x42, 0x42, 0x66, 0x24, 0x24, 0x3c, 0x18,
    0x18, 0x18, 0x10, 0x60, 0x0,

    /* U+007A "z" */
    0x7e, 0xc, 0x30, 0x41, 0x86, 0x8, 0x30, 0xfe,

    /* U+007B "{" */
    0x1c, 0xc2, 0xc, 0x30, 0xc3, 0x30, 0x30, 0xc3,
    0xc, 0x30, 0x83, 0x7,

    /* U+007C "|" */
    0xff, 0xfe,

    /* U+007D "}" */
    0xc0, 0x82, 0x8, 0x20, 0x82, 0x7, 0x20, 0x82,
    0x8, 0x20, 0x82, 0x30,

    /* U+007E "~" */
    0x71, 0x9b, 0x8e,

    /* U+2588 "█" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 158, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 158, .box_w = 2, .box_h = 12, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 158, .box_w = 4, .box_h = 4, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 6, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 158, .box_w = 7, .box_h = 18, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 34, .adv_w = 158, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 158, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 158, .box_w = 1, .box_h = 4, .ofs_x = 4, .ofs_y = 9},
    {.bitmap_index = 63, .adv_w = 158, .box_w = 5, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 73, .adv_w = 158, .box_w = 5, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 83, .adv_w = 158, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 90, .adv_w = 158, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 98, .adv_w = 158, .box_w = 3, .box_h = 6, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 101, .adv_w = 158, .box_w = 7, .box_h = 1, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 102, .adv_w = 158, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 158, .box_w = 8, .box_h = 16, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 120, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 218, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 158, .box_w = 3, .box_h = 9, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 158, .box_w = 3, .box_h = 11, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 238, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 246, .adv_w = 158, .box_w = 6, .box_h = 4, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 249, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 257, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 158, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 281, .adv_w = 158, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 339, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 423, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 158, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 474, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 158, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 158, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 536, .adv_w = 158, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 563, .adv_w = 158, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 577, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 158, .box_w = 4, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 597, .adv_w = 158, .box_w = 8, .box_h = 16, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 613, .adv_w = 158, .box_w = 4, .box_h = 15, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 621, .adv_w = 158, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 626, .adv_w = 158, .box_w = 8, .box_h = 1, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 627, .adv_w = 158, .box_w = 4, .box_h = 3, .ofs_x = 3, .ofs_y = 10},
    {.bitmap_index = 629, .adv_w = 158, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 158, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 158, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 657, .adv_w = 158, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 677, .adv_w = 158, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 158, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 703, .adv_w = 158, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 158, .box_w = 6, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 722, .adv_w = 158, .box_w = 5, .box_h = 16, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 732, .adv_w = 158, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 744, .adv_w = 158, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 756, .adv_w = 158, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 158, .box_w = 6, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 782, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 793, .adv_w = 158, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 804, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 812, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 158, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 158, .box_w = 6, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 158, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 158, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 858, .adv_w = 158, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 158, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 880, .adv_w = 158, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 158, .box_w = 6, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 900, .adv_w = 158, .box_w = 1, .box_h = 15, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 902, .adv_w = 158, .box_w = 6, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 914, .adv_w = 158, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 917, .adv_w = 158, .box_w = 10, .box_h = 19, .ofs_x = 0, .ofs_y = -5}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 9608, .range_length = 1, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

// extern const lv_font_t lv_font_montserrat_16;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_firacode_16 = {
#else
lv_font_t lv_font_firacode_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 5,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_16,
#endif
    // .user_data = NULL,
};



#endif /*#if LV_FONT_FIRACODE_16*/

