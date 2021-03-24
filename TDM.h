#ifndef _TDM_H_
#define _TDM_H_
#include <Arduino.h>

#define MAX_NODE               100
#define MOMENT_DURATION_SEC    600UL
#define PER_NODE_INTERVAL_SEC  3

struct node_t
{
  uint16_t deviceId;
  uint8_t slotNo;
  uint8_t isAllotted:1;
  uint8_t losSlot:4;
};

struct nodeBuf_t
{
  struct node_t node[MAX_NODE];
};

////typedef struct nodeBuf_t
////{
////  uint16_t addr[MAX_NODE];
////  uint8_t  status[MAX_NODE/2 +1];
////}
//
//typedef struct node_t
//{
//  uint8_t slot;
//  uint16_t addr;
//}
//void nodeslotBegin();

void tdmBegin(uint32_t unixsec);
void tdmUpdateSlot(uint32_t unixSec);
struct node_t *tdmGetCurrentSlot();
struct node_t *tdmGetNewslot();

#endif
