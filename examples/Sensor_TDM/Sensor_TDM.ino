#include "TDM.h"
#include "AVR_Timer1.h"
#include <EEPROM.h>

#define EEPROM_ADDR   0
//void tdmSaveNode(uint32_t addr, struct node_t *node);
//void tdmReadNode(uint32_t addr, struct node_t *node);
void eepromUpdate(uint32_t addr, uint8_t *buf, uint16_t len);
void eepromRead(uint32_t addr, uint8_t *buf, uint16_t len);
uint32_t nowUnix =0;

volatile uint32_t _second = nowUnix;
//slot_t slotData;
uint8_t slotId;
void setup()
{
  Serial.begin(9600);
  SerialBegin(9600); //supporting serial c library
  Serial.println(F("Setup Done"));
  tdmBegin(EEPROM_ADDR, eepromRead, eepromUpdate, 600, 100,20);
//  tdmReset();

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

    slotId = tdmGetFreeSlot(id);
  }
  else if (cmd == 2)
  {
    tdmConfirmSlot(slotId);
  }
}

void timerIsr(void)
{
  _second++;
  Serial.print(F("Sec : ")); Serial.println(_second);
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

void tdmSaveNode(uint32_t addr, struct node_t *node)
{
  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = (uint8_t*)node;
  //  Serial.print(F("TDM EEPROM Saving Addr : ")); Serial.println(eepAddr);
  //  Serial.println(sizeof(struct node_t));
  for (uint8_t i = 0; i < sizeof(struct node_t); i++)
  {
    EEPROM.update(eepAddr + i, *(ptr + i));
    //    Serial.print(*(ptr + i));Serial.print(F("  "));
  }
  //  Serial.println();

}

void tdmReadNode(uint32_t addr, struct node_t *node)
{
  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = (uint8_t*)node;
  //  Serial.print(F("TDM EEPROM Reading Addr : ")); Serial.println(eepAddr);
  //  Serial.println(sizeof(struct node_t));
  for (uint8_t i = 0 ; i < sizeof(struct node_t); i++)
  {
    *(ptr + i) = EEPROM.read(eepAddr + i);
    //    Serial.print(*(ptr + i));Serial.print(F("  "));
  }
  //  Serial.println();
}

void eepromRead(uint32_t addr, uint8_t *buf, uint16_t len)
{

  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = buf;
  Serial.print(F("EEPROM Reading Addr : ")); Serial.println(eepAddr);
  for (uint16_t i = 0 ; i < len; i++)
  {
    *(ptr + i) = EEPROM.read(eepAddr + i);
    Serial.print( *(ptr + i)); Serial.print(F("  "));
  }
  Serial.println();
}

void eepromUpdate(uint32_t addr, uint8_t *buf, uint16_t len)
{

  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = buf;
  Serial.print(F("EEPROM Update Addr : ")); Serial.println(eepAddr);
  for (uint16_t i = 0; i < len; i++)
  {
    EEPROM.update(eepAddr + i, *(ptr + i));
    Serial.print( *(ptr + i)); Serial.print(F("  "));
  }
  Serial.println();
}
