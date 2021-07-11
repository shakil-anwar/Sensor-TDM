#include "TDM.h"
#include "AVR_Timer1.h"
#include <EEPROM.h>

#define SPI_SPEED 1000000UL
#define TDM_EEPROM_ADDR    200
#define MAX_NODE      20
#define TRAY_MAX_NODE 5
#define MOMENT_SEC    60
#define RESERVE_NODE  2
#define TDM_BUF_SZ    (TRAY_MAX_NODE*4+1)
#define TDM_DATA_SAVE_ADDR      0 


#define FLASH1_CS     5
#define FLASH1_HOLD   7



volatile uint8_t tdmBuf[TDM_BUF_SZ];
volatile uint8_t tdmMetaBuf[6];

uint32_t nowUnix = 0;

volatile uint32_t _second = nowUnix;

uint8_t slotId;


Flash flash(FLASH1_CS, FLASH1_HOLD);

void setup()
{
  Serial.begin(115200);
  SerialBegin(0); //supporting serial c library
  Serial.println(F("Setup Done"));

  gpioBegin();


 tdmDebug(true);
 tdmBegin((uint8_t *)tdmBuf,(uint8_t *)tdmMetaBuf, TDM_DATA_SAVE_ADDR, nodeReader, nodeWriter,nodeEraser,
          TDM_EEPROM_ADDR, eepromRead,eepromUpdate,
           MOMENT_SEC, MAX_NODE, RESERVE_NODE, TRAY_MAX_NODE);


  timer1.initialize(1);
  timer1.attachIntCompB(timerIsr);
  timer1.start();
}

void loop()
{
  int cmd = getSerialCmd();
  Serial.println(F("------------------------------"));
  if (cmd == 1)
  {
    Serial.print(F("Id :"));
    int id = getSerialCmd();
    Serial.println(id);

    slotId = tdmGetFreeSlot2(id);
  }
  else if (cmd == 2)
  {
    tdmConfirmSlot2(slotId);
  }

  else if (cmd == 3)
  {
    tdmPrintSlotDetails();
  }

  else if (cmd == 0)
  {
    tdmReset();
    flash.eraseSector(0);
  }
}

void timerIsr(void)
{
  _second++;
  Serial.println(_second); //Serial.println(F("*************"));
  tdmUpdateSlot(_second);
}

int getSerialCmd()
{
  //  Serial.print(F("Input Cmd :"));
  while (!Serial.available())
  {
    delay(10);
  }
  int cmd = Serial.parseInt();
  //  Serial.println(cmd);
  return cmd;
}

void nodeReader(uint32_t addr, uint8_t *buf, uint16_t len)
{
  flash.read(addr, buf, len);
//  Serial.print(F("<====Tail :"));
//  Serial.print(addr);
//  Serial.println(F("====>"));
  
}

void nodeWriter(uint32_t addr, uint8_t *buf, uint16_t len)
{
  flash.write(addr, buf,len);
//  Serial.print(F("<====Head :"));
//  Serial.print(addr);
//  Serial.println(F("====>"));
}
void nodeEraser(uint32_t addr)
{
  flash.eraseSector(addr);
}
void eepromRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = buf;
//  Serial.print(F("EEPROM Read Addr: ")); Serial.println(eepAddr);
  for (uint16_t i = 0 ; i < len; i++)
  {
    *(ptr + i) = EEPROM.read(eepAddr + i);
//    Serial.print( *(ptr + i)); Serial.print(F("  "));
  }
//  Serial.println();
}

void eepromUpdate(uint32_t addr, uint8_t *buf, uint16_t len)
{
  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = buf;
//  Serial.print(F("EEPROM Update Addr: ")); Serial.println(eepAddr);
  for (uint16_t i = 0; i < len; i++)
  {
    EEPROM.update(eepAddr + i, *(ptr + i));
//    Serial.print( *(ptr + i)); Serial.print(F("  "));
  }
//  Serial.println();
}

void gpioBegin()
{
  pinMode(FLASH1_CS,OUTPUT);
  digitalWrite(FLASH1_CS,HIGH);
  flash.begin(SPI_SPEED);
}
