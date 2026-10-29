#include <random>
#include <chrono>
#include <limits>
#include <cstdint>
#include <array>
#include "mpu_message.h"



constexpr std::array<uint32_t, 31> two_bit_flips = { 0xC0000000, 0xA0000000, 0x90000000, 0x88000000, 0x84000000, 0x82000000, 0x81000000,
													 0x80800000, 0x80400000, 0x80200000, 0x80100000, 0x80080000, 0x80040000, 0x80020000,
													 0x80010000, 0x80008000, 0x80004000, 0x80002000, 0x80001000, 0x80000800, 0x80000400,
													 0x80000200, 0x80000100, 0x80000080, 0x80000040, 0x80000020, 0x80000010, 0x80000008,
													 0x80000004, 0x80000002, 0x80000001};


bool hammingTest(int trials, uint32_t seed = chrono::system_clock::now().time_since_epoch().count())
{
    std::mt19937 generator(seed);
	std::uniform_int_distribution<uint32_t> distribution(0, std::numeric_limits<uint32_t>::max());
	auto rand_data = std::bind(distribution, generator);
	
	uint32_t bit_flipper;
	auto msg = MPUMessage(MessageTypeEnum::INFO, 0, 0);
	
	for (int trial = 0; trial < trials; trials++)
	{
		uint32_t raw_msg_data = rand_data();
		msg.setRawData(raw_msg_data);
		
		auto old_device_id = msg.getDeviceID();
		auto old_message_type = msg.getMessageType();
		auto old_message = msg.getMessage();
		
		bool error_free;
		auto error_idx = msg.checkErrors();
		assert (error_idx == 0);
		
		uint32_t mod_msg_data;
		bit_flipper = 0x80000000;
		for (int i = 0; i < 31; i++)
		{
			mod_msg_data = bit_flipper ^ raw_msg_data;
			msg.setRawData(mod_msg_data);
			error_idx = msg.checkErrors();
			
			assert (error_idx > 0);
			assert (msg.getRawData() != raw_msg_data);
			
			error_free = msg.fixError();
			error_idx = msg.checkErrors();
			
			assert (error_free);
			assert (error_idx == 0);
			assert (msg.getRawData() == raw_msg_data);
			
			bit_flipper >>= 1;
		}
		
		for (bit_flipper : two_bit_flips)
		{
			mod_msg_data = bit_flipper ^ raw_msg_data;
			msg.setRawData(mod_msg_data);
			error_idx = msg.checkErrors();
			
			assert (error_idx < 0);
			assert (msg.getRawData() != raw_msg_data);
			
			error_free = msg.fixError();
			msg.setRawData(raw_msg_data);
			
			assert (!error_free);
			assert (error_idx == 0);
			assert (msg.getRawData() == raw_msg_data);
		}
	}
	
	return true;
}