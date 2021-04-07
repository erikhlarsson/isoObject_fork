#pragma once

#include <set>
#include <map>
#include <iostream>
#include <functional>
#include "iso22133.h"

// TODO replace these with the ISO structs
typedef struct {int a;} strt;
typedef struct {int a;} traj;

namespace ISO22133 {
class TestObject;


// TODO this should be in iso22133.h
// typedef ObjectStateType ObjectStateID;
typedef enum {
	ISO_OBJECT_STATE_UNKNOWN = -1,
	ISO_OBJECT_STATE_OFF = 0,
	ISO_OBJECT_STATE_INIT = 1,
	ISO_OBJECT_STATE_ARMED = 2,
	ISO_OBJECT_STATE_DISARMED = 3,
	ISO_OBJECT_STATE_RUNNING = 4,
	ISO_OBJECT_STATE_POSTRUN = 5,
	ISO_OBJECT_STATE_REMOTE_CONTROLLED = 6,
	ISO_OBJECT_STATE_ABORTING = 7,
	ISO_OBJECT_STATE_PRE_ARMING = 8,
	ISO_OBJECT_STATE_PRE_RUNNING = 9
} ObjectStateID;

namespace Events {
typedef enum {
	D, //!< system started
	L, //!< control center connection lost
	B, //!< control center connection established
	H, //!< remote control requested
	M, //!< arm requested
	N, //!< arm completed
	S, //!< start requested
	T, //!< start completed
	U, //!< soft stop requested
	Y, //!< scenario completed
	R, //!< object stopped
	W, //!< emergency stop requested
	E, //!< internal error detected
	F, //!< disarm requested
	X  //!< emergency stop reset
} EventType;

static const std::map<EventType, std::string> descriptions = {
	{D, "system started"},
	{L, "control center connection lost"},
	{B, "control center connection established"},
	{H, "remote control requested"},
	{M, "arm requested"},
	{N, "arm completed"},
	{S, "start requested"},
	{T, "start completed"},
	{U, "soft stop requested"},
	{Y, "scenario completed"},
	{R, "object stopped"},
	{W, "emergency stop requested"},
	{E, "internal error detected"},
	{F, "disarm requested"},
	{X, "emergency stop reset"}
};

}

static const std::map<ObjectStateID, std::string> stateNames = {
	{ISO_OBJECT_STATE_UNKNOWN, "Unknown"},
	{ISO_OBJECT_STATE_OFF, "Off"},
	{ISO_OBJECT_STATE_INIT, "Init"},
	{ISO_OBJECT_STATE_ARMED, "Armed"},
	{ISO_OBJECT_STATE_DISARMED, "Disarmed"},
	{ISO_OBJECT_STATE_RUNNING, "Running"},
	{ISO_OBJECT_STATE_POSTRUN, "NormalStop"},
	{ISO_OBJECT_STATE_REMOTE_CONTROLLED, "RemoteControlled"},
	{ISO_OBJECT_STATE_ABORTING, "EmergencyStop"},
	{ISO_OBJECT_STATE_PRE_ARMING, "PreArming"},
	{ISO_OBJECT_STATE_PRE_RUNNING, "PreRunning"}
};


/*!
 * \brief The State class is an abstract form of the states used in
 *			ISO 22133. It is not intended to be used outside of
 *			iso22133object.h
 */
class State {
friend class TestObject;
public:
	State() {}
	virtual ~State() {}

	//! Intended to be called by user, and implemented by the
	//! inheriting class
	virtual void executeBehaviour(TestObject&) = 0;

	virtual ObjectStateID getStateID() const = 0;
	std::string getName() const { return stateNames.at(getStateID()); }

	// Handles events according to ISO22133 state change description
	void handleEvent(TestObject&, const Events::EventType);
protected:
	//! Will be called on transition, indended to be
	//! overridden by the inheriting class if necessary
	virtual void onEnter(TestObject&) {std::cout << "Entering state: " << this->getName() << std::endl;}
	virtual void onExit(TestObject&) {std::cout << "Leaving state: " << this->getName() << std::endl;}

