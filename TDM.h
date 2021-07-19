#ifndef _TDM_H_
#define _TDM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ARDUINO_ARCH_AVR)
    #include <Arduino.h>
    #if defined(PROD_BUILD)
        #include "../arduinoCwrapper/Serial.h"
    #else
        #include "Serial.h"
    #endif
#elif defined(ARDUINO_ARCH_SAM)
    #include <Arduino.h>
#elif defined(__MSP430G2553__) || defined(__MSP430FR2433__)
    #include "mspDriver.h"
#else
    #error "TDM did not find chip architecture "
#endif


#define HOUR_SEC      3600UL
#define DAY_TOTAL_SEC (24UL*HOUR_SEC)

#define MAX_SENSOR_NODE               100
#define MOMENT_DURATION_SEC           600UL
#define PER_NODE_INTERVAL_SEC         (MOMENT_DURATION_SEC/MAX_SENSOR_NODE)

typedef void (*tdmMemFun_t)(uint32_t addr,uint8_t *data,uint16_t len);
typedef void (*tdmMemErase_t)(uint32_t Addr);

struct node_t
{
  uint16_t deviceId;
  uint8_t slotNo;
  uint8_t isAllotted;
  uint8_t losSlot;
  uint8_t reserve;
  uint32_t regTime;
  uint32_t deployTime;
  uint16_t nodeChecksum;
};

struct tdmMeta_t
{
  uint8_t freeSlotId;
  uint8_t maxNode;  
  uint8_t perNodeInterval;
  uint8_t maxTrayNode;
  uint8_t reserveSlot;
  uint8_t reserve;
  uint16_t momentDuration;
  uint16_t metaChecksum;
};





void tdmInit(uint16_t durationMoment, uint8_t maxNode, uint8_t slotReserve, uint8_t trayMaxNode);
// void tdmAttachMem(uint8_t *buf,uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite);
void tdmAttachMem(uint8_t *buf, uint8_t *metaBuf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite, tdmMemErase_t nodeErase,
                            uint32_t metaBaseAddr, tdmMemFun_t metaRead, tdmMemFun_t metaWrite);


bool tdmBegin(uint8_t *buf, uint8_t *metaBuf, uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite, tdmMemErase_t nodeErase,
                        uint32_t metaBaseAddr, tdmMemFun_t metaRead, tdmMemFun_t metaWrite,
                            uint16_t momentDuration, uint8_t maxNode, uint8_t reserveSlot, uint8_t trayMaxNode);
void tdmReset();

void tdmUpdateSlot(uint32_t unixSec);
uint8_t tdmGetFreeSlot(uint16_t sensorId);
bool tdmConfirmSlot(uint8_t slotNo);

struct node_t *tdmGetCurrentNode();
struct tdmMeta_t *tdmGetMetaData();

void tdmDebug(bool debug);
void tdmPrintCurrentSlot();
uint8_t tdmIsRegistered(uint16_t sensorId);
uint8_t tdmIsRegistered2(uint16_t sensorId, uint8_t slotID);
void tdmPrintSlotDetails();
#ifdef __cplusplus
}
#endif


#endif
