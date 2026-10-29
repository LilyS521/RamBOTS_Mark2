#ifndef RAMBOT_MPU_MESSAGE_H
#define RAMBOT_MPU_MESSAGE_H
#include <bitset>
#include <cinttypes>
#include <array>



enum MessageTypeEnum
{
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3
};


struct MPUMessage
{
	MPUMessage(MessageTypeEnum message_type, uint8_t device_id, uint16_t message_data);

	MessageTypeEnum getMessageType() const;
	uint16_t getMessage() const;
	int64_t checkErrors() const;
	bool fixError();

	inline uint8_t getDeviceID() const { return dvc_id; }
	inline MessageTypeEnum getMessageType() const { return msg_type; }
	inline uint32_t getRawData() const { return raw_data; }
	inline void setRawData(uint32_t raw_data) { this->raw_data = raw_data; }

private:
	void setMessage(uint16_t data);
	void setHammingCode();

	union
	{
		struct
		{
			uint32_t msg_type : 2;
			uint32_t dvc_id : 8;
			uint32_t msg_1 : 5;
			uint32_t hc_1 : 1;
			uint32_t msg_2 : 7;
			uint32_t hc_2 : 1;
			uint32_t msg_3 : 3;
			uint32_t hc_3 : 1;
			uint32_t msg_4 : 1;
			uint32_t hc_4 : 3;
		};
		std::bitset<32> bit_data;
		uint32_t raw_data;
	};

	constexpr static std::array<std::array<uint8_t, 16>, 5> idx_arrays = {{{ 0,  2,  4,  6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30},
																		   { 0,  1,  4,  5,  8,  9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29},
																		   { 0,  1,  2,  3,  8,  9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27},
																		   { 0,  1,  2,  3,  4,  5,  6,  7, 16, 17, 18, 19, 20, 21, 22, 23},
																		   { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}}};
};


#endif //RAMBOT_MPU_MESSAGE_H
