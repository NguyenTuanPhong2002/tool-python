/*
 * File:    customprotocol.h
 * Author:  Nguyen Tuan Phong
 * Date:    2024-09-05
 */

#include "customprotocol.h"

CustomProtocolMessage msg = {0};
CustomProtocolRingBuffer buffer = {0};

static uint16_t
custom_protocol_checksum(uint8_t *data, uint16_t length)
{
    uint16_t checksum = 0;
    for (uint16_t i = 0; i < length; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

Custom_Protocol_StateTypeDef CPT_Init(Custom_Protocol_Handle_Typedef *protocol)
{
    if (protocol == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }

    protocol->msg = &msg;
    protocol->buffer = &buffer;

    return protocol->Init();
}

Custom_Protocol_StateTypeDef CPT_Transmit(Custom_Protocol_Handle_Typedef *protocol, uint8_t *data, uint16_t length)
{
    if (protocol == NULL || length > CUSTOM_PROTOCOL_SIZE || data == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }

    protocol->data[0] = CUSTOM_PROTOCOL_START_BIT;
    protocol->data[1] = CUSTOM_PROTOCOL_COMMAND_WRITE;
    protocol->data[2] = (length >> 8) & 0xFF;
    protocol->data[3] = length & 0xFF;
    for (int16_t i = 0; i < length; i++)
    {
        protocol->data[4 + i] = data[i];
    }
    protocol->msg->checksum = custom_protocol_checksum(data, length);

    protocol->data[4 + length] = (protocol->msg->checksum >> 8) & 0xFF;
    protocol->data[5 + length] = protocol->msg->checksum & 0xFF;
    protocol->data[6 + length] = CUSTOM_PROTOCOL_END_BIT;

    return protocol->Transmit(protocol->data, length + 7);
}

Custom_Protocol_StateTypeDef CPT_ReceiverIT(Custom_Protocol_Handle_Typedef *protocol, uint32_t timeout)
{
    if (protocol == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }

    protocol->timeout = timeout;
    protocol->even_time = protocol->get_tick_ms();

    protocol->msg->step = CUSTOM_PROTOCOL_STEP_START_BIT;

    protocol->Receive(protocol->data);

    return CUSTOM_PROTOCOL_STATE_OK;
}

Custom_Protocol_StateTypeDef CPT_Receiver_Process(Custom_Protocol_Handle_Typedef *protocol, uint16_t size)
{
    if (protocol == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }

    protocol->buffer->size = size;

    uint16_t index = 0;
    while (index < protocol->buffer->size)
    {
        protocol->buffer->data[protocol->buffer->write_index] = protocol->data[index];
        if (++protocol->buffer->write_index > CUSTOM_PROTOCOL_RING_BUFFER_SIZE)
        {
            protocol->buffer->write_index = 0;
        }
        index++;
    }

    Custom_Protocol_StateTypeDef status = CUSTOM_PROTOCOL_STATE_ERROR;

    uint16_t data_index = 0;

    for (uint16_t i = 0; i < protocol->buffer->size; i++)
    {

#if CUSTOM_PROTOCOL_TIMEOUT_ENABLE
        if ((protocol->get_tick_ms() - protocol->even_time) > protocol->timeout)
        {
            protocol->msg->step = CUSTOM_PROTOCOL_STEP_START_BIT;
            return CUSTOM_PROTOCOL_STATE_TIMEOUT;
        }
#endif

        switch (protocol->msg->step)
        {
        case CUSTOM_PROTOCOL_STEP_START_BIT:
            if (protocol->buffer->data[protocol->buffer->read_index] == CUSTOM_PROTOCOL_START_BIT)
            {
                protocol->msg->start_bit = protocol->buffer->data[protocol->buffer->read_index];
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_COMMAND;
            }
            else
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_START_BIT;
            }
            break;
        case CUSTOM_PROTOCOL_STEP_COMMAND:
            if (protocol->buffer->data[protocol->buffer->read_index] == CUSTOM_PROTOCOL_COMMAND_READ)
            {
                protocol->msg->command = protocol->buffer->data[protocol->buffer->read_index];
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_LENGTH0;
            }
            else
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_START_BIT;
            }
            break;

        case CUSTOM_PROTOCOL_STEP_LENGTH0:
            protocol->msg->length = protocol->buffer->data[protocol->buffer->read_index];
            protocol->msg->step = CUSTOM_PROTOCOL_STEP_LENGTH1;
            break;
        case CUSTOM_PROTOCOL_STEP_LENGTH1:
            protocol->msg->length = (protocol->msg->length << 8) | protocol->buffer->data[protocol->buffer->read_index];
            if (protocol->msg->length <= CUSTOM_PROTOCOL_SIZE)
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_DATA;
            }
            else
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_START_BIT;
            }
            break;
            break;
        case CUSTOM_PROTOCOL_STEP_DATA:
            protocol->msg->data[data_index++] = protocol->buffer->data[protocol->buffer->read_index];
            if (data_index > protocol->msg->length - 1)
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_CRC1;
            }
            break;
        case CUSTOM_PROTOCOL_STEP_CRC1:
            protocol->msg->checksum = protocol->buffer->data[protocol->buffer->read_index];
            protocol->msg->step = CUSTOM_PROTOCOL_STEP_CRC2;
            break;
        case CUSTOM_PROTOCOL_STEP_CRC2:
            protocol->msg->checksum = (protocol->msg->checksum << 8) | protocol->buffer->data[protocol->buffer->read_index];
            protocol->msg->step = CUSTOM_PROTOCOL_STEP_CRC2;
            if (protocol->msg->checksum == custom_protocol_checksum(protocol->msg->data, protocol->msg->length))
            {
                protocol->msg->step = CUSTOM_PROTOCOL_STEP_END_BIT;
            }
            break;
        case CUSTOM_PROTOCOL_STEP_END_BIT:
            if (protocol->buffer->data[protocol->buffer->read_index] == CUSTOM_PROTOCOL_END_BIT)
            {
                status = CUSTOM_PROTOCOL_STATE_OK;
                if (++protocol->buffer->read_index > CUSTOM_PROTOCOL_RING_BUFFER_SIZE)
                {
                    protocol->buffer->read_index = 0;
                }
                return status;
            }
            break;
        default:
            break;
        }

        if (++protocol->buffer->read_index > CUSTOM_PROTOCOL_RING_BUFFER_SIZE)
        {
            protocol->buffer->read_index = 0;
        }
    }

    protocol->Receive(protocol->data);

    return status;
}

