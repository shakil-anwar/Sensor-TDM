#include "TDM.h"


void printTdmMeta();
uint8_t tdmIsRegistered(uint16_t sensorId);
void tdmPrintSlot(struct node_t *node, uint8_t slotNo);
void tdmPrintSlotDetails();

static bool _debug = true;
volatile struct node_t *tdmNode;
volatile struct tdmMeta_t *tdmMeta;

volatile bool _tdmIsSync;

volatile uint16_t _MomentSec;
volatile uint16_t _prevMomentSec;
volatile  uint8_t _currentSlot;

volatile uint32_t _romBaseAddr;
tdmMemFun_t _nodeRead;
tdmMemFun_t _nodeWrite;


void tdmDebug(bool debug)
{
  _debug = debug;
}

void tdmAttachMem(uint8_t *buf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite)
{
  _romBaseAddr = baseAddr;
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;
  tdmNode = (struct node_t*)buf;
  // SerialPrintF(P("Test0: "));
  // SerialPrintlnU16((uint16_t)tdmNode);
  // SerialPrintlnU16((uint16_t)&tdmNode[0]);
}

void tdmInit(uint16_t durationMoment, uint8_t nodeMax, uint8_t slotReserve)
{

  tdmMeta = (struct tdmMeta_t*) &tdmNode[nodeMax];

  // SerialPrintF(P("Max Node : ")); SerialPrintlnU8(tdmMeta->maxNode);
  // SerialPrintF(P("momentDuration: ")); SerialPrintlnU8(tdmMeta->momentDuration);

  // SerialPrintF(P("tdmNode[tdmMeta->maxNode]"));
  // SerialPrintlnU16((uint16_t)tdmMeta);

  uint16_t _tdmLen = ((uint8_t*)tdmMeta - (uint8_t*)tdmNode) + sizeof(struct tdmMeta_t) + 1;
  _nodeRead(_romBaseAddr, (uint8_t*)tdmNode, _tdmLen);

  // SerialPrintF(P("TDM Buf Size: ")); SerialPrintlnU16(_tdmLen);

  if(_debug){tdmPrintSlotDetails();}

  tdmMeta->maxNode = nodeMax;
  tdmMeta->momentDuration = durationMoment;
  tdmMeta->reserveSlot = slotReserve;
  tdmMeta->perNodeInterval = (durationMoment/nodeMax);

  _tdmIsSync = false;//device not synced initially
  
}

void tdmBegin(uint8_t *buf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite,
              uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot)
{
  // buf = malloc(_tdmLen);
  tdmAttachMem(buf,baseAddr,nodeRead, nodeWrite);
  tdmInit(momentDuration,maxNode,reserveSlot);
  // SerialPrintF(P("rom base addr : ")); SerialPrintlnU32(_romBaseAddr);
  //validate basic value for operation
  bool tdmOk = (tdmMeta->maxNode>0 ) && (tdmMeta->momentDuration>0 ) && 
               (tdmMeta->perNodeInterval > 0);

  if(_debug){SerialPrintF(P("TDM OK: "));SerialPrintlnU8(tdmOk);}
}

void tdmReset()
{
  // SerialPrintF(P("Resetting TDM : "));
  // SerialPrintF(P("tdmNode[tdmMeta->maxNode]"));
  // SerialPrintlnU16((uint16_t)tdmMeta);
  // SerialPrintlnU16((uint16_t)&tdmNode[tdmMeta->maxNode]);

  int16_t nodeLen = (int16_t)((uint8_t*)tdmMeta - (uint8_t*)tdmNode);

  memset(tdmNode, 0,nodeLen);
  tdmMeta -> freeSlotId = 0;

  nodeLen += sizeof(struct tdmMeta_t)+1;
  _nodeWrite(_romBaseAddr, (uint8_t*)tdmNode, nodeLen);
}

bool tdmSync(uint32_t unixSec)
{
  _MomentSec = (uint16_t)(unixSec % (uint32_t)tdmMeta->momentDuration);
  uint16_t syncRem = _MomentSec % (uint16_t)(tdmMeta->perNodeInterval);
  return (syncRem == 0);   //starting of new slot returns true
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
        // SerialPrintlnF(P("Max Node Exceeded----------------->"));
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
    if (sync)
    {
      
      //After sync Calculation
      _tdmIsSync = true;
      _prevMomentSec = _MomentSec;
      _currentSlot = _MomentSec / tdmMeta->perNodeInterval;

      if(_debug)
      {
        SerialPrintF(P("_MomentSec : ")); SerialPrintlnU16(_MomentSec);
        tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
      }      
    }

    if(_debug){SerialPrintF(P("TDM Sync :")); SerialPrintlnU8((uint8_t)sync);}

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

uint8_t tdmIsRegistered(uint16_t sensorId)
{
  uint8_t i;
  uint8_t maxnode = tdmMeta->maxNode;
  for(i = 0; i< maxnode; i++)
  {
    if(tdmNode[i].deviceId == sensorId)
    {
      return tdmNode[i].slotNo;
    }
  }
  return 255; //invalid 
}

uint8_t tdmGetFreeSlot(uint16_t sensorId)
{
  uint8_t slotAvail = tdmIsRegistered(sensorId);
  if(slotAvail !=255)
  {
    if(_debug){SerialPrintF(P("Sensor Already Registered:")); SerialPrintlnU8(slotAvail);}
    return slotAvail;
  }

  slotAvail = tdmMeta->freeSlotId;
  if(_debug){SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);}

  if (slotAvail < (tdmMeta->maxNode - tdmMeta->reserveSlot))
  {
    //fill up node info for new sensor
    tdmNode[slotAvail].deviceId = sensorId;
    tdmNode[slotAvail].slotNo = slotAvail;
    tdmPrintSlot(&tdmNode[slotAvail],slotAvail);
    return slotAvail;
  }
  else
  {
    if(_debug){SerialPrintF(P("Slot Not Available"));}
  }
  return 255; //invalid slot
}

bool tdmConfirmSlot(uint8_t slotNo)
{
  if (slotNo == tdmMeta->freeSlotId)
  {
    
    tdmNode[slotNo].isAllotted = 1; // slot allocation ok
    uint32_t currentAddr = _romBaseAddr + (uint32_t)((uint8_t*)&tdmNode[slotNo] - (uint8_t*)tdmNode);
    _nodeWrite(currentAddr, (uint8_t*)&tdmNode[slotNo], sizeof(struct node_t));
    //update metadata
    tdmMeta->freeSlotId++;
    currentAddr = _romBaseAddr + (uint32_t)((uint8_t*)tdmMeta - (uint8_t*)tdmNode);
    _nodeWrite(currentAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));
    if(_debug){SerialPrintF(P("Confirmed Slot : ")); SerialPrintlnU8(slotNo);}
  }
  else
  {
    if(_debug){SerialPrintF(P("Slot Registered or Failed"));}
  }
}



void tdmPrintSlot(struct node_t *node, uint8_t slotNo)
{
  SerialPrintF(P("slot #:")); SerialPrintU8(slotNo);
  SerialPrintF(P(" |slotId:")); SerialPrintU8(node -> slotNo);
  SerialPrintF(P(" | deviceId:")); SerialPrintU16(node -> deviceId);
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


    // SerialPrintF(P("node addr from lib : ")); SerialPrintlnU32(_currentAddr);
  // //read saved data
    // struct node_t nodeBuf;
    // _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    // tdmPrintSlot(&nodeBuf,slotNo);