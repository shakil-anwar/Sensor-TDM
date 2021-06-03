#include "TDM.h"
/*
Author : Shuvangkar Chandra Das 
Contributor :  Shakil Anwar
Description :  This library maintains all the functionlity of sensor scheduling.
*/
struct freeslotLog_t
{
	bool isRegtered;
	bool isAvail;
	uint8_t slotId;
	uint8_t reserve;
};

void tdmPrintSlotReg(struct freeslotLog_t *fslotLog);

void printTdmMeta();

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


//enable of disable tdm internal debug
void tdmDebug(bool debug)
{
  _debug = debug;
}

//this function points the ram and perment memory address and read write function of eeprom 
void tdmAttachMem(uint8_t *buf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite)
{
  _romBaseAddr = baseAddr; //perment memory base address pointer 
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;
  tdmNode = (struct node_t*)buf;
  // SerialPrintF(P("Test0: "));
  // SerialPrintlnU16((uint16_t)tdmNode);
  // SerialPrintlnU16((uint16_t)&tdmNode[0]);
}

//initialize tdm from user defined memory 
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

//tdm final begin function for user firmware api. 
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

  if(_debug){SerialPrintF(P("TDM->BEGIN->OK:"));SerialPrintlnU8(tdmOk);}
}

//this erases all the memory related to tdm
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


//This function is very cruital function for tdm. It sync tdm sheduling with the real time. 
// if the nearest slot sync with the tdm returns true else returns false 
bool tdmSync(uint32_t unixSec)
{
  _MomentSec = (uint16_t)(unixSec % (uint32_t)tdmMeta->momentDuration);
  uint16_t syncRem = _MomentSec % (uint16_t)(tdmMeta->perNodeInterval);
  return (syncRem == 0);   //starting of new slot returns true
}

//This function update tdm slot in timer interrupt 
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
      }
      
      // tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
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

      // if(_debug)
      // {
      //   SerialPrintF(P("_MomentSec : ")); SerialPrintlnU16(_MomentSec);
      //   tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
      // }      
    }

    if(_debug){SerialPrintF(P("TDM->SYNC:")); SerialPrintlnU8((uint8_t)sync);}

  }

}

//print tdm current slot info in main loop 
void tdmPrintCurrentSlot()
{
  static uint8_t lastslot;
  if(_currentSlot != lastslot)
  {
    tdmPrintSlot(&tdmNode[_currentSlot],_currentSlot);
    lastslot = _currentSlot;
  }

}

//return current node information if tdm is synced else return null 
struct node_t *tdmGetCurrentNode()
{
  if(_tdmIsSync)
  {
    return &tdmNode[_currentSlot];
  }
  return NULL;
}

//return pointer of tdm meta data 
struct tdmMeta_t *tdmGetMetaData()
{
  return tdmMeta;
}

//check whether a sensor node is registered before. if registered return slot id. 
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

uint8_t tdmIsRegistered2(uint16_t sensorId, uint8_t slotID)
{

  if(tdmNode[slotID].deviceId == sensorId)
  {
    return slotID;
  }else
  {
    return 255;
  }
}


void tdmPrintSlotReg(struct freeslotLog_t *fslotLog)
{
	SerialPrintF(P("TDM->GETSLOT->isAvail:"));SerialPrintU8(fslotLog->isAvail);
	SerialPrintF(P("|isOld:"));SerialPrintU8(fslotLog->isRegtered);
	SerialPrintF(P("|slotId:"));SerialPrintU8(fslotLog->slotId);
	SerialPrintF(P("|devId:"));SerialPrintlnU16(tdmNode[fslotLog->slotId].deviceId);
}