Custom_Protocol_StateTypeDef CPT_GetData(Custom_Protocol_Handle_Typedef *protocol, uint8_t *data, uint16_t *size)
{
    if (protocol == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }
    *size = protocol->msg->length;
    for (uint16_t i = 0; i < *size; i++)
    {
        data[i] = protocol->msg->data[i];
    }

    return CUSTOM_PROTOCOL_STATE_OK;
}

Custom_Protocol_StateTypeDef CPT_GetData_Float(Custom_Protocol_Handle_Typedef *protocol, float *data, uint16_t *size)
{
    if (protocol == NULL)
    {
        return CUSTOM_PROTOCOL_STATE_ERROR;
    }
    uint16_t sizeBuffer = 0;
    union ByteToFloat
    {
        uint8_t data[4];
        float value;
    };

    union ByteToFloat buffer;

    uint16_t i = 0;

    while (i < protocol->msg->length)
    {
        buffer.data[0] = protocol->msg->data[i];
        buffer.data[1] = protocol->msg->data[i + 1];
        buffer.data[2] = protocol->msg->data[i + 2];
        buffer.data[3] = protocol->msg->data[i + 3];

        data[sizeBuffer++] = buffer.value;

        i = i + 4;
    }

    *size = sizeBuffer;

    return CUSTOM_PROTOCOL_STATE_OK;
}