	//! When a message arrives, these methods are
	//! the 'front line' handlers
	virtual void _handleHEAB(TestObject&,HeabMessageDataType&);
	virtual void _handleTRAJ(TestObject&,traj&) { unexpectedMessageWarning("TRAJ"); }
	virtual void _handleOSEM(TestObject&,ObjectSettingsType&) { unexpectedMessageWarning("OSEM"); }
	virtual void _handleOSTM(TestObject&,ObjectCommandType&) { unexpectedMessageWarning("OSTM"); }
	virtual void _handleSTRT(TestObject&,strt&) { unexpectedMessageWarning("STRT"); }

	//! Will be called asynchronously, indended to be
	//! overridden by the inheriting class if necessary
	virtual void handleHEAB(TestObject&,HeabMessageDataType&) {}
	virtual void handleTRAJ(TestObject&,traj&) {}
	virtual void handleOSEM(TestObject&,ObjectSettingsType&) {}
	virtual void handleOSTM(TestObject&,ObjectCommandType&) {}
	virtual void handleSTRT(TestObject&,strt&) {}

	[[noreturn]]
	void unexpectedMessageError(const std::string& msgName) {
		throw std::runtime_error("Unexpected " + msgName + " in state " + this->getName());
	}
	void unexpectedMessageWarning(const std::string& msgName) {
		std::cerr << "Unexpected " + msgName + " in state " + this->getName() << std::endl;
	}
};

class Unknown : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_UNKNOWN; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject&, HeabMessageDataType&) final override { unexpectedMessageError("HEAB"); }
	void _handleTRAJ(TestObject&, traj&) final override { unexpectedMessageError("TRAJ"); }
	void _handleOSEM(TestObject&, ObjectSettingsType&) final override { unexpectedMessageError("OSEM"); }
	void _handleOSTM(TestObject&, ObjectCommandType&) final override { unexpectedMessageError("OSTM"); }
	void _handleSTRT(TestObject&, strt&) final override { unexpectedMessageError("STRT"); }
};
class Off : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_OFF; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject&, HeabMessageDataType&) final override { unexpectedMessageError("HEAB"); }
	void _handleTRAJ(TestObject&, traj&) final override { unexpectedMessageError("TRAJ"); }
	void _handleOSEM(TestObject&, ObjectSettingsType&) final override { unexpectedMessageError("OSEM"); }
	void _handleOSTM(TestObject&, ObjectCommandType&) final override { unexpectedMessageError("OSTM"); }
	void _handleSTRT(TestObject&, strt&) final override { unexpectedMessageError("STRT"); }
};
class Init : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_INIT; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject&, HeabMessageDataType&) final override { unexpectedMessageError("HEAB"); }
	void _handleTRAJ(TestObject&, traj&) final override { unexpectedMessageError("TRAJ"); }
	void _handleOSEM(TestObject&, ObjectSettingsType&) final override { unexpectedMessageError("OSEM"); }
	void _handleOSTM(TestObject&, ObjectCommandType&) final override { unexpectedMessageError("OSTM"); }
	void _handleSTRT(TestObject&, strt&) final override { unexpectedMessageError("STRT"); }
};
class Armed : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_ARMED; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleSTRT(TestObject&, strt&) final override;
	void _handleOSTM(TestObject&, ObjectCommandType&) final override;
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override { State::_handleHEAB(obj, msg); }
};
class Disarmed : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_DISARMED; }
	virtual void executeBehaviour(TestObject&) override{};
	virtual void onEnter(TestObject& obj);
	virtual void onExit(TestObject& obj);
private:
	void _handleHEAB(TestObject&, HeabMessageDataType&) final override;
	void _handleTRAJ(TestObject&, traj&) final override;
	void _handleOSEM(TestObject&, ObjectSettingsType&) final override;
	void _handleOSTM(TestObject&, ObjectCommandType&) final override;
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
};
class Running : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_RUNNING; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override;
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override { State::_handleOSTM(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};
class PostRun : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_POSTRUN; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override { State::_handleHEAB(obj, msg); }
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override { State::_handleOSTM(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};
class RemoteControlled : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_REMOTE_CONTROLLED; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override;
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override { State::_handleHEAB(obj, msg); }
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};
class Aborting : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_ABORTING; }
	virtual void executeBehaviour(TestObject&) override{};
