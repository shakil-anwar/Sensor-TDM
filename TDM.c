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
volatile struct node_t *tdmTray;
volatile struct tdmMeta_t *tdmMeta;

volatile struct node_t tdmTempNodeData;
volatile struct node_t *tdmRegTempNode = &tdmTempNodeData;


volatile bool _tdmIsSync;

volatile uint16_t _MomentSec;
volatile uint16_t _prevMomentSec;
volatile  uint8_t _currentSlot;
volatile  uint8_t _currentTrayNo;
volatile  uint8_t _currentTraySlotNo;

volatile uint32_t _romBaseAddr;
tdmMemFun_t _nodeRead;
tdmMemFun_t _nodeWrite;
tdmMemErase_t _nodeErase;

volatile uint32_t _metaBaseAddr;
tdmMemFun_t _metaRead;
tdmMemFun_t _metaWrite;


//enable of disable tdm internal debug
void tdmDebug(bool debug)
{
  _debug = debug;
}

//this function points the ram and perment memory address and read write function of eeprom 
void tdmAttachMem(uint8_t *buf, uint8_t *metaBuf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite, tdmMemErase_t nodeErase,
                            uint32_t metaBaseAddr, tdmMemFun_t metaRead, tdmMemFun_t metaWrite)
{
  _romBaseAddr = baseAddr; //perment memory base address pointer 
  _nodeRead = nodeRead;
  _nodeWrite = nodeWrite;
  tdmTray = (struct node_t*)buf;

  _metaBaseAddr = metaBaseAddr; 
  _metaRead = metaRead;
  _metaWrite = metaWrite;
  tdmMeta = (struct tdmMeta_t*)metaBuf;

  _nodeErase = nodeErase;
}
//initialize tdm from user defined memory 
void tdmInit(uint16_t durationMoment, uint8_t maxNode, uint8_t slotReserve, uint8_t trayMaxNode)
{
    _metaRead(_metaBaseAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));

  if(_debug){tdmPrintSlotDetails();}

  tdmMeta->maxNode = maxNode;
  tdmMeta->momentDuration = durationMoment;
  tdmMeta->reserveSlot = slotReserve;
  tdmMeta->perNodeInterval = (durationMoment/maxNode);
  tdmMeta->maxTrayNode = trayMaxNode;
  _metaWrite(_metaBaseAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));

  _tdmIsSync = false;//device not synced initially
  
  uint16_t _tdmLen = (uint16_t) (tdmMeta->maxTrayNode*sizeof(struct node_t));
  _nodeRead(_romBaseAddr, (uint8_t*)tdmTray, _tdmLen);
}

//tdm final begin function for user firmware api. 
void tdmBegin(uint8_t *buf,uint8_t *metaBuf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite, tdmMemErase_t nodeErase,
                        uint32_t metaBaseAddr, tdmMemFun_t metaRead, tdmMemFun_t metaWrite,
                            uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot, uint8_t trayMaxNode)
{
  tdmAttachMem(buf,metaBuf,baseAddr,nodeRead, nodeWrite,nodeErase, metaBaseAddr,metaRead, metaWrite);
  tdmInit(momentDuration,maxNode,reserveSlot,trayMaxNode);
  // SerialPrintF(P("rom base addr : ")); SerialPrintlnU32(_romBaseAddr);
  //validate basic value for operation
  bool tdmOk = (tdmMeta->maxNode>0 ) && (tdmMeta->momentDuration>0 ) && 
               (tdmMeta->perNodeInterval > 0);

  if(_debug){SerialPrintF(P("TDM->BEGIN->OK:"));SerialPrintlnU8(tdmOk);}
}

