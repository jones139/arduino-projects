
#include <avr/pgmspace.h>

#define P(name) const prog_uchar name[] PROGMEM   // declare a program memory string for use by serialPrintP etc.

int memoryFree();  // return the number of bytes of memory free.
void serialPrintP(const prog_uchar *str);
//void lcdPrintP(const prog_uchar *str);



