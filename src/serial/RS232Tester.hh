// $Id$

#ifndef __RS232TESTER_HH__
#define __RS232TESTER_HH__

#include <fstream>
#include <cstdio>
#include <list>
#include "openmsx.hh"
#include "RS232Device.hh"
#include "Thread.hh"
#include "Schedulable.hh"
#include "Semaphore.hh"
#include "StringSetting.hh"
#include "FilenameSetting.hh"

using std::list;
using std::ofstream;


namespace openmsx {

class RS232Connector;


class RS232Tester : public RS232Device, private Runnable, private Schedulable
{
public:
	RS232Tester();
	virtual ~RS232Tester();

	// Pluggable
	virtual void plug(Connector* connector, const EmuTime& time)
		throw(PlugException);
	virtual void unplug(const EmuTime& time);
	virtual const string& getName() const;
	virtual const string& getDescription() const;

	// input
	virtual void signal(const EmuTime& time);

	// output
	virtual void recvByte(byte value, const EmuTime& time);

private:
	// Runnable
	virtual void run();

	// Schedulable
	virtual void executeUntil(const EmuTime& time, int userData) throw();
	virtual const string& schedName() const;

	Thread thread;
	FILE* inFile;
	RS232Connector* connector;
	list<byte> queue;
	Semaphore lock; // to protect queue

	ofstream outFile;

	FilenameSetting rs232InputFilenameSetting;
	StringSetting rs232OutputFilenameSetting;
};

} // namespace openmsx

#endif // __RS232TESTER_HH__