private:
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override;
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override { State::_handleOSTM(obj, msg); }
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};
class PreArming : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_PRE_ARMING; }
	virtual void executeBehaviour(TestObject&) override{};
	virtual void onEnter(TestObject& obj) {this->handleEvent(obj, Events::N);}
private:
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override { State::_handleHEAB(obj, msg); }
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override { State::_handleOSTM(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};
class PreRunning : public State {
public:
	virtual ObjectStateID getStateID() const final override { return ISO_OBJECT_STATE_PRE_RUNNING; }
	virtual void executeBehaviour(TestObject&) override{};
	virtual void onEnter(TestObject& obj) {this->handleEvent(obj, Events::T);}
private:
	void _handleHEAB(TestObject& obj, HeabMessageDataType& msg) final override { State::_handleHEAB(obj, msg); }
	void _handleSTRT(TestObject& obj, strt& msg) final override { State::_handleSTRT(obj, msg); }
	void _handleOSTM(TestObject& obj, ObjectCommandType& msg) final override { State::_handleOSTM(obj, msg); }
	void _handleTRAJ(TestObject& obj, traj& msg) final override { State::_handleTRAJ(obj, msg); }
	void _handleOSEM(TestObject& obj, ObjectSettingsType& msg) final override { State::_handleOSEM(obj, msg); }
};

typedef struct {
	ObjectStateID source;
	Events::EventType event;
	ObjectStateID target;
} Transition;



inline bool operator< (const Transition &lhs, const Transition &rhs){
	return std::tie(lhs.source, lhs.event, lhs.target) < std::tie(rhs.source, rhs.event, rhs.target);
}

static const std::set<Transition> language = {
	{ISO_OBJECT_STATE_OFF,					Events::D,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_INIT,					Events::B,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_ARMED,				Events::E,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_ARMED,				Events::F,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_ARMED,				Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_ARMED,				Events::S,		ISO_OBJECT_STATE_PRE_RUNNING},
	{ISO_OBJECT_STATE_ARMED,				Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_DISARMED,				Events::M,		ISO_OBJECT_STATE_PRE_ARMING},
	{ISO_OBJECT_STATE_DISARMED,				Events::H,		ISO_OBJECT_STATE_REMOTE_CONTROLLED},
	{ISO_OBJECT_STATE_DISARMED,				Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_DISARMED,				Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_RUNNING,				Events::U,		ISO_OBJECT_STATE_POSTRUN},
	{ISO_OBJECT_STATE_RUNNING,				Events::Y,		ISO_OBJECT_STATE_POSTRUN},
	{ISO_OBJECT_STATE_RUNNING,				Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_RUNNING,				Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_POSTRUN,				Events::R,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_POSTRUN,				Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_POSTRUN,				Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_REMOTE_CONTROLLED,	Events::F,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_REMOTE_CONTROLLED,	Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_REMOTE_CONTROLLED,	Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_ABORTING,				Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_ABORTING,				Events::X,		ISO_OBJECT_STATE_DISARMED},
	{ISO_OBJECT_STATE_ABORTING,				Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_PRE_ARMING,			Events::N,		ISO_OBJECT_STATE_ARMED},
	{ISO_OBJECT_STATE_PRE_ARMING,			Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_PRE_ARMING,			Events::W,		ISO_OBJECT_STATE_ABORTING},
	{ISO_OBJECT_STATE_PRE_RUNNING,			Events::T,		ISO_OBJECT_STATE_RUNNING},
	{ISO_OBJECT_STATE_PRE_RUNNING,			Events::L,		ISO_OBJECT_STATE_INIT},
	{ISO_OBJECT_STATE_PRE_RUNNING,			Events::W,		ISO_OBJECT_STATE_ABORTING}
};

} // end namespace

