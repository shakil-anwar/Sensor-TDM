#include "TDM.h"
/*
   For each node, allocate 4 bits for status.
   bit 0 :  alloted status
   bit 1 :  connected
   bit 2~3 : missCount
*/

void printAllSlot();

#define HOUR_SEC      3600UL
#define DAY_TOTAL_SEC (24UL*HOUR_SEC)


// uint32_t _todaySec;
// uint8_t  _currentMomentNo; 
// struct node_t *_currentNode;


struct tdm_t tdm;
bool _tdmIsSync = false;

uint16_t _MomentSec;
uint16_t _prevMomentSec;
uint8_t _currentSlot;

uint32_t _baseAddr;
tdmMemFun_t _nodeRead;
tdmMemFun_t _nodeWrite;


void tdmBegin(uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite,
              uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot)
{
  _baseAddr = baseAddr;
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;

   //  read slot from eeprom into ram
  SerialPrintlnF(P("Loading Saved Slot"));
  _nodeRead(_baseAddr, (uint8_t*)&tdm, sizeof(struct tdm_t));
  printAllSlot();

  tdm.meta.maxNode = maxNode;
  tdm.meta.momentDuration = momentDuration;
  tdm.meta.reserveSlot = reserveSlot;
  tdm.meta.perNodeInterval = (momentDuration/maxNode);

  SerialPrintF(P("Max Node Number: "));
  SerialPrintlnU8(tdm.meta.maxNode);
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
  uint32_t todaySec = unixSec % DAY_TOTAL_SEC;                //calculate today remaining second
  SerialPrintF(P("todaySec : ")); SerialPrintlnU32(todaySec);
  _MomentSec = todaySec % tdm.meta.momentDuration;       //Calculate current moment second
  _prevMomentSec = _MomentSec;
  _currentSlot = _MomentSec / tdm.meta.perNodeInterval;  //calculate current slot no
  return (_MomentSec % tdm.meta.perNodeInterval == 0);   //starting of new slot returns true
}

void tdmUpdateSlot(uint32_t unixSec)
{
  if (_tdmIsSync)
  {
    _MomentSec++;
    if (_MomentSec - _prevMomentSec >= tdm.meta.perNodeInterval)
    {
      _currentSlot++;
      if (_currentSlot > tdm.meta.maxNode - 1)
      {
        SerialPrintlnF(P("Max Node Exceeded----------------->"));
        //Start a new momenet and update time
        _MomentSec = 0;
        _currentSlot = 0;
        //        tdmSync(unixSec);
      }
      // _currentNode = &tdm.node[_currentSlot];
      printSlot(&tdm.node[_currentSlot]);
      
      // printMomentVar();
      _prevMomentSec = _MomentSec;
    }
  }
  else
  {
    bool sync = tdmSync(unixSec);
    SerialPrintF(P("------------------>Sync :")); SerialPrintlnU8((uint8_t)sync);
    if (sync)
    {
      // _currentNode = &tdm.node[_currentSlot];
      SerialPrintF(P("_MomentSec : ")); SerialPrintlnU16(_MomentSec);
      printSlot(&tdm.node[_currentSlot]);
      // printMomentVar();
      _tdmIsSync = true;
    }
  }

}

struct node_t *tdmGetCurrentNode()
{
  return &tdm.node[_currentSlot];
}


uint8_t tdmGetFreeSlot(uint16_t deviceId)
{
  uint8_t slotAvail = tdm.meta.freeSlotId;
  SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);
  if (slotAvail < tdm.meta.maxNode)
  {
    //fill up node info
    tdm.node[slotAvail].deviceId = deviceId;
    tdm.node[slotAvail].slotNo = slotAvail;
    printSlot(&tdm.node[slotAvail]);

    return slotAvail;
  }
  return 255; //invalid slot
}

bool tdmConfirmSlot(uint8_t slotNo)
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



void printSlot(struct node_t *node)
{
  SerialPrintF(P("Slot No:")); SerialPrintU8(node -> slotNo);
  SerialPrintF(P(" | deviceId:")); SerialPrintU8(node -> deviceId);
  SerialPrintF(P(" | isAllotted:")); SerialPrintU8(node -> isAllotted);
  SerialPrintF(P(" | losSlot:")); SerialPrintlnU8(node -> losSlot);
}

void printAllSlot()
{
  SerialPrintF(P("Max Node Number: "));
  SerialPrintlnU8(tdm.meta.maxNode);
  for ( uint8_t i= 0; i < tdm.meta.maxNode; i++)
  {
    SerialPrintlnU8(i);
    printSlot(&tdm.node[i]);
  }
}
