#ifndef RAMBOT_RAM_SPI_H
#define RAMBOT_RAM_SPI_H
#include <SPI.h>

/*
 * https://github.com/mavriq-dev/Teensy-Documentation/blob/main/SPI.md
 *
 *
 * SPI_MODE0: CPOL=0, CPHA=0  // Clock idle low,  data sampled on rising edge
 * SPI_MODE1: CPOL=0, CPHA=1  // Clock idle low,  data sampled on falling edge
 * SPI_MODE2: CPOL=1, CPHA=0  // Clock idle high, data sampled on falling edge
 * SPI_MODE3: CPOL=1, CPHA=1  // Clock idle high, data sampled on rising edge
 */

/// Compresses pin and data mode choices into int for template usage
template <int cs_pin, int sck_pin, int read_pin, int write_pin, uint8_t data_mode>
constexpr static int compileTemplateData()
{
	int res = ((data_mode == 0x00) || (data_mode == 0x0C)) ? 1 : 0;

	res <<= 7;
	res |= (0x7F & cs_pin);

	res <<= 8;
	res |= (0xFF & sck_pin);

	res <<= 8;
	res |= (0xFF & read_pin);

	res <<= 8;
	res |= (0xFF & write_pin);

	return res;
}


constexpr int CS_PINS[3] = {10, 0, 44};
constexpr int PICO_PINS[3] = {11, 26, 43};
constexpr int POCI_PINS[3] = {12, 1, 42};
constexpr int SCK_PINS[3] = {13, 27, 45};
constexpr uint8_t SPI_MODE_READ_STATE[4] = {true, false, false, true};


/// Stores the SPI Bus pin settings
struct SPIBusPins
{
	SPIBusPins(int cs_pin, int sck_pin, int read_pin, int write_pin) : cs_pin(cs_pin), sck_pin(sck_pin),
	read_pin(read_pin), write_pin(write_pin) {}

	/// Create an SPIBusPins object based on a predefined bus index.
	/// spi_bus_idx valid values: 0 - 2
	/// is_controller should be true if the bus is a Controller, or false if it is a Peripheral
	static SPIBusPins fromBusIndex(uint8_t spi_bus_idx, bool is_controller)
	{
		auto cs_pin = CS_PINS[spi_bus_idx];
		auto sck_pin = SCK_PINS[spi_bus_idx];
		auto read_pin = (is_controller) ? POCI_PINS[spi_bus_idx] : PICO_PINS[spi_bus_idx];
		auto write_pin = (is_controller) ? PICO_PINS[spi_bus_idx] : POCI_PINS[spi_bus_idx];

		return {cs_pin, sck_pin, read_pin, write_pin};
	}

	int cs_pin, sck_pin, read_pin, write_pin;
};


/// Abstract base class for SPI Communication
//template<const SPIBusPins* SPI_BUS_PINS>
template<int SPI_BUS_PINS>
class RamSPIBase
{
public:
	RamSPIBase(uint32_t clock_hz, uint8_t bit_order, uint8_t data_mode) : settings(SPISettings(clock_hz, bit_order, data_mode)) {}

	virtual void setup() = 0;

	/// Parse data mode index from compressed template argument SPI_BUS_PINS
	static constexpr int data_mode_idx() { return SPI_BUS_PINS >> 31; }

	/// Parse Chip Select pin from compressed template argument SPI_BUS_PINS
	static constexpr int cs_pin() { return 0x7F & (SPI_BUS_PINS >> 24); }

	/// Parse Serial Clock pin from compressed template argument SPI_BUS_PINS
	static constexpr int sck_pin() { return 0xFF & (SPI_BUS_PINS >> 16); }

	/// Parse Serial Read pin from compressed template argument SPI_BUS_PINS
	static constexpr int read_pin() { return 0xFF & (SPI_BUS_PINS >> 8); }

	/// Parse Serial Write pin from compressed template argument SPI_BUS_PINS
	static constexpr int write_pin() { return 0xFF & SPI_BUS_PINS; }

protected:
	const SPISettings settings;
};