//this erases all the memory related to tdm
void tdmReset()
{
  SerialPrintF(P("Resetting TDM : "));
  tdmMeta -> freeSlotId = 0;
  _metaWrite(_metaBaseAddr,(uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));
  _nodeErase(_romBaseAddr);
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
// printTdmMeta(tdmMeta);
  if (_tdmIsSync)
  {
    _MomentSec++;
    if (_MomentSec - _prevMomentSec >= tdmMeta->perNodeInterval)
    {
      _currentSlot++;
      _currentTraySlotNo =_currentSlot % tdmMeta->maxTrayNode;
      
      if (_currentSlot > tdmMeta->maxNode - 1)
      {
        // SerialPrintlnF(P("Max Node Exceeded----------------->"));
        //Start a new momenet and update time
        _MomentSec = 0;
        _currentSlot = 0;
        _currentTrayNo = 0;
        _currentTraySlotNo = 0;
      }

      if(_currentTraySlotNo == 0)
      {        
        if(_currentSlot != 0)
        {  
          _currentTrayNo++;
        }
        tdmTrayUpdate(_currentTrayNo);
        // tdmTrayPrint(_currentTrayNo);
      }
      SerialPrintF(P("Slot: ")); SerialPrintU8(_currentSlot);
      SerialPrintF(P(" |TrayNo: ")); SerialPrintU8(_currentTrayNo);
      SerialPrintF(P(" |TraySlot: ")); SerialPrintlnU8(_currentTraySlotNo);
      maxCheckPerNode(&tdmTray[_currentTraySlotNo],_currentSlot);
      tdmPrintSlot(&tdmTray[_currentTraySlotNo],_currentSlot);       
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
      _currentTrayNo = _currentSlot / tdmMeta->maxTrayNode;
      _currentTraySlotNo = _currentSlot % tdmMeta->maxTrayNode;
      tdmTrayUpdate(_currentTrayNo);
      if(_debug)
      {
        SerialPrintF(P("_MomentSec : ")); SerialPrintlnU16(_MomentSec);
      }      
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
    tdmPrintSlot(&tdmTray[_currentTraySlotNo],_currentSlot);
    lastslot = _currentSlot;
  }

}

//return current node information if tdm is synced else return null 
struct node_t *tdmGetCurrentNode()
{
  if(_tdmIsSync)
  {
    return &tdmTray[_currentTraySlotNo];
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
 uint8_t slotno;
  uint8_t maxnode = tdmMeta->maxNode;

  for(slotno = 0; slotno< maxnode; slotno++)
  {
    tdmRegNodeRead(slotno);
    if(tdmRegTempNode->deviceId == sensorId)
    {
      return tdmRegTempNode->slotNo;
    }
  }
  return 255; //invalid 
}

uint8_t tdmIsRegistered2(uint16_t sensorId, uint8_t slotID)
{

 tdmRegNodeRead( sensorId, slotID );
  if(tdmRegTempNode->deviceId == sensorId)
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
  SerialPrintF(P("|devId:"));SerialPrintlnU16(tdmRegTempNode->deviceId);
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
    if(_debug){SerialPrintF(P("Sensor Already Registered:")); SerialPrintlnU8(slotAvail);}
    // return slotAvail;
  }
  else
  {
  	slotAvail = tdmMeta->freeSlotId;
	// if(_debug){SerialPrintF(P("slot Avail :")); SerialPrintlnU8(slotAvail);}
  	if (slotAvail < (tdmMeta->maxNode - tdmMeta->reserveSlot))
  	{
  	    //fill up node info for new sensor
  	    tdmRegTempNode->deviceId = sensorId;
        tdmRegTempNode->slotNo = slotAvail;
        tdmRegTempNode->losSlot = 0; // did it just for print
        tdmPrintSlot(tdmRegTempNode,slotAvail);

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
    tdmRegTempNode->isAllotted = 1; // slot allocation ok
    tdmRegNodeWrite(slotNo);
    //update metadata
    tdmMeta->freeSlotId++;
    _metaWrite(_metaBaseAddr, (uint8_t*)tdmMeta, sizeof(struct tdmMeta_t));
    isSlotConfirmed = true;
    if(_debug){SerialPrintF(P("Confirmed Slot : ")); SerialPrintlnU8(slotNo);}
  }

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
  uint8_t maxTrayNo = tdmMeta->maxNode / tdmMeta->maxTrayNode;
  uint8_t i;
  for ( i = 0; i < maxTrayNo; i++)
  {
    tdmTrayUpdate(i);
    tdmTrayPrint(i);
  }
}

//print metadata information 
void printTdmMeta(struct tdmMeta_t *meta)
{
  SerialPrintF(P("TDM->META->Node:")); SerialPrintU8(meta -> maxNode);
  SerialPrintF(P("|Dur:")); SerialPrintU16(meta -> momentDuration);
  SerialPrintF(P("|Int:")); SerialPrintU8(meta -> perNodeInterval);
  SerialPrintF(P("|MaxTryN:")); SerialPrintU8(meta -> maxTrayNode);
  SerialPrintF(P("|rsrvSlt:")); SerialPrintU8(meta -> reserveSlot);
  SerialPrintF(P("|freeSlot:")); SerialPrintlnU8(meta -> freeSlotId);
}

void maxCheckPerNode(struct node_t *node, uint8_t slotNo)
{
  if((node -> slotNo == 0xFF) && (node -> deviceId == 0xFFFF))
  {
    node -> slotNo=0;
    node -> deviceId=0;
    node -> isAllotted=0;
    node -> losSlot=0;
  }  
}

void tdmTrayUpdate(uint8_t trayno)
{
  uint32_t currentAddr = _romBaseAddr + (uint32_t) (trayno * tdmMeta->maxTrayNode  * sizeof(struct node_t));
  SerialPrintF(P("TrayNo: ")); SerialPrintU8( trayno); SerialPrintlnF(P(" is updated"));
  // SerialPrintF(P("baseAddr: ")); SerialPrintlnU32( _romBaseAddr);
  // SerialPrintF(P("CurrAddr: ")); SerialPrintlnU32(currentAddr);
  _nodeRead(currentAddr, (uint8_t*)tdmTray,tdmMeta->maxTrayNode*sizeof(struct node_t));
}

void tdmTrayPrint(uint8_t trayno)
{
  uint8_t maxTrayNode = tdmMeta->maxTrayNode;
  uint8_t i;
  // SerialPrintlnF(P("trayData: "));
  for ( i = 0; i < maxTrayNode; i++)
  {
    uint8_t slotno = trayno * maxTrayNode +i;
    maxCheckPerNode(&tdmTray[i], slotno);   
    tdmPrintSlot(&tdmTray[i], slotno);
  }
  // SerialPrintlnF(P("---------"));
}

void tdmRegNodeRead(uint8_t slotno)
{
  uint32_t currentAddr = _romBaseAddr + (uint32_t) (slotno * sizeof(struct node_t));
  // SerialPrintF(P("baseAddr: ")); SerialPrintlnU32( _romBaseAddr);
  // SerialPrintF(P("CurrAddr: ")); SerialPrintlnU32(currentAddr);
  _nodeRead(currentAddr, (uint8_t*)tdmRegTempNode, sizeof(struct node_t));
}

void tdmRegNodeWrite(uint8_t slotno)
{
  uint32_t currentAddr = _romBaseAddr + (uint32_t) (slotno * sizeof(struct node_t)) ;
  // SerialPrintF(P("slotNo : ")); SerialPrintlnU32(slotno);
  // SerialPrintF(P("currentAddr : ")); SerialPrintlnU32(currentAddr);
  _nodeWrite(currentAddr, (uint8_t*)tdmRegTempNode, sizeof(struct node_t));
}