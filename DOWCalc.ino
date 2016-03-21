/*
Look for faster ways to calculate the day of the week on Arduino
   by Matt Weber (matthew.d.weber@ieee.org) (linkedin.com/in/mattweberdesign)
 Written to give Max a helping hand
  http://www.embedded.com/electronics-blogs/max-unleashed-and-unfettered/4441655/Cunning-Chronograph--Outfoxed-by-Daylight-Saving-Time
  http://www.embedded.com/electronics-blogs/max-unleashed-and-unfettered/4441673/Is-that-the-daylight-saving-time-

 Uses math tricks from these two spots
  http://www.bowens.plus.com/code/integer_division_uint16.h
  http://www.hackersdelight.org/divcMore.pdf

*/

#define U16REC1(A, M, S) (uint16_t)((((uint32_t)(A) * (uint32_t)(M)) >> 16u) >> (S))
#define U16REC2(A, M, S) (uint16_t)((((((uint32_t)(A) * (uint32_t)(M)) >> 16u) + (A)) >> 1u) >> (S))
#define U16DIVBY9(A)    U16REC1(A, 0xE38Fu, 3u)
#define U16DIVBY100(A)    U16REC2(A, 0x47AFu, 6u)

const uint8_t jan=1;
const uint8_t feb=jan+1;
const uint8_t mar=feb+1;
const uint8_t apr=mar+1;
const uint8_t may=apr+1;
const uint8_t jun=may+1;
const uint8_t jul=jun+1;
const uint8_t aug=jul+1;
const uint8_t sep=aug+1;
const uint8_t oct=sep+1;
const uint8_t nov=oct+1;
const uint8_t dec=nov+1;
const uint16_t startYear = 2000;
const uint16_t endYear   = 2099;

void setup() {
  Serial.begin(115200);
  uint8_t res = 0;
  uint32_t startMillis = millis(); 
  // Test by measuring time to calculate doy of week for all dates in this century
  for (uint16_t y = startYear;y<=endYear;y++) {
    for (uint8_t m=jan; m<=dec; m++) {
      for (uint8_t d=1; d<=31;d++) { // gets invalid dates like April 31 and Feb 30, etc but math should still work okay
          res ^= getDOW(y,m,d); // accumulate so compiler doesn't optimize it out
      }
    }
  }
  uint32_t endMillis = millis(); 
  Serial.print(res);
  Serial.print("  getDOW      time(ms): "); Serial.println(endMillis-startMillis);
  res = 0;
  startMillis = millis(); 
  for (uint16_t y = startYear;y<=endYear;y++) {
    for (uint8_t m=jan; m<=dec; m++) {
      for (uint8_t d=1; d<=31;d++) {
          res ^= getDOWFast(y,m,d); // accumulate so compiler doesn't optimize it out
          //if (getDOWFast3(y,m,d) != getDOW(y,m,d)) {Serial.print("Error ");Serial.println(y);}
      }
    }
  }
  endMillis = millis(); 
  Serial.print(res);
  Serial.print("  getDOWFast  time(ms): "); Serial.println(endMillis-startMillis);
  res = 0;
  startMillis = millis(); 
  for (uint16_t y = startYear;y<=endYear;y++) {
    for (uint8_t m=jan; m<=dec; m++) {
      for (uint8_t d=1; d<=31;d++) {
          res ^= getDOWFast2(y,m,d); // accumulate so compiler doesn't optimize it out
          //if (getDOWFast3(y,m,d) != getDOW(y,m,d)) {Serial.print("Error ");Serial.println(y);}
      }
    }
  }
  endMillis = millis(); 
  Serial.print(res);
  Serial.print("  getDOWFast2 time(ms): "); Serial.println(endMillis-startMillis);
  res = 0;
  startMillis = millis(); 
  for (uint16_t y = startYear;y<=endYear;y++) {
    for (uint8_t m=jan; m<=dec; m++) {
      for (uint8_t d=1; d<=31;d++) {
          res ^= getDOWFast3(y,m,d); // accumulate so compiler doesn't optimize it out
          //if (getDOWFast3(y,m,d) != getDOW(y,m,d)) {Serial.print("Error ");Serial.println(y);}
      }
    }
  }
  endMillis = millis(); 
  Serial.print(res);
  Serial.print("  getDOWFast3 time(ms): "); Serial.println(endMillis-startMillis);
}

