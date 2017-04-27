#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <mraa/i2c.h>
#include "lcd.h"

void send(uint8_t, uint8_t);
void setReg(unsigned char addr, unsigned char dta);

mraa_i2c_context i2c;
uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;

uint8_t _initialized;

uint8_t _numlines,_currline;

void i2c_send_byte(unsigned char dta)
{
//    Wire.beginTransmission(LCD_ADDRESS);        // transmit to device #4
//    Wire.write(dta);                            // sends five bytes
//    Wire.endTransmission();                     // stop transmitting
    mraa_i2c_address(i2c, LCD_ADDRESS);
    mraa_i2c_write_byte(i2c, dta);

}

void i2c_send_byteS(unsigned char *dta, unsigned char len)
{
//    Wire.beginTransmission(LCD_ADDRESS);        // transmit to device #4
//    for(int i=0; i<len; i++)
//    {
//        Wire.write(dta[i]);
//    }
//    Wire.endTransmission();                     // stop transmitting
    mraa_i2c_address(i2c, LCD_ADDRESS);
    mraa_i2c_write(i2c, dta, len);
}

void begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    
    //Wire.begin();
    mraa_init();
    i2c = mraa_i2c_init(1);
    
    if (lines > 1) {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;
    _currline = 0;
    
    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != 0) && (lines == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }
    
    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
 //   delayMicroseconds(50000);
    usleep(50000);
    
    
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23
    
    // Send function set command sequence
    command(LCD_FUNCTIONSET | _displayfunction);
//    delayMicroseconds(4500);  // wait more than 4.1ms
    usleep(4500);
  
    // second try
    command(LCD_FUNCTIONSET | _displayfunction);
//    delayMicroseconds(150);
    usleep(150);    

    // third go
    command(LCD_FUNCTIONSET | _displayfunction);
    
    
    // finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);
    
    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();
    
    // clear it off
    clear();
    
    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
    
    
    // backlight init
    setReg(REG_MODE1, 0);
    // set LEDs controllable by both PWM and GRPPWM registers
    setReg(REG_OUTPUT, 0xFF);
    // set MODE2 values
    // 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
    setReg(REG_MODE2, 0x20);
    
    setColorWhite();
    
}

/********** high level commands, for the user! */
void clear()
{
    command(LCD_CLEARDISPLAY);        // clear display, set cursor position to zero
//    delayMicroseconds(2000);          // this command takes a long time!
    usleep(2000);
}

void home()
{
    command(LCD_RETURNHOME);        // set cursor position to zero
//    delayMicroseconds(2000);        // this command takes a long time!
    usleep(2000);
}

void setCursor(uint8_t col, uint8_t row)
{
    
    unsigned char dta[2] = {0x80, col};
    col = (row == 0 ? col|0x80 : col|0xc0);
    
    i2c_send_byteS(dta, 2);
    
}

// Turn the display on/off (quickly)
void noDisplay()
{
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void display() {
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void noCursor()
{
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void cursor() {
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void noBlink()
{
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void blink()
{
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void leftToRight(void)
{
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(void)
{
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(void)
{
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(void)
{
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void createChar(uint8_t location, uint8_t charmap[])
{
    int i;    
    unsigned char dta[9];
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    
    
    dta[0] = 0x40;
    for(i=0; i<8; i++)
    {
        dta[i+1] = charmap[i];
    }
    i2c_send_byteS(dta, 9);
}

// Control the backlight LED blinking
void blinkLED(void)
{
    // blink period in seconds = (<reg 7> + 1) / 24
    // on/off ratio = <reg 6> / 256
    setReg(0x07, 0x17);  // blink every second
    setReg(0x06, 0x7f);  // half on, half off
}

void noBlinkLED(void)
{
    setReg(0x07, 0x00);
    setReg(0x06, 0xff);
}

/*********** mid level commands, for sending data/cmds */

// send command
inline void command(uint8_t value)
{
    unsigned char dta[2] = {0x80, value};
    i2c_send_byteS(dta, 2);
}

size_t printNumber(unsigned long n)
{
    char *cur;
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];
    int base = 10;
    int count = 0;
    *str = '\0';
    
    do {
        char c = n % base;
        n /= base;
        
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
        count++;
    } while(n);
    
    for(cur = str; *cur != '\0'; cur++)
    {
        print(*cur);
    }
    return count;
}

size_t printString(char* string )
{
	int i=0;
	for(i=0;i<strlen(string);i++){
		print(string[i]);
	}
}

size_t printFloat(float number)
{
    uint8_t i;
    size_t n = 0;
    int digits = 1;
    double rounding = 0.5;
    unsigned long int_part;
    double remainder;
    
    // Handle negative numbers
    if (number < 0.0)
    {
        n += print('-');
        number = -number;
    }
    
    // Round correctly so that print(1.999, 2) prints as "2.00"
    
    for (i=0; i<digits; ++i)
        rounding /= 10.0;
    
    number += rounding;
    
    // Extract the integer part of the number and print it
    int_part = (unsigned long)number;
    remainder = number - (double)int_part;
    n += printNumber(int_part);
    
    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
        n += print('.');
    }
    
    // Extract digits from the remainder one at a time
    while (digits-- > 0)
    {
        unsigned int toPrint;
        remainder *= 10.0;
        toPrint = (unsigned int)(remainder);
        n += printNumber(toPrint);
        remainder -= toPrint; 
    } 
    
    return n;
}

// send data
inline size_t print(uint8_t value)
{
    
    unsigned char dta[2] = {0x40, value};
    i2c_send_byteS(dta, 2);
    return 1; // assume sucess
}

void setReg(unsigned char addr, unsigned char dta)
{
//    Wire.beginTransmission(RGB_ADDRESS); // transmit to device #4
//    Wire.write(addr);
//    Wire.write(dta);
//    Wire.endTransmission();    // stop transmitting
    mraa_i2c_address(i2c, RGB_ADDRESS);
    mraa_i2c_write_byte(i2c, addr);
    mraa_i2c_write_byte(i2c, dta);
}

void setRGB(unsigned char r, unsigned char g, unsigned char b)
{
    setReg(REG_RED, r);
    setReg(REG_GREEN, g);
    setReg(REG_BLUE, b);
}

void setPWM(unsigned char color, unsigned char pwm)
{
    setReg(color, pwm);
}

const unsigned char color_define[4][3] =
{
    {255, 255, 255},            // white
    {255, 0, 0},                // red
    {0, 255, 0},                // green
    {0, 0, 255},                // blue
};

void setColor(unsigned char color)
{
    if(color > 3)return ;
    setRGB(color_define[color][0], color_define[color][1], color_define[color][2]);
}

void setColorAll(void)
{
    setRGB(0, 0, 0);
}

void setColorWhite(void)
{
    setRGB(255, 255, 255);	
}
