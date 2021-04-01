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
  SerialPrintlnF(P("Loading Saved Slot"));
  _nodeRead(_baseAddr, (uint8_t*)&tdm, sizeof(struct tdm_t));
  printAllSlot();

}

void tdmReset()
{
  SerialPrintlnF(P("Resetting TDM Slot Data"));
  memset(&tdm, 0, sizeof(struct tdm_t));
  _nodeWrite(_baseAddr, (uint8_t*)&tdm, sizeof(struct tdm_t));
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
        SerialPrintlnF(P("Max Node Exceeded----------------->"));
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
    SerialPrintF(P("------------------>Sync :")); SerialPrintlnU8((uint8_t)sync);
    if (sync)
    {
      _currentNode = &tdm.node[_currentSlot];
      printSlot(_currentNode);
      printMomentVar();
      _tdmIsSync = true;
    }
  }

}

void *tdmGetCurrentNode()
{
  return (void*)_currentNode;
}

void tdmGetFreeSlot(uint16_t deviceId, struct slot_t *slot)
{
  uint8_t slotAvail = tdm.meta.freeSlotId;
  SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);
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
    SerialPrintF(P("Confirming Slot : ")); SerialPrintlnU8(slotNo);
    tdm.node[slotNo].isAllotted = 1; // slot allocation ok

    uint32_t memAddr = _baseAddr + slotNo * sizeof(struct node_t);
    SerialPrintF(P("node addr : ")); SerialPrintlnU8(memAddr);
    _nodeWrite(memAddr, (uint8_t*)&tdm.node[slotNo], sizeof(struct node_t));

    //read saved data
    struct node_t nodeBuf;
    _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    printSlot(&nodeBuf);

    //update metadata
    tdm.meta.freeSlotId++;
    memAddr = _baseAddr + sizeof(tdm.node);
    _nodeWrite(memAddr, (uint8_t*)&tdm.meta, sizeof(struct tdmMeta_t));

  }
  else
  {
    SerialPrintF(P("slotNo :")); SerialPrintlnU8(slotNo);
    SerialPrintF(P("tdm.meta.freeSlotId :")); SerialPrintlnU8(tdm.meta.freeSlotId);
  }
}



void printMomentVar()
{
  SerialPrintF(P("_todaySec : ")); SerialPrintlnU32(_todaySec);
  SerialPrintF(P(" | _MomentSec : ")); SerialPrintlnU16(_MomentSec);
  SerialPrintF(P(" | _currentSlot : ")); SerialPrintlnU8(_currentSlot);
}

void printSlot(struct node_t *node)
{
  SerialPrintF(P("Slot No:")); SerialPrintlnU8(node -> slotNo);
  SerialPrintF(P(" | deviceId:")); SerialPrintlnU8(node -> deviceId);
  SerialPrintF(P(" | isAllotted:")); SerialPrintlnU8(node -> isAllotted);
  SerialPrintF(P(" | losSlot:")); SerialPrintlnU8(node -> losSlot);
}

void printAllSlot()
{
  for (uint8_t i = 0; i < MAX_SENSOR_NODE; i++)
  {
    SerialPrintlnU8(i);
    printSlot(&tdm.node[i]);
  }
}
