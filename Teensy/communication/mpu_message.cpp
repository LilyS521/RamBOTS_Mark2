#include "mpu_message.h"


/************************************************
Inputs: MessageTypeEnum message_type, uint8_t device_id, uint16_t message_data
Output: MPUMessage instance
Function: Encode the arguments into the MPUMessage data
Test files/test location: message_test.cpp
*************************************************/
MPUMessage::MPUMessage(MessageTypeEnum message_type, uint8_t device_id, uint16_t message_data)
{
	msg_type = message_type;
	dvc_id = device_id;
	setMessage(message_data);
	setHammingCode();
}

/************************************************
Inputs: void
Output: MessageTypeEnum
Function: Parse and return the message type encoded in the MPUMessage data
Test files/test location: message_test.cpp
*************************************************/
MessageTypeEnum MPUMessage::getMessageType() const
{
	switch (msg_type)
	{
		case 0:
			return MessageTypeEnum::DEBUG;
		case 1:
			return MessageTypeEnum::INFO;
		case 2:
			return MessageTypeEnum::WARNING;
		default:
			return MessageTypeEnum::ERROR;
	}
}

/************************************************
Inputs: void
Output: uint16_t(two data byte array encoded together as uint16_t)
Function: Parse and return the message data encoded in the MPUMessage data
Test files/test location: message_test.cpp
*************************************************/
uint16_t MPUMessage::getMessage() const
{
	uint16_t res = msg_1;

	res <<= 7;
	res |= msg_2;

	res <<= 3;
	res |= msg_3;

	res <<= 1;
	res |= msg_4;

	return res;
}

/************************************************
Inputs: void
Output: bool(true if the data doesn't contain an error, either because of fix or didn't have error to begin with)
Function: Check the encoded MPUMessage data for errors and fix them if they exist and are fixable
Test files/test location: hamming_test.cpp
*************************************************/
bool MPUMessage::fixError()
{
	auto error_idx = checkErrors();
	if (error_idx < 0)
		return false;
	else if (error_idx > 0)
		bit_data.flip(error_idx);
	return true;
}

/************************************************
Inputs: uint16_t data(two data byte array encoded together as uint16_t)
Output: void
Function: Set the encoded message data and update the hamming code accordingly
Test files/test location: hamming_test.cpp
*************************************************/
void MPUMessage::setMessage(uint16_t data)
{
	msg_1 = data >> 11;
	msg_2 = (data >> 4) & 0x7F;
	msg_3 = (data >> 1) & 0x07;
	msg_4 = data & 0x01;
	setHammingCode();
}

/************************************************
Inputs: void
Output: void
Function: Updates the hamming code that is encoded in the MPUMessage data
Test files/test location: hamming_test.cpp
*************************************************/
void MPUMessage::setHammingCode()
{
	/*


	 XXXXXXXX
	 XXXXXXX0
	 XXXXXXX0
	 XXX0X000

	 MMDDDDDD
	 DDIIIII0
	 IIIIIII0
	 III0I000


	 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30
	 0 1 4 5 8 9 12 13 16 17 20 21 24 25 28 29
	 0 1 2 3 8 9 10 11 16 17 18 19 24 25 26 27

	 0 1 2 3 4 5 6 7 16 17 18 19 20 21 22 23
	 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15

	 */

	uint8_t idx_xor;

	for (auto idx_ar : idx_arrays)
	{
		idx_xor = 0;
		for (uint8_t bit_idx : idx_ar)
			if (bit_data.test(bit_idx)) idx_xor ^= bit_idx;
		if (idx_xor != 0) bit_data.flip(idx_xor);
	}
}

/************************************************
Inputs: void
Output: int64_t(return 0 if no error, -1 if more than one bit error, or return the index of the single bit error itself(always >0))
Function: Check the encoded MPUMessage data for errors
Test files/test location: hamming_test.cpp
*************************************************/
int64_t MPUMessage::checkErrors() const
{
	uint8_t parity_count = 0;
	uint32_t idx_xor = 0;

	for (uint32_t bit_idx = 0; bit_idx < 32; bit_idx++)
	{
		if (bit_data.test(bit_idx))
		{
			idx_xor ^= bit_idx;
			parity_count++;
		}
	}
	
	bool is_odd = (parity_count % 2) == 1;
	
	if (idx_xor > 0)
		return (is_odd) ? (int64_t)idx_xor : -1;
	else if (!is_odd)
		return -1;
	else
		return 0;
}
