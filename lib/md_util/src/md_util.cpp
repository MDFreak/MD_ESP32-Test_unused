#include "md_util.h"
#include "wire.h"


// TwoWire I2Cone = TwoWire(0);
// TwoWire I2Ctwo = TwoWire(1);

//--------------------------
// Setzen / Loeschen eines Bit in einer 16-Bit Flags-Wort
uint16_t setBit(const uint16_t inWert, const uint16_t inBit, const bool inVal)
  {
    int ret;
    if (inVal)    // Error gefunden
    { // Fehler setzen
      ret = inWert | inBit;
    }
    else
    { // Fehler loeschen
      ret = inWert & (0xffff ^ inBit);
    }
  /*
    #ifdef SERIAL_DEBUG
      Serial.print("  inWert="); Serial.print(inWert);
      Serial.print("  inBit="); Serial.print(inBit);
      Serial.print("  !inBit="); Serial.print(0xffff ^ inBit);
      Serial.print("  inVal="); Serial.print(inVal);
      Serial.print("  ret="); Serial.print(ret);
    #endif
  */
    return ret;
  }

//--------------------------
// scan IIC - serial output
uint8_t scanIIC(uint8_t no, uint8_t start, uint8_t sda, uint8_t scl)
  {
    uint8_t i = 0;
    TwoWire I2C = TwoWire(no-1);
    I2C.begin(sda,scl,400000);
    Serial.println();
    Serial.print("Scanning I2C Addresses Channel "); Serial.print(no);
    //uint8_t cnt=0;
    for(i = start; i < 128 ; i++)
      {
        I2C.beginTransmission(i);
        uint8_t ec=I2C.endTransmission(true);
        if(ec==0)
          {
            Serial.print(" device at address 0x");
            if (i<16) Serial.print('0');
            Serial.print(i, HEX);
            break;
          }
      }
    I2C.~TwoWire();
    Serial.println();
    return i;
  }

// class msTimer
msTimer::msTimer()
  {
    startT(0);
  }

msTimer::msTimer(const unsigned long inTOut)
  {
    startT(inTOut);
  }


bool msTimer::TOut()
  {
    if ((millis() - _tstart) > _tout)
      return true;
    else
      return false;
  }

void msTimer::startT()
  {
    _tstart = millis();
  }

void msTimer::startT(const unsigned long inTOut)
  {
    _tstart = millis();
    _tout   = inTOut;
  }