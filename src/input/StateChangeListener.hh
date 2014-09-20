#ifndef STATECHANGELISTENER_HH
#define STATECHANGELISTENER_HH

#include "EmuTime.hh"
#include <memory>

namespace openmsx {

class StateChange;

class StateChangeListener
{
public:
	/** This method gets called when a StateChange event occurs.
	 * This can be either a replayed or a 'live' event, (though that
	 * shouldn't matter, it should be handled in exactly the same way).
	 */
	virtual void signalStateChange(
		const std::shared_ptr<StateChange>& event) = 0;

	/** This method gets called when we switch from replayed events to
	 * live events. A input device should resync its state with the current
	 * host state. A replayer/recorder should switch from replay to record.
	 * Note that it's not possible to switch back to replay state (when
	 * the user triggers a replay, we always start from a snapshot, so
	 * we create 'fresh' objects).
	 */
	virtual void stopReplay(EmuTime::param time) = 0;

protected:
	StateChangeListener() {}
	~StateChangeListener() {}
};

class StateChangeRecorder : public StateChangeListener
{
public:
	virtual bool isReplaying() const = 0;
};

} // namespace openmsx

#endif