/// Controller Object for SPI Communication Bus handling
template<int SPI_BUS_PINS>
class Controller : public RamSPIBase<SPI_BUS_PINS>
{
public:
	Controller(uint32_t clock_hz, uint8_t bit_order, uint8_t data_mode) :
	RamSPIBase<SPI_BUS_PINS>(clock_hz, bit_order, data_mode) {}

	/// Setup should always be ran once per object within the setup() method of main.cpp
	void setup() override
   	{
		// Assign signal direction to associated pins
		pinMode(this->cs_pin(), OUTPUT);
	    pinMode(this->sck_pin(), OUTPUT);
	    pinMode(this->write_pin(), OUTPUT);
	    pinMode(this->read_pin(), INPUT);

		// Assign signal function to associated pins
	    SPI.setCS(this->cs_pin());
	    SPI.setMISO(this->read_pin());
	    SPI.setMOSI(this->write_pin());
	    SPI.setSCK(this->sck_pin());
	    SPI.begin();

		// CS is active when LOW
	    digitalWriteFast(this->cs_pin(), HIGH);
    }

	/// Begin simultaneously transmit and receive a byte of data from the assigned SPI bus
    uint8_t transmit(uint8_t data)
    {
	    SPI.beginTransaction(this->settings);
	    auto res = SPI.transfer(data);
	    SPI.endTransaction();
	    return res;
    }
};


/// Peripheral Object for SPI Communication Bus handling
template<int SPI_BUS_PINS>
class Peripheral : public RamSPIBase<SPI_BUS_PINS>
{
public:
	Peripheral(uint32_t clock_hz, uint8_t bit_order, uint8_t data_mode) : RamSPIBase<SPI_BUS_PINS>(clock_hz, bit_order, data_mode) {}

	/// Setup should always be ran once per object within the setup() method of main.cpp
	void setup() override
	{
		// Assign signal direction to associated pins
		pinMode(this->cs_pin(), INPUT);
		pinMode(this->sck_pin(), INPUT);
		pinMode(this->write_pin(), OUTPUT);
		pinMode(this->read_pin(), INPUT);

		// Assign signal function to associated pins
		SPI.setCS(this->cs_pin());
		SPI.setMISO(this->write_pin());
		SPI.setMOSI(this->read_pin());
		SPI.setSCK(this->sck_pin());
		SPI.begin();

		// CS is active when LOW
		attachInterrupt(digitalPinToInterrupt(this->cs_pin()), transmit, LOW);
	}

protected:
	/// Called via interrupt when the CS signal is high
	/// Begin simultaneously transmitting and receiving data while the CS signal is still high, through the assigned SPI bus
	static void transmit()
	{
		// Initial data write to get ahead of the SCK signal
		writeSPI();

		attachInterrupt(digitalPinToInterrupt(Peripheral<SPI_BUS_PINS>::sck_pin()), handleSPI, CHANGE); // Takes approx 1 us

		// Wait some time(TBD) before checking the CS signal again
		while (!digitalReadFast(Peripheral<SPI_BUS_PINS>::cs_pin()))
			delayMicroseconds(1);

		detachInterrupt(digitalPinToInterrupt(Peripheral<SPI_BUS_PINS>::sck_pin())); // Takes approx 1 us
	}

	/// Called when the SCK signal changes
	/// Handle whichever value the SCK signal changed to appropriately
	/// Reading data when in one state, and writing it while inverse
	/// The variable that decides which state is reading or writing is data_mode
	static void handleSPI()
	{
		// Read the current state from the SCK signal
		auto state = digitalReadFast(Peripheral<SPI_BUS_PINS>::sck_pin());

		// Read the SPI data if the state matches data_mode_idx, or writes SPI data otherwise
		if (state == Peripheral<SPI_BUS_PINS>::data_mode_idx()) readSPI();
		else writeSPI();
	}

	/// Read a bit of data into the buffer
	static bool readSPI()
	{
		auto data_in = (bool)digitalReadFast(Peripheral<SPI_BUS_PINS>::read_pin());
		return data_in;
	}

	/// Write a bit of data into the buffer
	static void writeSPI(bool data_out)
	{
		digitalWriteFast(Peripheral<SPI_BUS_PINS>::write_pin(), (uint8_t)data_out);
	}
};

#endif //RAMBOT_RAM_SPI_H
