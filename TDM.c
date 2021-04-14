#include "TDM.h"


void printTdmMeta();


struct node_t *tdmNode;
struct tdmMeta_t *tdmMeta;

bool _tdmIsSync = false;

uint16_t _MomentSec;
uint16_t _prevMomentSec;
uint8_t _currentSlot;

uint32_t _baseAddr;
tdmMemFun_t _nodeRead;
tdmMemFun_t _nodeWrite;


void tdmAttachMem(uint8_t *buf,uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite)
{
  _baseAddr = baseAddr;
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;
  tdmNode = (struct node_t*)buf;
}

void tdmInit(uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot)
{
  tdmMeta = (struct tdmMeta_t*) &tdmNode[maxNode];
  tdmMeta->maxNode = maxNode;
  tdmMeta->momentDuration = momentDuration;
  tdmMeta->reserveSlot = reserveSlot;
  tdmMeta->perNodeInterval = (momentDuration/maxNode);

  // SerialPrintlnU16((uint16_t)tdmMeta);
  // SerialPrintlnU16((uint16_t)&tdmNode[maxNode]);

   //Load Slot info from eeprom
  uint16_t _tdmLen = ((uint8_t*)&tdmNode[tdmMeta->maxNode] - (uint8_t*)tdmNode) + sizeof(struct tdmMeta_t) + 1;
  _nodeRead(_baseAddr, (uint8_t*)tdmNode, _tdmLen);
  SerialPrintF(P("TDM Buf Size: ")); SerialPrintlnU16(_tdmLen);
  tdmPrintSlotDetails();
}

void tdmBegin(uint8_t *buf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite,
              uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot)
{
  // buf = malloc(_tdmLen);
  tdmAttachMem(buf,baseAddr,nodeRead, nodeWrite);
  tdmInit(momentDuration,maxNode,reserveSlot);
}

void tdmReset()
{
  uint16_t nodeLen = (uint8_t*)&tdmNode[tdmMeta->maxNode] - (uint8_t*)tdmNode;
  SerialPrintF(P("Resetting TDM : "));
  SerialPrintlnS16(nodeLen);
  
  memset(tdmNode, 0,nodeLen);
  tdmMeta -> freeSlotId = 0;

  nodeLen += sizeof(struct tdmMeta_t)+1;
  _nodeWrite(_baseAddr, (uint8_t*)tdmNode, nodeLen);
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
      
      tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
      
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
      tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
      
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

uint8_t tdmGetFreeSlot(uint16_t sensorId)
{
  uint8_t slotAvail = tdmMeta->freeSlotId;
  SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);
  if (slotAvail < (tdmMeta->maxNode - tdmMeta->reserveSlot))
  {
    //fill up node info
    tdmNode[slotAvail].deviceId = sensorId;
    tdmNode[slotAvail].slotNo = slotAvail;
    tdmPrintSlot(&tdmNode[slotAvail],slotAvail);

    return slotAvail;
  }
  else
  {
    SerialPrintF(P("No slot"));
  }
  return 255; //invalid slot
}

bool tdmConfirmSlot(uint8_t slotNo)
{
  if (slotNo == tdmMeta->freeSlotId)
  {
    SerialPrintF(P("Confirming Slot : ")); SerialPrintlnU8(slotNo);
    tdmNode[slotNo].isAllotted = 1; // slot allocation ok

    // uint32_t memAddr = _baseAddr + slotNo * sizeof(struct node_t);
    uint32_t memAddr = _baseAddr + (uint32_t)((uint8_t*)&tdmNode[slotNo] - (uint8_t*)tdmNode);
    _nodeWrite(memAddr, (uint8_t*)&tdmNode[slotNo], sizeof(struct node_t));
    //  SerialPrintF(P("node addr : ")); SerialPrintlnU16(memAddr);

    //read saved data
    struct node_t nodeBuf;
    _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    tdmPrintSlot(&nodeBuf,slotNo);

    //update metadata
    tdmMeta->freeSlotId++;
    // memAddr = _baseAddr + tdmMeta->maxNode*sizeof(struct node_t);
    memAddr = _baseAddr + (uint32_t)((uint8_t*)&tdmNode[tdmMeta->maxNode] - (uint8_t*)tdmNode);
    _nodeWrite(memAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));

  }
  else
  {

    SerialPrintF(P("slotNo :")); SerialPrintU8(slotNo); SerialPrintlnF(P("Slot not confirmed"));
    SerialPrintF(P("tdm.meta.freeSlotId :")); SerialPrintlnU8(tdmMeta->freeSlotId);
  }
}



void tdmPrintSlot(struct node_t *node, uint8_t slotNo)
{
  SerialPrintF(P("slot #:")); SerialPrintU8(slotNo);
  SerialPrintF(P(" |slotId:")); SerialPrintU8(node -> slotNo);
  SerialPrintF(P(" | deviceId:")); SerialPrintU8(node -> deviceId);
  SerialPrintF(P(" | isAllotted:")); SerialPrintU8(node -> isAllotted);
  SerialPrintF(P(" | losSlot:")); SerialPrintlnU8(node -> losSlot);
}

void tdmPrintSlotDetails()
{
  printTdmMeta(tdmMeta);
  uint8_t maxNode = tdmMeta->maxNode;
  uint8_t i;
  for ( i = 0; i < maxNode; i++)
  {
    SerialPrintlnU8(i);
    tdmPrintSlot(&tdmNode[i],i);
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