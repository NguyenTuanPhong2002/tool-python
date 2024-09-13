/*
 * File:    customprotocol.h
 * Author:  Nguyen Tuan Phong
 * Date:    2024-09-05
 */
#ifndef CUSTOMPROTOCOL_H_
#define CUSTOMPROTOCOL_H_

#include <stdint.h>

#define CUSTOM_PROTOCOL_COMMAND_READ 0x01
#define CUSTOM_PROTOCOL_COMMAND_WRITE 0x00
#define CUSTOM_PROTOCOL_SIZE 500U

#define CUSTOM_PROTOCOL_RING_BUFFER_SIZE 500U

#define CUSTOM_PROTOCOL_START_BIT 0xFF
#define CUSTOM_PROTOCOL_END_BIT 0x00
#define NULL ((void *)0)

#define CUSTOM_PROTOCOL_TIMEOUT_ENABLE (1)

enum
{
	CUSTOM_PROTOCOL_STEP_START_BIT,
	CUSTOM_PROTOCOL_STEP_COMMAND,
	CUSTOM_PROTOCOL_STEP_LENGTH0,
	CUSTOM_PROTOCOL_STEP_LENGTH1,
	CUSTOM_PROTOCOL_STEP_DATA,
	CUSTOM_PROTOCOL_STEP_CRC1,
	CUSTOM_PROTOCOL_STEP_CRC2,
	CUSTOM_PROTOCOL_STEP_END_BIT,

};

typedef struct
{
	uint8_t step;
	uint8_t start_bit;
	uint8_t command;
	uint16_t length;
	uint8_t data[CUSTOM_PROTOCOL_SIZE];
	uint16_t checksum;
	uint8_t end_bit;
} CustomProtocolMessage;

typedef enum
{
	CUSTOM_PROTOCOL_STATE_RESET,
	CUSTOM_PROTOCOL_STATE_OK,
	CUSTOM_PROTOCOL_STATE_BUSY,
	CUSTOM_PROTOCOL_STATE_TIMEOUT,
	CUSTOM_PROTOCOL_STATE_ERROR
} Custom_Protocol_StateTypeDef;

typedef struct
{
	uint8_t data[CUSTOM_PROTOCOL_RING_BUFFER_SIZE];
	uint16_t read_index;
	uint16_t write_index;
	uint16_t size;
} CustomProtocolRingBuffer;

typedef struct
{
	Custom_Protocol_StateTypeDef (*Init)(void);
	Custom_Protocol_StateTypeDef (*Transmit)(uint8_t *pData, uint16_t size);
	Custom_Protocol_StateTypeDef (*Receive)(uint8_t *pData);
#if CUSTOM_PROTOCOL_TIMEOUT_ENABLE
	uint32_t (*get_tick_ms)(void);
	uint32_t timeout;
	uint32_t even_time;
#endif
	CustomProtocolMessage *msg;
	CustomProtocolRingBuffer *buffer;
	uint8_t data[CUSTOM_PROTOCOL_SIZE];
} Custom_Protocol_Handle_Typedef;

Custom_Protocol_StateTypeDef CPT_Init(Custom_Protocol_Handle_Typedef *protocol);
Custom_Protocol_StateTypeDef CPT_Transmit(Custom_Protocol_Handle_Typedef *protocol, uint8_t *data, uint16_t length);
Custom_Protocol_StateTypeDef CPT_ReceiverIT(Custom_Protocol_Handle_Typedef *protocol, uint32_t timeout);
Custom_Protocol_StateTypeDef CPT_Receiver_Process(Custom_Protocol_Handle_Typedef *protocol, uint16_t size);
Custom_Protocol_StateTypeDef CPT_GetData(Custom_Protocol_Handle_Typedef *protocol, uint8_t *data ,uint16_t *size);
Custom_Protocol_StateTypeDef CPT_GetData_Float(Custom_Protocol_Handle_Typedef *protocol, float *data ,uint16_t *size);

static uint16_t custom_protocol_checksum(uint8_t *data, uint16_t length);

#endif
