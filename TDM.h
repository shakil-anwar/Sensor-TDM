#ifndef _TDM_H_
#define _TDM_H_
#include <Arduino.h>

#define MAX_SENSOR_NODE               100
#define MOMENT_DURATION_SEC           600UL
#define PER_NODE_INTERVAL_SEC         (MOMENT_DURATION_SEC/MAX_SENSOR_NODE)

struct node_t
{
  uint16_t deviceId;
  uint8_t slotNo;
  uint8_t isAllotted:1;
  uint8_t losSlot:4;
};

struct tdm_t
{
  uint8_t freeSlot;
  struct node_t node[MAX_SENSOR_NODE];
};

struct slot_t
{
  uint16_t momentDuration;
  uint16_t perNodeInterval;
  uint8_t slotNo;
};

void tdmBegin(uint32_t baseAddr);
void tdmUpdateSlot(uint32_t unixSec);
void tdmGetFreeSlot(uint16_t deviceId, struct slot_t *slot);
void tdmConfirmSlot(uint8_t slotNo);

struct node_t *tdmGetCurrentSlot();
struct node_t *tdmGetNewslot();

void tdmSaveNode(uint32_t addr, struct node_t *node);
void tdmReadNode(uint32_t addr, struct node_t *node);

#endif
