#include "TDM.h"
/*
   For each node, allocate 4 bits for status.
   bit 0 :  alloted status
   bit 1 :  connected
   bit 2~3 : missCount
*/

void printAllSlot();
void printTdmMeta();

// #define HOUR_SEC      3600UL
// #define DAY_TOTAL_SEC (24UL*HOUR_SEC)


// uint32_t _todaySec;
// uint8_t  _currentMomentNo; 
// struct node_t *_currentNode;


// struct tdm_t tdm;
// struct tdm_t *tdm;

struct node_t *tdmNode;
struct tdmMeta_t *tdmMeta;
uint16_t _tdmLen;

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
  // allocate dynamic ram for tdm data
  _tdmLen = (maxNode)*sizeof(struct node_t) + sizeof(struct tdmMeta_t) + 1;
  SerialPrintF(P("TDM Ram size: ")); SerialPrintlnU16(_tdmLen);

  void *ptr = malloc(_tdmLen);
  tdmNode = (struct node_t*)ptr;
  tdmMeta = (struct tdmMeta_t*) (tdmNode + maxNode*sizeof(struct node_t));
  if(ptr != NULL)
  {
    SerialPrintlnF(P("TDM Memory Allocated"));
  }
  //  read slot from eeprom into ram;
  _nodeRead(_baseAddr, (uint8_t*)tdmNode, _tdmLen);
  printAllSlot();

  tdmMeta->maxNode = maxNode;
  tdmMeta->momentDuration = momentDuration;
  tdmMeta->reserveSlot = reserveSlot;
  tdmMeta->perNodeInterval = (momentDuration/maxNode);
}

void tdmReset()
{
  SerialPrintlnF(P("Resetting TDM"));
  memset(tdmNode, 0, sizeof(tdmNode[tdmMeta->maxNode]));
  // memset(tdmNode, 0, _tdmLen);
  tdmMeta -> freeSlotId = 0;
  _nodeWrite(_baseAddr, (uint8_t*)tdmNode, _tdmLen);
}

bool tdmSync(uint32_t unixSec)
{
  _MomentSec = unixSec % tdmMeta->momentDuration;
  return (_MomentSec % tdmMeta->perNodeInterval == 0);   //starting of new slot returns true
}

void tdmUpdateSlot(uint32_t unixSec)
{
  if (_tdmIsSync)
  {
    _MomentSec++;
    if (_MomentSec - _prevMomentSec >= tdmMeta->perNodeInterval)
    {
      _currentSlot++;
      if (_currentSlot > tdmMeta->maxNode - 1)
      {
        SerialPrintlnF(P("Max Node Exceeded----------------->"));
        //Start a new momenet and update time
        _MomentSec = 0;
        _currentSlot = 0;
        //        tdmSync(unixSec);
      }
      // _currentNode = &tdm.node[_currentSlot];
      
      printSlot(&tdmNode[_currentSlot],_currentSlot);
      
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
      
      //After sync Calculation
      _tdmIsSync = true;
      _prevMomentSec = _MomentSec;
      _currentSlot = _MomentSec / tdmMeta->perNodeInterval;

      SerialPrintF(P("_MomentSec : ")); SerialPrintlnU16(_MomentSec);
      printSlot(&tdmNode[_currentSlot],_currentSlot);
      
    }
  }

}

struct node_t *tdmGetCurrentNode()
{
  if(_tdmIsSync)
  {
    return &tdmNode[_currentSlot];
  }
  return NULL;
}

struct tdmMeta_t *tdmGetMetaData()
{
  return tdmMeta;
}

uint8_t tdmGetFreeSlot(uint16_t deviceId)
{
  uint8_t slotAvail = tdmMeta->freeSlotId;
  SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);
  if (slotAvail < tdmMeta->maxNode)
  {
    //fill up node info
    tdmNode[slotAvail].deviceId = deviceId;
    tdmNode[slotAvail].slotNo = slotAvail;
    printSlot(&tdmNode[slotAvail],slotAvail);

    return slotAvail;
  }
  return 255; //invalid slot
}

bool tdmConfirmSlot(uint8_t slotNo)
{
  if (slotNo == tdmMeta->freeSlotId)
  {
    SerialPrintF(P("Confirming Slot : ")); SerialPrintlnU8(slotNo);
    tdmNode[slotNo].isAllotted = 1; // slot allocation ok

    uint32_t memAddr = _baseAddr + slotNo * sizeof(struct node_t);
    SerialPrintF(P("node addr : ")); SerialPrintlnU8(memAddr);
    _nodeWrite(memAddr, (uint8_t*)&tdmNode[slotNo], sizeof(struct node_t));

    //read saved data
    struct node_t nodeBuf;
    _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    printSlot(&nodeBuf,slotNo);

    //update metadata
    tdmMeta->freeSlotId++;
    memAddr = _baseAddr + tdmMeta->maxNode*sizeof(struct node_t);
    _nodeWrite(memAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));

  }
  else
  {

    SerialPrintF(P("slotNo :")); SerialPrintU8(slotNo); SerialPrintlnF(P("Slot not confirmed"));
    SerialPrintF(P("tdm.meta.freeSlotId :")); SerialPrintlnU8(tdmMeta->freeSlotId);
  }
}



void printSlot(struct node_t *node, uint8_t slotNo)
{
  SerialPrintF(P("slot #:")); SerialPrintU8(slotNo);
  SerialPrintF(P(" |slotId:")); SerialPrintU8(node -> slotNo);
  SerialPrintF(P(" | deviceId:")); SerialPrintU8(node -> deviceId);
  SerialPrintF(P(" | isAllotted:")); SerialPrintU8(node -> isAllotted);
  SerialPrintF(P(" | losSlot:")); SerialPrintlnU8(node -> losSlot);
}

void printAllSlot()
{
  printTdmMeta(tdmMeta);
  uint8_t maxNode = tdmMeta->maxNode;
  uint8_t i;
  for ( i = 0; i < maxNode; i++)
  {
    SerialPrintlnU8(i);
    printSlot(&tdmNode[i],i);
  }
}

void printTdmMeta(struct tdmMeta_t *meta)
{
  SerialPrintF(P("maxNode:")); SerialPrintU8(meta -> maxNode);
  SerialPrintF(P(" | momentDuration:")); SerialPrintU16(meta -> momentDuration);
  SerialPrintF(P(" | perNodeInterval:")); SerialPrintU8(meta -> perNodeInterval);
  SerialPrintF(P(" | reserveSlot:")); SerialPrintU8(meta -> reserveSlot);
  SerialPrintF(P(" | freeSlotId:")); SerialPrintlnU8(meta -> freeSlotId);
}