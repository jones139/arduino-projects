#include <Arduino.h>
#include <WProgram.h>
#include <LiquidCrystal.h>
#include "memUtils.h"


/* return the number of bytes of free RAM.
 * From the book "Arduino Cookbook" by Michael Margolis (O'Riley)
 */
 extern int __bss_end;
 extern void *__brkval;
int memoryFree() {
 int freeValue;
 
 if ((int)__brkval == 0)
  freeValue = ((int)&freeValue) - ((int)&__bss_end);
 else
  freeValue = ((int)&freeValue) - ((int)__brkval);
 return(freeValue);    
}

void serialPrintP(const prog_uchar *str) {
  char c;
  while ((c = pgm_read_byte(str++)))
    Serial.write(c);
}

/*void lcdPrintP(const prog_uchar *str) {
 * char c;
 * while ((c = pgm_read_byte(str++)))
 *   lcd.write(c);
 *}
 */
