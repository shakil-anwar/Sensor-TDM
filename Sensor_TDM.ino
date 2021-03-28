#include "TDM.h"
#include "AVR_Timer1.h"

uint32_t nowUnix = 0;

volatile uint32_t _second = nowUnix;
void setup()
{
  Serial.begin(9600);
  timer1.initialize(1);
  timer1.attachIntCompB(timerIsr);
  timer1.start();
  Serial.println(F("Setup Done"));
  //  tdmBegin(nowUnix);
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
    slot_t slotData;
    tdmGetFreeSlot(id,&slotData);
  }
  else if (cmd == 2)
  {
    
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