//this function return free slot id for new registratino 
uint8_t tdmGetFreeSlot(uint16_t sensorId)
{
  uint8_t slotAvail = tdmIsRegistered(sensorId);
  struct freeslotLog_t slotLog;
  if(slotAvail !=255)
  {
  	slotLog.isRegtered = true;
  	slotLog.isAvail = false;
    // if(_debug){SerialPrintF(P("Sensor Already Registered:")); SerialPrintlnU8(slotAvail);}
    // return slotAvail;
  }
  else
  {
  	slotAvail = tdmMeta->freeSlotId;
	// if(_debug){SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);}
  	if (slotAvail < (tdmMeta->maxNode - tdmMeta->reserveSlot))
  	{
  	    //fill up node info for new sensor
  	    tdmNode[slotAvail].deviceId = sensorId;
  	    tdmNode[slotAvail].slotNo = slotAvail;
  	    // tdmPrintSlot(&tdmNode[slotAvail],slotAvail);

  	    slotLog.isRegtered = false;
    		slotLog.isAvail = true;
  	    // return slotAvail;
  	}
  	else
  	{
  		  slotAvail = 255; //invalid slot

  		  slotLog.isRegtered = false;
    		slotLog.isAvail = true;
  	    // if(_debug){SerialPrintF(P("Slot Not Available"));}
  	}
  }

  slotLog.slotId = slotAvail;
  if(_debug){tdmPrintSlotReg(&slotLog);}
  return slotAvail;
  // return 255; //invalid slot
}

//this function confirms registration of current slot 
bool tdmConfirmSlot(uint8_t slotNo)
{
  bool isSlotConfirmed = false;
  if (slotNo == tdmMeta->freeSlotId)
  {
    
    tdmNode[slotNo].isAllotted = 1; // slot allocation ok
    uint32_t currentAddr = _romBaseAddr + (uint32_t)((uint8_t*)&tdmNode[slotNo] - (uint8_t*)tdmNode);
    _nodeWrite(currentAddr, (uint8_t*)&tdmNode[slotNo], sizeof(struct node_t));
    //update metadata
    tdmMeta->freeSlotId++;
    currentAddr = _romBaseAddr + (uint32_t)((uint8_t*)tdmMeta - (uint8_t*)tdmNode);
    _nodeWrite(currentAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));
    isSlotConfirmed = true;
    // if(_debug){SerialPrintF(P("Confirmed Slot : ")); SerialPrintlnU8(slotNo);}
  }
  // else
  // {

  // }
  // else
  // {
  //   if(_debug){SerialPrintF(P("Slot Registered or Failed"));}
  // }

  if(_debug)
  {
  	SerialPrintF(P("TDM->CNFRM_SLOT->isRegDone:")); SerialPrintlnU8(isSlotConfirmed);
  }

  return isSlotConfirmed;
}


//print slot information 
void tdmPrintSlot(struct node_t *node, uint8_t slotNo)
{
  SerialPrintF(P("TDM->")); SerialPrintU8(slotNo);
  SerialPrintF(P("|slotId:")); SerialPrintU8(node -> slotNo);
  SerialPrintF(P("|devId:")); SerialPrintU16(node -> deviceId);
  SerialPrintF(P("|isAllot:")); SerialPrintU8(node -> isAllotted);
  SerialPrintF(P("|losSlot:")); SerialPrintlnU8(node -> losSlot);
}

//print all details of slot 
void tdmPrintSlotDetails()
{
  printTdmMeta(tdmMeta);
  uint8_t maxNode = tdmMeta->maxNode;
  uint8_t i;
  for ( i = 0; i < maxNode; i++)
  {
    // SerialPrintlnU8(i);
    tdmPrintSlot(&tdmNode[i],i);
  }
}

//print metadata information 
void printTdmMeta(struct tdmMeta_t *meta)
{
  SerialPrintF(P("TDM->META->Node:")); SerialPrintU8(meta -> maxNode);
  SerialPrintF(P("|Dur:")); SerialPrintU16(meta -> momentDuration);
  SerialPrintF(P("|Int:")); SerialPrintU8(meta -> perNodeInterval);
  SerialPrintF(P("|rsrvSlt:")); SerialPrintU8(meta -> reserveSlot);
  SerialPrintF(P("|freeSlot:")); SerialPrintlnU8(meta -> freeSlotId);
}


    // SerialPrintF(P("node addr from lib : ")); SerialPrintlnU32(_currentAddr);
  // //read saved data
    // struct node_t nodeBuf;
    // _nodeRead(memAddr, (uint8_t*)&nodeBuf, sizeof(struct node_t));
    // tdmPrintSlot(&nodeBuf,slotNo);