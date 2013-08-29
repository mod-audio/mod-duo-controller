
#ifndef __CDCUSER_H__
#define __CDCUSER_H__

#include <stdint.h>

#define CDC_RX_BUFFER_SIZE      1024
#define CDC_TX_BUFFER_SIZE      256

/* CDC Data In/Out Endpoint Address */
#define CDC_DEP_IN       0x82
#define CDC_DEP_OUT      0x02

/* CDC Communication In Endpoint Address */
#define CDC_CEP_IN       0x81

/* CDC User Functions */
void CDC_Init(void);
void CDC_SetMessageCallback(void (*callback)(uint32_t msg_size));
uint32_t CDC_GetMessage(uint8_t *msg_buffer, uint32_t msg_size);
void CDC_Send(const uint8_t *data, uint32_t data_size);

/* CDC Requests Callback Functions */
extern uint32_t CDC_SendEncapsulatedCommand(void);
extern uint32_t CDC_GetEncapsulatedResponse(void);
extern uint32_t CDC_SetCommFeature(unsigned short wFeatureSelector);
extern uint32_t CDC_GetCommFeature(unsigned short wFeatureSelector);
extern uint32_t CDC_ClearCommFeature(unsigned short wFeatureSelector);
extern uint32_t CDC_GetLineCoding(void);
extern uint32_t CDC_SetLineCoding(void);
extern uint32_t CDC_SetControlLineState(unsigned short wControlSignalBitmap);
extern uint32_t CDC_SendBreak(unsigned short wDurationOfBreak);

/* CDC Bulk Callback Functions */
extern void CDC_BulkIn(void);
extern void CDC_BulkOut(void);

#endif  /* __CDCUSER_H__ */

