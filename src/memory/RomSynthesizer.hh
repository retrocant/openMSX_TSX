// $Id$

#ifndef ROMSYNTHESIZER_HH
#define ROMSYNTHESIZER_HH

#include "RomBlocks.hh"

namespace openmsx {

class DACSound8U;

class RomSynthesizer : public Rom16kBBlocks
{
public:
	RomSynthesizer(MSXMotherBoard& motherBoard, const XMLElement& config,
	               std::auto_ptr<Rom> rom);
	virtual ~RomSynthesizer();

	virtual void reset(const EmuTime& time);
	virtual void writeMem(word address, byte value, const EmuTime& time);
	virtual byte* getWriteCacheLine(word address) const;

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	std::auto_ptr<DACSound8U> dac;
};

REGISTER_MSXDEVICE(RomSynthesizer, "RomSynthesizer");

} // namespace openmsx

#endif
