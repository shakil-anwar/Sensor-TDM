#include "TDM.h"
#include "AVR_Timer1.h"

uint32_t nowUnix = 2000;

volatile uint32_t _second;
void setup()
{
  Serial.begin(9600);
  timer1.initialize(1);
  timer1.attachIntCompB(timerIsr);
  timer1.start();//start rt timer
  Serial.println(F("Setup Done"));
  tdmBegin(nowUnix);
//  tdmUpdateSlot(nowUnix);
}

void loop()
{

}

void timerIsr(void)
{
  _second++;
  Serial.print(F("Sec : "));Serial.println(_second);
  tdmUpdateSlot(_second);
}
