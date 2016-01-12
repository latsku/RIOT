#include "hd44780.h"
#include "hd44780_internal.h"


// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

void LCD_init(uint32_t rs, uint32_t rw, uint32_t enable,
              uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3)
{
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;

  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3;
//  _data_pins[4] = d4;
//  _data_pins[5] = d5;
//  _data_pins[6] = d6;
//  _data_pins[7] = d7;

  int fourbitmode = 1;

  if (fourbitmode)
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  else
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;

  LCD_begin(16, 1, LCD_5x10DOTS);
}

void LCD_begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;

  LCD_setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  gpio_init(_rs_pin, GPIO_DIR_OUT, GPIO_NOPULL);
  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (_rw_pin != 255) {
    gpio_init(_rw_pin, GPIO_DIR_OUT, GPIO_NOPULL);
  }
  gpio_init(_enable_pin, GPIO_DIR_OUT, GPIO_NOPULL);

  // Do these once, instead of every time a character is drawn for speed reasons.
  for (int i=0; i<((_displayfunction & LCD_8BITMODE) ? 8 : 4); ++i)
  {
    gpio_init(_data_pins[i], GPIO_DIR_OUT, GPIO_NOPULL);
   }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
  xtimer_usleep(50000);
  // Now we pull both RS and R/W low to begin commands
  gpio_write(_rs_pin, LOW);
  gpio_write(_enable_pin, LOW);
  if (_rw_pin != 255) {
    gpio_write(_rw_pin, LOW);
  }

  //put the LCD into 4 bit or 8 bit mode
  if (! (_displayfunction & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    LCD_write4bits(0x03);
    xtimer_usleep(4500); // wait min 4.1ms

    // second try
    LCD_write4bits(0x03);
    xtimer_usleep(4500); // wait min 4.1ms

    // third go!
    LCD_write4bits(0x03);
    xtimer_usleep(150);

    // finally, set to 4-bit interface
    LCD_write4bits(0x02);
  } else {
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    LCD_command(LCD_FUNCTIONSET | _displayfunction);
    xtimer_usleep(4500);  // wait more than 4.1ms

    // second try
    LCD_command(LCD_FUNCTIONSET | _displayfunction);
    xtimer_usleep(150);

    // third go
    LCD_command(LCD_FUNCTIONSET | _displayfunction);
  }

  // finally, set # lines, font size, etc.
  LCD_command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  LCD_display();

  // clear it off
  LCD_clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  LCD_command(LCD_ENTRYMODESET | _displaymode);

}

void LCD_setRowOffsets(int row0, int row1, int row2, int row3)
{
  _row_offsets[0] = row0;
  _row_offsets[1] = row1;
  _row_offsets[2] = row2;
  _row_offsets[3] = row3;
}

/********** high level commands, for the user! */
void LCD_clear(void)
{
  LCD_command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  xtimer_usleep(2000);  // this command takes a long time!
}

void LCD_home(void)
{
  LCD_command(LCD_RETURNHOME);  // set cursor position to zero
  xtimer_usleep(2000);  // this command takes a long time!
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
  const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  if ( row >= max_lines ) {
    row = max_lines - 1;    // we count rows starting w/0
  }
  if ( row >= _numlines ) {
    row = _numlines - 1;    // we count rows starting w/0
  }

  LCD_command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void LCD_noDisplay(void) {
  _displaycontrol &= ~LCD_DISPLAYON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_display(void) {
  _displaycontrol |= LCD_DISPLAYON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCD_noCursor(void) {
  _displaycontrol &= ~LCD_CURSORON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_cursor(void) {
  _displaycontrol |= LCD_CURSORON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LCD_noBlink(void) {
  _displaycontrol &= ~LCD_BLINKON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_blink(void) {
  _displaycontrol |= LCD_BLINKON;
  LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LCD_scrollDisplayLeft(void) {
  LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LCD_scrollDisplayRight(void) {
  LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LCD_leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LCD_rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LCD_autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LCD_noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LCD_createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  LCD_command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    LCD_write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void LCD_command(uint8_t value) {
  LCD_send(value, LOW);
}

inline size_t LCD_write(uint8_t value) {
  LCD_send(value, HIGH);
  return 1; // assume sucess
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LCD_send(uint8_t value, uint8_t mode) {
  gpio_write(_rs_pin, mode);

  // if there is a RW pin indicated, set it low to Write
  if (_rw_pin != 255) {
    gpio_write(_rw_pin, LOW);
  }

  if (_displayfunction & LCD_8BITMODE) {
    LCD_write8bits(value);
  } else {
    LCD_write4bits(value>>4);
    LCD_write4bits(value);
  }
}

void LCD_pulseEnable(void) {
  gpio_write(_enable_pin, LOW);
  xtimer_usleep(1);
  gpio_write(_enable_pin, HIGH);
  xtimer_usleep(1);    // enable pulse must be >450ns
  gpio_write(_enable_pin, LOW);
  xtimer_usleep(100);   // commands need > 37us to settle
}

void LCD_write4bits(uint8_t value) {
  for (int i = 0; i < 4; i++) {
    gpio_write(_data_pins[i], (value >> i) & 0x01);
  }

  LCD_pulseEnable();
}

void LCD_write8bits(uint8_t value) {
  for (int i = 0; i < 8; i++) {
    gpio_write(_data_pins[i], (value >> i) & 0x01);
  }

  LCD_pulseEnable();
}