void loop() {
}

uint8_t getDOW (uint16_t y, uint8_t m, uint8_t d) {
    uint16_t d16 = d; // need uint8_t isn't big enough to hold d+=m<3?y--:y-2
    return (d16+=m<3?y--:y-2,23*m/9+d16+4+y/4-y/100+y/400)%7;
}

uint8_t getDOWFast (uint16_t y, uint8_t m, uint8_t d) {
    uint16_t yDiv100 = U16DIVBY100(y);
    uint8_t m23Div9 = U16DIVBY9(23*m);
    uint16_t tmp = d + (m<3?y--:y-2);
    uint32_t tmp2 = m23Div9+tmp+4+y/4-yDiv100+yDiv100/4;
    // Fast modulo 7 function from here http://www.hackersdelight.org/divcMore.pdf
    //tmp2 = (0x24924924*tmp2 + (tmp2 >> 1) + (tmp2 >> 4)) >> 29;
    // with 16 bit y, tmp2 won't get large enough to need the accuracy terms
    tmp2 = (0x24924924*tmp2) >> 29;
    return tmp2 & ((int32_t)(tmp2 - 7) >> 31);
}
uint8_t getDOWFast2 (uint16_t y, uint8_t m, uint8_t d) {
    uint16_t yorig = y;
    uint8_t m23Div9 = U16DIVBY9(23*m);
    uint16_t tmp = d + (m<3?y--:y-2); 
    //  When y starts to approach 65,535 tmp2 can overflow
    //  a uint16_t but we don't need it to work nearly that
    //  far out
    uint16_t tmp2 = m23Div9+tmp+y/4+4;
    tmp2 = tmp2 - 2000/100 + 2000/400;  // Leap day adjustment. Optimize speed for common case (this century)
    if ((y <= 2000) || (y >= 2100)) { // correction if not the common case
      uint8_t yDiv100 = U16DIVBY100(yorig);  // uint8_t = assumtion we don't need to go beyond year 25,500
      tmp2 = tmp2 + 2000/100 - 2000/400 - yDiv100 + yDiv100/4;      
    }
    // Fast modulo 7 function from here http://www.hackersdelight.org/divcMore.pdf
    // Unsigned remainder modulo 7, digit summing method
    static char table[75] = {0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1,2,3,4,5,6, 0,1,2,3,4};
    tmp2 = (tmp2 >> 15) + (tmp2 & 0x7FFF); // Max 0x27FFE.
    tmp2 = (tmp2 >> 9) + (tmp2 & 0x01FF); // Max 0x33D.
    tmp2 = (tmp2 >> 6) + (tmp2 & 0x003F); // Max 0x4A.
    return table[tmp2];
}
uint8_t getDOWFast3 (uint16_t y, uint8_t m, uint8_t d) {
    // Only handling y in range of 2000-2099 allows us to speed things
    //  up by using 8 bit math and avoiding adjustment for leap
    //  year on the centuries.
    uint8_t yAdjusted = y-1996;
    uint8_t m23Div9 = U16DIVBY9(23*m);
    uint8_t tmp = d + (m<3?yAdjusted--:yAdjusted-2); 
    uint8_t tmp2 = m23Div9+tmp+yAdjusted/4;
    // rotated from modulo 7 table to get rid of a subtraction
    static char table[66] = {6, 0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1,2,3,4,5,6, 0,1,2,3,4,5,6,
        0,1,2,3,4,5,6, 0,1};
    tmp2 = (tmp2 >> 6) + (tmp2 & 0x3F);
    return table[tmp2];
}

