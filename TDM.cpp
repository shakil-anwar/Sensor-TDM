#include "TDM.h"
/*
 * For each node, allocate 4 bits for status.
 * bit 0 :  alloted status 
 * bit 1 :  connected
 * bit 2~3 : missCount
 */

void printMomentVar();
void tdmTimerInit(uint32_t unixSec);

#define HOUR_SEC      3600UL
#define DAY_TOTAL_SEC (24UL*HOUR_SEC)

#define ALLOT_STATUS   0
#define CONN_STATUS    1
#define MISS_STATUS    2


uint32_t _todaySec;
uint8_t  _currentMoment;
uint16_t _MomentSec;
uint16_t _prevMomentSec;
uint8_t _currentSlot;
bool _tdmIsInit = false;

void tdmBegin(uint32_t unixsec)
{
//  _dayRemSec = unixsec%DAY_TOTAL_SEC;
//  Serial.print(F("Current Moment : ")); Serial.println(_dayRemSec/MOMENT_DURATION_SEC);
}

void tdmTimerInit(uint32_t unixSec)
{
  _todaySec = unixSec%DAY_TOTAL_SEC;
  _currentMoment = _todaySec/MOMENT_DURATION_SEC;
  _MomentSec = _todaySec%MOMENT_DURATION_SEC;
  
}

bool tdmSync(uint32_t unixSec)
{
  _todaySec = unixSec%DAY_TOTAL_SEC;
  _MomentSec = _todaySec%MOMENT_DURATION_SEC;
  _prevMomentSec = _MomentSec;
  _currentSlot = _MomentSec/PER_NODE_INTERVAL_SEC;
  return (_MomentSec%PER_NODE_INTERVAL_SEC == 0); 
}

void tdmUpdateSlot(uint32_t unixSec)
{
  if(_tdmIsInit == false)
  {
    bool sync = tdmSync(unixSec);
    Serial.print(F("Sync :"));Serial.println(sync);
    _tdmIsInit = true;
  }
  else
  {
       _MomentSec++;
       if(_MomentSec - _prevMomentSec >=PER_NODE_INTERVAL_SEC)
       {
          _currentSlot++;
          _prevMomentSec = _MomentSec;
       }
  }
  printMomentVar();
}

void printMomentVar()
{
  Serial.print(F("_todaySec : "));Serial.println(_todaySec);
  Serial.print(F("_MomentSec : "));Serial.println(_MomentSec);
  Serial.print(F("_currentSlot : "));Serial.println(_currentSlot);
}
















/*


void setStatus(uint8_t slot,uint8_t status);


nodeBuf_t nodeBuf;
uint8_t _currentSlot;//current slot available for new registration
uint8_t _availableSlot;//available slot

node_t _node;

void nodeslotBegin()
{
  //parse slot from persistant storage. 
  
  _currentSlot = 0;
  _node.addr = 0;
  _node.slot = 0;
}

void setStatus(uint8_t slot,uint8_t status)
{
  uint8_t byteNo = slot>>1; //divided by 2;
  uint8_t odd = slot - (byteNo<<1);
  uint8_t reg;
  if(odd)
  {
    reg = nodeBuf.status[byteNo]>>4;
  }
  else
  {
    reg = nodeBuf.status[byteNo]>>0;
  }

//  reg = reg
  
  
}

nodeSlot_t *getSlot(uint16_t addr)
{
  _node.addr = addr;
  _node.slot = _currentSlot;
  
  return &_node;
}

void slotConfirmed(node_t *node)
{
  
  _currentSlot++;
}

*/
