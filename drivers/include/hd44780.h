#ifndef hd44780_h
#define hd44780_h

#include <inttypes.h>


#include "periph/gpio.h"
#include "xtimer.h"

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00


  void LCD_init(uint32_t rs, uint32_t rw, uint32_t enable,
                uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3);

  void LCD_begin(uint8_t cols, uint8_t rows, uint8_t charsize);

  void LCD_clear(void);
  void LCD_home(void);

  void LCD_noDisplay(void);
  void LCD_display(void);
  void LCD_noBlink(void);
  void LCD_blink(void);
  void LCD_noCursor(void);
  void LCD_cursor(void);
  void LCD_scrollDisplayLeft(void);
  void LCD_scrollDisplayRight(void);
  void LCD_leftToRight(void);
  void LCD_rightToLeft(void);
  void LCD_autoscroll(void);
  void LCD_noAutoscroll(void);

  void LCD_setRowOffsets(int row1, int row2, int row3, int row4);
  void LCD_createChar(uint8_t, uint8_t[]);
  void LCD_setCursor(uint8_t, uint8_t);
  size_t LCD_write(uint8_t);
  void LCD_command(uint8_t);

#endif
