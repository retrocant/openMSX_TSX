// $Id$

#include "EventDelay.hh"
#include "EventDistributor.hh"
#include "MSXEventDistributor.hh"
#include "InputEvents.hh"
#include "FloatSetting.hh"
#include "Timer.hh"
#include "MSXException.hh"
#include "checked_cast.hh"
#include <cassert>

namespace openmsx {

EventDelay::EventDelay(Scheduler& scheduler,
                       CommandController& commandController,
                       EventDistributor& eventDistributor_,
                       MSXEventDistributor& msxEventDistributor_)
	: Schedulable(scheduler)
	, eventDistributor(eventDistributor_)
	, msxEventDistributor(msxEventDistributor_)
	, prevEmu(EmuTime::zero)
	, prevReal(Timer::getTime())
	, delaySetting(new FloatSetting(commandController, "inputdelay",
	               "delay input to avoid key-skips",
	               0.03, 0.0, 10.0))
{
	eventDistributor.registerEventListener(
		OPENMSX_KEY_DOWN_EVENT, *this, EventDistributor::MSX);
	eventDistributor.registerEventListener(
		OPENMSX_KEY_UP_EVENT,   *this, EventDistributor::MSX);

	eventDistributor.registerEventListener(
		OPENMSX_MOUSE_MOTION_EVENT,      *this, EventDistributor::MSX);
	eventDistributor.registerEventListener(
		OPENMSX_MOUSE_BUTTON_DOWN_EVENT, *this, EventDistributor::MSX);
	eventDistributor.registerEventListener(
		OPENMSX_MOUSE_BUTTON_UP_EVENT,   *this, EventDistributor::MSX);

	eventDistributor.registerEventListener(
		OPENMSX_JOY_AXIS_MOTION_EVENT, *this, EventDistributor::MSX);
	eventDistributor.registerEventListener(
		OPENMSX_JOY_BUTTON_DOWN_EVENT, *this, EventDistributor::MSX);
	eventDistributor.registerEventListener(
		OPENMSX_JOY_BUTTON_UP_EVENT,   *this, EventDistributor::MSX);
}

EventDelay::~EventDelay()
{
	eventDistributor.unregisterEventListener(
		OPENMSX_KEY_DOWN_EVENT, *this);
	eventDistributor.unregisterEventListener(
		OPENMSX_KEY_UP_EVENT,   *this);

	eventDistributor.unregisterEventListener(
		OPENMSX_MOUSE_MOTION_EVENT,      *this);
	eventDistributor.unregisterEventListener(
		OPENMSX_MOUSE_BUTTON_DOWN_EVENT, *this);
	eventDistributor.unregisterEventListener(
		OPENMSX_MOUSE_BUTTON_UP_EVENT,   *this);

	eventDistributor.unregisterEventListener(
		OPENMSX_JOY_AXIS_MOTION_EVENT, *this);
	eventDistributor.unregisterEventListener(
		OPENMSX_JOY_BUTTON_DOWN_EVENT, *this);
	eventDistributor.unregisterEventListener(
		OPENMSX_JOY_BUTTON_UP_EVENT,   *this);
}

bool EventDelay::signalEvent(EventPtr event)
{
	toBeScheduledEvents.push_back(event);
	return true;
}

void EventDelay::sync(EmuTime::param emuTime)
{
	unsigned long long curRealTime = Timer::getTime();
	unsigned long long realDuration = curRealTime - prevReal;
	EmuDuration emuDuration = emuTime - prevEmu;

	double factor = emuDuration.toDouble() / realDuration;
	EmuDuration extraDelay(delaySetting->getValue());

	EmuTime time = prevEmu + extraDelay;
	for (std::vector<EventPtr>::const_iterator it =
	        toBeScheduledEvents.begin();
	     it != toBeScheduledEvents.end(); ++it) {
		scheduledEvents.push_back(*it);
		const TimedEvent* timedEvent =
			checked_cast<const TimedEvent*>(it->get());
		unsigned long long eventRealTime = timedEvent->getRealTime();
		assert(eventRealTime <= curRealTime);
		unsigned long long offset = curRealTime - eventRealTime;
		EmuDuration emuOffset(factor * offset);
		EmuTime schedTime = time + emuOffset;
		if (schedTime < emuTime) {
			//PRT_DEBUG("input delay too short");
			schedTime = emuTime;
		}
		setSyncPoint(schedTime);
	}
	toBeScheduledEvents.clear();

	prevReal = curRealTime;
	prevEmu = emuTime;
}

void EventDelay::executeUntil(EmuTime::param time, int /*userData*/)
{
	try {
		msxEventDistributor.distributeEvent(scheduledEvents.front(), time);
	} catch (MSXException&) {
		// ignore
	}
	scheduledEvents.pop_front();
}

const std::string& EventDelay::schedName() const
{
	static const std::string name = "EventDelay";
	return name;
}

} // namespace openmsx
