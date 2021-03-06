#include "ZlibInflate.hh"
#include "FileException.hh"
#include "MemBuffer.hh"
#include "StringOp.hh"
#include <limits>

namespace openmsx {

ZlibInflate::ZlibInflate(const byte* input, size_t inputLen_)
{
	if (inputLen_ > std::numeric_limits<decltype(s.avail_in)>::max()) {
		throw FileException(
			"Error while decompressing: input file too big");
	}
	auto inputLen = static_cast<decltype(s.avail_in)>(inputLen_);

	s.zalloc = nullptr;
	s.zfree  = nullptr;
	s.opaque = nullptr;
	s.next_in  = const_cast<byte*>(input);
	s.avail_in = inputLen;
	wasInit = false;
}

ZlibInflate::~ZlibInflate()
{
	if (wasInit) {
		inflateEnd(&s);
	}
}

void ZlibInflate::skip(size_t num)
{
	for (size_t i = 0; i < num; ++i) {
		getByte();
	}
}

byte ZlibInflate::getByte()
{
	if (s.avail_in <= 0) {
		throw FileException(
			"Error while decompressing: unexpected end of file.");
	}
	--s.avail_in;
	return *(s.next_in++);
}

unsigned ZlibInflate::get16LE()
{
	unsigned result = getByte();
	result += getByte() << 8;
	return result;
}

unsigned ZlibInflate::get32LE()
{
	unsigned result = getByte();
	result += getByte() <<  8;
	result += getByte() << 16;
	result += getByte() << 24;
	return result;
}

std::string ZlibInflate::getString(size_t len)
{
	std::string result;
	for (size_t i = 0; i < len; ++i) {
		result.push_back(getByte());
	}
	return result;
}

std::string ZlibInflate::getCString()
{
	std::string result;
	while (char c = getByte()) {
		result.push_back(c);
	}
	return result;
}

size_t ZlibInflate::inflate(MemBuffer<byte>& output, size_t sizeHint)
{
	int initErr = inflateInit2(&s, -MAX_WBITS);
	if (initErr != Z_OK) {
		throw FileException(StringOp::Builder()
			<< "Error initializing inflate struct: " << zError(initErr));
	}
	wasInit = true;

	size_t outSize = sizeHint;
	output.resize(outSize);
	s.avail_out = uInt(outSize); // TODO overflow?
	while (true) {
		s.next_out = output.data() + s.total_out;
		int err = ::inflate(&s, Z_NO_FLUSH);
		if (err == Z_STREAM_END) {
			break;
		}
		if (err != Z_OK) {
			throw FileException(StringOp::Builder()
				<< "Error decompressing gzip: " << zError(err));
		}
		auto oldSize = outSize;
		outSize = oldSize * 2; // double buffer size
		output.resize(outSize);
		s.avail_out = uInt(outSize - oldSize); // TODO overflow?
	}

	// set actual size
	output.resize(s.total_out);
	return s.total_out;
}

} // namespace openmsx
