#include "TDM.h"
#include "AVR_Timer1.h"
#include <EEPROM.h>

#define EEPROM_ADDR   0
void tdmSaveNode(uint32_t addr, struct node_t *node);
void tdmReadNode(uint32_t addr, struct node_t *node);
uint32_t nowUnix = 0;

volatile uint32_t _second = nowUnix;
slot_t slotData;

void setup()
{
  Serial.begin(9600);
  timer1.initialize(1);
  timer1.attachIntCompB(timerIsr);
  timer1.start();
  Serial.println(F("Setup Done"));
  tdmBegin(EEPROM_ADDR, tdmReadNode, tdmSaveNode);
  
  struct node_t nodeBuf;
  nodeBuf.deviceId  = 20;
  nodeBuf.slotNo = 2;
  nodeBuf.isAllotted = 1;
  nodeBuf.losSlot = 0;
  nodeBuf.reserve = 0;
  tdmSaveNode(10, &nodeBuf);

  struct node_t nodebuf2;
  tdmReadNode(10, &nodebuf2);
  printSlot(&nodebuf2);
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

    tdmGetFreeSlot(id, &slotData);
  }
  else if (cmd == 2)
  {
    tdmConfirmSlot(slotData.slotNo);
  }
}

void timerIsr(void)
{
  _second++;
  //  Serial.print(F("Sec : ")); Serial.println(_second);
  //  tdmUpdateSlot(_second);
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
  Serial.print(F("TDM EEPROM Saving Addr : ")); Serial.println(eepAddr);
  Serial.println(sizeof(struct node_t));
  for (uint8_t i = 0; i < sizeof(struct node_t); i++)
  {
    EEPROM.update(eepAddr+i, *(ptr + i));
    Serial.print(*(ptr + i));Serial.print(F("  "));
  }
  Serial.println();

}

void tdmReadNode(uint32_t addr, struct node_t *node)
{
  uint16_t eepAddr = (uint16_t)addr;
  uint8_t *ptr = (uint8_t*)node;
  Serial.print(F("TDM EEPROM Reading Addr : ")); Serial.println(eepAddr);
  Serial.println(sizeof(struct node_t));
  for (uint8_t i = 0 ; i < sizeof(struct node_t); i++)
  {
    *(ptr + i) = EEPROM.read(eepAddr + i);
    Serial.print(*(ptr + i));Serial.print(F("  "));
  }
  Serial.println();
}
