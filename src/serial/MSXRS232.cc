// $Id$

#include <cassert>
#include "MSXRS232.hh"
#include "RS232Device.hh"


namespace openmsx {

MSXRS232::MSXRS232(Device *config, const EmuTime &time)
	: MSXDevice(config, time)
	, MSXIODevice(config, time)
	, MSXMemDevice(config, time)
	, RS232Connector("msx-rs232")
	, rxrdyIRQlatch(false)
	, rxrdyIRQenabled(false)
	, cntr0(*this), cntr1(*this)
	, i8254(&cntr0, &cntr1, NULL, time)
	, interf(*this)
	, i8251(&interf, time)
	, rom(config, time)
{
	EmuDuration total(1.0 / 1.8432e6); // 1.8432MHz
	EmuDuration hi   (1.0 / 3.6864e6); //   half clock period
	i8254.getClockPin(0).setPeriodicState(total, hi, time);
	i8254.getClockPin(1).setPeriodicState(total, hi, time);
	i8254.getClockPin(2).setPeriodicState(total, hi, time);
	reset(time);
}

MSXRS232::~MSXRS232()
{
}

void MSXRS232::reset(const EmuTime &time)
{
	rxrdyIRQlatch = false;
	rxrdyIRQenabled = false;
	rxrdyIRQ.reset();
}

byte MSXRS232::readMem(word address, const EmuTime &time)
{
	return rom.read(address & 0x3FFF);
}

const byte* MSXRS232::getReadCacheLine(word start) const
{
	return rom.getBlock(start & 0x3FFF);
}

byte MSXRS232::readIO(byte port, const EmuTime &time)
{
	byte result;
	port &= 0x07;
	switch (port) {
		case 0: // UART data register
		case 1: // UART status register
			result = i8251.readIO(port, time);
			break;
		case 2: // Status sense port
			result = readStatus(time);
			break;
		case 3: // no function
			result = 0xFF;
			break;
		case 4: // counter 0 data port
		case 5: // counter 1 data port
		case 6: // counter 2 data port
		case 7: // timer command register
			result = i8254.readIO(port - 4, time);
			break;
		default:
			assert(false);
			result = 0xFF;	// avoid warning
	}
	//PRT_DEBUG("MSXRS232 read " << (int)port << " " << (int)result);
	return result;
}

void MSXRS232::writeIO(byte port, byte value, const EmuTime &time)
{
	//PRT_DEBUG("MSXRS232 write " << (int)port << " " << (int)value);
	port &= 0x07;
	switch (port & 0x07) {
		case 0: // UART data register
		case 1: // UART command register
			i8251.writeIO(port, value, time);
			break;
		case 2: // interrupt mask register
			setIRQMask(value);
			break;
		case 3: // no function
			break;
		case 4: // counter 0 data port
		case 5: // counter 1 data port
		case 6: // counter 2 data port
		case 7: // timer command register
			i8254.writeIO(port - 4, value, time);
			break;
	}
}

byte MSXRS232::readStatus(const EmuTime& time)
{
	byte result = 0;	// TODO check unused bits
	if (!interf.getCTS(time)) {
		result |= 0x80;
	}
	if (i8254.getOutputPin(2).getState(time)) {
		result |= 0x40;
	}
	return result;
}

void MSXRS232::setIRQMask(byte value)
{
	enableRxRDYIRQ(!(value & 1));
}

void MSXRS232::setRxRDYIRQ(bool status)
{
	if (rxrdyIRQlatch != status) {
		rxrdyIRQlatch = status;
		if (rxrdyIRQenabled) {
			if (rxrdyIRQlatch) {
				rxrdyIRQ.set();
			} else {
				rxrdyIRQ.reset();
			}
		}
	}
}

void MSXRS232::enableRxRDYIRQ(bool enabled)
{
	if (rxrdyIRQenabled != enabled) {
		rxrdyIRQenabled = enabled;
		if (!rxrdyIRQenabled && rxrdyIRQlatch) {
			rxrdyIRQ.reset();
		}
	}
}


// I8251Interface  (pass calls from I8251 to outConnector)

MSXRS232::I8251Interf::I8251Interf(MSXRS232& rs232_)
	: rs232(rs232_)
{
}

MSXRS232::I8251Interf::~I8251Interf()
{
}

void MSXRS232::I8251Interf::setRxRDY(bool status, const EmuTime& time)
{
	rs232.setRxRDYIRQ(status);
}

void MSXRS232::I8251Interf::setDTR(bool status, const EmuTime& time)
{
	((RS232Device*)rs232.pluggable)->setDTR(status, time);
}

void MSXRS232::I8251Interf::setRTS(bool status, const EmuTime& time)
{
	((RS232Device*)rs232.pluggable)->setRTS(status, time);
}

bool MSXRS232::I8251Interf::getDSR(const EmuTime& time)
{
	return ((RS232Device*)rs232.pluggable)->getDSR(time);
}

bool MSXRS232::I8251Interf::getCTS(const EmuTime& time)
{
	return ((RS232Device*)rs232.pluggable)->getCTS(time);
}

void MSXRS232::I8251Interf::setDataBits(DataBits bits)
{
	((RS232Device*)rs232.pluggable)->setDataBits(bits);
}

void MSXRS232::I8251Interf::setStopBits(StopBits bits)
{
	((RS232Device*)rs232.pluggable)->setStopBits(bits);
}

void MSXRS232::I8251Interf::setParityBit(bool enable, ParityBit parity)
{
	((RS232Device*)rs232.pluggable)->setParityBit(enable, parity);
}

void MSXRS232::I8251Interf::recvByte(byte value, const EmuTime& time)
{
	((RS232Device*)rs232.pluggable)->recvByte(value, time);
}

void MSXRS232::I8251Interf::signal(const EmuTime& time)
{
	((RS232Device*)rs232.pluggable)->signal(time); // for input
}


// Counter 0 output

MSXRS232::Counter0::Counter0(MSXRS232& rs232_)
	: rs232(rs232_)
{
}

MSXRS232::Counter0::~Counter0()
{
}

void MSXRS232::Counter0::signal(ClockPin& pin, const EmuTime& time)
{
	ClockPin& clk = rs232.i8251.getClockPin();
	if (pin.isPeriodic()) {
		clk.setPeriodicState(pin.getTotalDuration(),
		                     pin.getHighDuration(), time);
	} else {
		clk.setState(pin.getState(time), time);
	}
}

void MSXRS232::Counter0::signalPosEdge(ClockPin& pin, const EmuTime& time)
{
	assert(false);
}


// Counter 1 output // TODO split rx tx

MSXRS232::Counter1::Counter1(MSXRS232& rs232_)
	: rs232(rs232_)
{
}

MSXRS232::Counter1::~Counter1()
{
}

void MSXRS232::Counter1::signal(ClockPin& pin, const EmuTime& time)
{
	ClockPin& clk = rs232.i8251.getClockPin();
	if (pin.isPeriodic()) {
		clk.setPeriodicState(pin.getTotalDuration(),
		                     pin.getHighDuration(), time);
	} else {
		clk.setState(pin.getState(time), time);
	}
}

void MSXRS232::Counter1::signalPosEdge(ClockPin& pin, const EmuTime& time)
{
	assert(false);
}


// RS232Connector input

bool MSXRS232::ready()
{
	return i8251.isRecvReady();
}

bool MSXRS232::acceptsData()
{
	return i8251.isRecvEnabled();
}

void MSXRS232::setDataBits(DataBits bits)
{
	i8251.setDataBits(bits);
}

void MSXRS232::setStopBits(StopBits bits)
{
	i8251.setStopBits(bits);
}

void MSXRS232::setParityBit(bool enable, ParityBit parity)
{
	i8251.setParityBit(enable, parity);
}

void MSXRS232::recvByte(byte value, const EmuTime& time)
{
	i8251.recvByte(value, time);
}

} // namespace openmsx

