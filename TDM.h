#ifndef _TDM_H_
#define _TDM_H_

#if defined(ARDUINO_ARCH_AVR)
    #include <Arduino.h>
    #if defined(PROD_BUILD)
        #include "../arduinoCwrapper/Serial.h"
        #include "../arduinoCwrapper/spi_driver.h"
    #else
        #include "spi_driver.h"
        #include "Serial.h"
    #endif
#elif defined(ARDUINO_ARCH_SAM)
    #include <Arduino.h>
#else
    #error "TDM did not find chip architecture "
#endif

#define MAX_SENSOR_NODE               100
#define MOMENT_DURATION_SEC           600UL
#define PER_NODE_INTERVAL_SEC         (MOMENT_DURATION_SEC/MAX_SENSOR_NODE)

#ifdef __cplusplus
extern "C" {
#endif

struct node_t
{
  uint16_t deviceId;
  uint8_t slotNo;
  uint8_t isAllotted:1;
  uint8_t losSlot:4;
  uint8_t reserve:3;
};

struct tdmMeta_t
{
  uint8_t maxNode;
  uint8_t freeSlotId;
  uint8_t deadSlot[6];
  uint8_t reserve;
  uint16_t momentDuration;
};

struct tdm_t
{
  struct node_t node[MAX_SENSOR_NODE];
  struct tdmMeta_t meta;
  uint8_t checksum;
};

struct slot_t
{
  uint16_t momentDuration;
  uint16_t perNodeInterval;
  uint8_t slotNo;
};
typedef void (*tdmMemFun_t)(uint32_t,uint8_t*,uint16_t len);



void tdmBegin(uint32_t baseAddr, tdmMemFun_t nodeRead, tdmMemFun_t nodeWrite,
              uint16_t momentDuration, uint8_t maxNode);
void tdmUpdateSlot(uint32_t unixSec);
void tdmGetFreeSlot(uint16_t deviceId, struct slot_t *slot);
void tdmConfirmSlot(uint8_t slotNo);
void tdmReset();
void *tdmGetCurrentNode();


void printSlot(struct node_t *node);

#ifdef __cplusplus
}
#endif


#endif
