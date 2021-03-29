#include "TDM.h"
/*
   For each node, allocate 4 bits for status.
   bit 0 :  alloted status
   bit 1 :  connected
   bit 2~3 : missCount
*/

void printMomentVar();
void printAllSlot();

#define HOUR_SEC      3600UL
#define DAY_TOTAL_SEC (24UL*HOUR_SEC)

#define ALLOT_STATUS   0
#define CONN_STATUS    1
#define MISS_STATUS    2


uint32_t _todaySec;
uint16_t _MomentSec;
uint16_t _prevMomentSec;
uint8_t  _currentMomentNo;


uint8_t _currentSlot;
bool _tdmIsSync = false;
struct node_t *_currentNode;
struct tdm_t tdm;


uint32_t _baseAddr;
tdmMemFun_t _nodeRead;
tdmMemFun_t _nodeWrite;

void tdmBegin(uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite)
{

  _baseAddr = baseAddr;
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;

  //  read slot from eeprom into ram
  Serial.println(F("Loading Saved Slot"));
  _nodeRead(_baseAddr, (uint8_t*)&tdm, sizeof(tdm_t));
  printAllSlot();

}

void tdmReset()
{
  Serial.println(F("Resetting TDM Slot Data"));
  memset(&tdm, 0, sizeof(tdm_t));
  _nodeWrite(_baseAddr, (uint8_t*)&tdm, sizeof(tdm_t));
  //  _nodeRead(_baseAddr, (uint8_t*)&tdm, sizeof(tdm_t));
}

bool tdmSync(uint32_t unixSec)
{
  _todaySec = unixSec % DAY_TOTAL_SEC;                //calculate today remaining second
  _MomentSec = _todaySec % MOMENT_DURATION_SEC;       //Calculate current moment second
  _prevMomentSec = _MomentSec;
  _currentSlot = _MomentSec / PER_NODE_INTERVAL_SEC;  //calculate current slot no
  return (_MomentSec % PER_NODE_INTERVAL_SEC == 0);   //starting of new slot returns true
}

void tdmUpdateSlot(uint32_t unixSec)
{
  if (_tdmIsSync)
  {
    _MomentSec++;
    if (_MomentSec - _prevMomentSec >= PER_NODE_INTERVAL_SEC)
    {
      _currentSlot++;
      if (_currentSlot > MAX_SENSOR_NODE - 1)
      {
        Serial.println(F("Max Node Exceeded----------------->"));
        //Start a new momenet and update time
        _MomentSec = 0;
        _currentSlot = 0;
        //        tdmSync(unixSec);
      }
      _currentNode = &tdm.node[_currentSlot];
      printSlot(_currentNode);
      printMomentVar();
      _prevMomentSec = _MomentSec;
    }
  }
  else
  {
    bool sync = tdmSync(unixSec);
    Serial.print(F("------------------>Sync :")); Serial.println(sync);
    if (sync)
    {
      _currentNode = &tdm.node[_currentSlot];
      printSlot(_currentNode);
      printMomentVar();
      _tdmIsSync = true;
    }
  }

}

void tdmGetFreeSlot(uint16_t deviceId, struct slot_t *slot)
{
  //  uint8_t slotAvail = tdm.freeSlot;
  uint8_t slotAvail = tdm.meta.freeSlotId;
  Serial.print(F("slot Avail :")); Serial.println(slotAvail);
  if (slotAvail < MAX_SENSOR_NODE)
  {
    //fill up node info
    tdm.node[slotAvail].deviceId = deviceId;
    tdm.node[slotAvail].slotNo = slotAvail;
    printSlot(&tdm.node[slotAvail]);

    slot -> slotNo = slotAvail;
    slot -> momentDuration = MOMENT_DURATION_SEC;
    slot -> perNodeInterval = PER_NODE_INTERVAL_SEC;
  }

}

void tdmConfirmSlot(uint8_t slotNo)
{
  if (slotNo == tdm.meta.freeSlotId)
  {
    Serial.print(F("Confirming Slot : ")); Serial.println(slotNo);
    tdm.node[slotNo].isAllotted = 1; // slot allocation ok

    uint32_t memAddr = _baseAddr + slotNo * sizeof(struct node_t);
    Serial.print(F("node addr : ")); Serial.println(memAddr);
    _nodeWrite(memAddr, (uint8_t*)&tdm.node[slotNo], sizeof(struct node_t));

    //read saved data
    node_t nodeBuf;
    _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    printSlot(&nodeBuf);

    tdm.meta.freeSlotId++;
    memAddr = _baseAddr + sizeof(tdm.node);
    _nodeWrite(memAddr, (uint8_t*)&tdm.meta, sizeof(struct tdmMeta_t));

  }
  else
  {
    Serial.print(F("slotNo :")); Serial.println(slotNo);
    Serial.print(F("tdm.meta.freeSlotId :")); Serial.println(tdm.meta.freeSlotId);
  }
}



void printMomentVar()
{
  Serial.print(F("_todaySec : ")); Serial.print(_todaySec);
  Serial.print(F(" | _MomentSec : ")); Serial.print(_MomentSec);
  Serial.print(F(" | _currentSlot : ")); Serial.println(_currentSlot);
}

void printSlot(struct node_t *node)
{
  Serial.println(F("<--------Current Slot---------->"));
  Serial.print(F("deviceId:")); Serial.println(node -> deviceId);
  Serial.print(F("slotNo:")); Serial.println(node -> slotNo);
  Serial.print(F("isAllotted:")); Serial.println(node -> isAllotted);
  Serial.print(F("losSlot:")); Serial.println(node -> losSlot);
}

void printAllSlot()
{
  for (uint8_t i = 0; i < MAX_SENSOR_NODE; i++)
  {
    Serial.println(i);
    printSlot(&tdm.node[i]);
  }
}













/*
  //void tdmTimerInit(uint32_t unixSec)
  //{
  //  _todaySec = unixSec%DAY_TOTAL_SEC;
  //  _currentMomentNo = _todaySec/MOMENT_DURATION_SEC;
  //  _MomentSec = _todaySec%MOMENT_DURATION_SEC;
  //
  //}

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
