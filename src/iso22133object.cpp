#include <chrono>
#include <thread>

#include "iso22133object.hpp"
#include "iso22133state.hpp"
#include "iso22133.h"

#define TCP_BUFFER_SIZE 1024
#define UDP_BUFFER_SIZE 1024

//TODO: get this from maestroTime.h in the future
namespace std::chrono {
    using quartermilliseconds = std::chrono::duration<int64_t, std::ratio<1,4000>>;
    using weeks = std::chrono::duration<uint16_t, std::ratio<7*24*60*60,1>>;

    template<typename Duration>
    struct timeval to_timeval(Duration&& d) {
        std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        struct timeval tv;
        tv.tv_sec  = sec.count();
        tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(d - sec).count();
        return tv;
    }

    template<typename Duration>
    void from_timeval(struct timeval & tv, Duration& d) {
        // TODO
        //const auto sec = std::chrono::seconds(tv.tv_sec);
        //const auto usec = std::chrono::microseconds(tv.tv_usec);
        //d = sec + usec;
    }
}

namespace ISO22133 {
TestObject::TestObject(const std::string& listenIP)
	: name("myTestObject"),
	  trajDecoder(),
	  ctrlPort(ISO_22133_DEFAULT_OBJECT_TCP_PORT)
{	

	CartesianPosition initPos;
	SpeedType initSpd;
	AccelerationType initAcc;
	std::cout << "Listen IP: " << listenIP << std::endl;
	localIP = listenIP;
	initPos.isHeadingValid = false;
	initPos.isPositionValid = false;
	initSpd.isLateralValid = false;
	initSpd.isLongitudinalValid = false;
	initAcc.isLateralValid = false;
	initAcc.isLongitudinalValid = false;
	this->setPosition(initPos);
	this->setSpeed(initSpd);
	this->setAcceleration(initAcc);
	this->state = this->createInit();
	this->startHandleTCP();
	this->startHEABCheck();
	this->stateChangeSig.connect(&TestObject::onStateChange, this);
	this->osemSig.connect(&TestObject::onOSEM, this);
	this->heabSig.connect(&TestObject::onHEAB, this);
	this->ostmSig.connect(&TestObject::onOSTM, this);
	this->trajSig.connect(&TestObject::onTRAJ, this);
	this->strtSig.connect(&TestObject::onSTRT, this);
	this->heabTimeout.connect(&TestObject::onHeabTimeout, this);
	
}

TestObject::~TestObject() {
	on = false;
	try {
		heabMonrThread.join();
	} catch (std::system_error) {}
	try {
		tcpReceiveThread.join(); // This blocks forever when the receive thread has not finished accept()
	} catch (std::system_error) {}
	try {
		heabTimeoutThread.join();
	} catch (std::system_error) {}
}; 


void TestObject::disconnect() {
	std::cout << "Disconnecting" << std::endl;
	try {
		ctrlPort.disconnect(); // Close TCP socket
	} catch(boost::system::system_error& e) {
		std::cerr << "TCP socket close error: " << e.what() << '\n';
	} catch(const std::exception& e) {
		std::cerr << "TCP socket close error: " << e.what() << '\n';
	}
	
	processPort.close();
	try {
		heabMonrThread.join();
	} catch (std::system_error& e) {
		std::cerr << "disconnect error: " << e.what() << std::endl;	
	}
	awaitingFirstHeab = true;
}

void TestObject::receiveTCP() {
	std::cout << "Started TCP thread." << std::endl;
	const unsigned int maxAwaitAttempts = 3;
	unsigned int remainingAwaitAttempts = maxAwaitAttempts;
	while(this->on) {
		std::cout << "\nAwaiting TCP connection from ATOS..." << std::endl;
		try {
			ctrlPort.acceptConnection();
		} catch (boost::system::system_error& e) {
			std::cout << "TCP accept failed: " << e.what() << std::endl;
			throw e;
		}
		std::cout << "TCP connection to ATOS established." << std::endl;
		state->handleEvent(*this, ISO22133::Events::B);
		try {
			while (true) {
				
				auto data = ctrlPort.receive();
				int nBytesHandled = 0;
				do {
					try {
						nBytesHandled = handleMessage(data);
					}
					catch (const std::exception& e) {
						std::cerr << e.what() << std::endl;
						break;
					}
					data.erase(data.begin(), data.begin() + nBytesHandled);
				} while(data.size() > 0);
			}
		} catch (boost::system::system_error&) {
			std::cerr << "Connection to ATOS lost" << std::endl;
			disconnect();
			state->handleEvent(*this, ISO22133::Events::L);
		}
	}
	std::cout << "Exiting TCP thread." << std::endl;
}

void TestObject::sendMONR(const BasicSocket::HostInfo& toWhere, bool debug) {
	std::vector<char> buffer(UDP_BUFFER_SIZE);
	struct timeval time;
	auto nanos = std::chrono::system_clock::now().time_since_epoch().count();
	time.tv_sec = nanos/1e9;
	time.tv_usec = nanos/1e3 - time.tv_sec*1e6;

	auto result = encodeMONRMessage(&time, this->position, this->speed, this->acceleration,
									this->driveDirection, this->state->getStateID(), this->readyToArm,
									this->errorState, buffer.data(), buffer.size(),debug);
	if (result < 0) {
		std::cout << "Failed to encode MONR data" << std::endl;
	}
	else {
		processPort.sendto({buffer, toWhere}, static_cast<size_t>(result));
	}
}

void TestObject::heabMonrLoop() {
	std::cout << "Started MONR/HEAB communication thread." << std::endl;
	processPort.bind({localIP, ISO_22133_OBJECT_UDP_PORT});
	awaitingFirstHeab = true;
	std::cout << "Awaiting UDP data from ATOS..." << std::endl;
	try {
		while (this->on) {
			auto [data, host] = processPort.recvfrom();
			if (awaitingFirstHeab) {
				std::cout << "Received UDP data from ATOS server at " << host.address << std::endl;
			}
			int nBytesHandled = 0;
			do {
				try {
					nBytesHandled = handleMessage(data);
					sendMONR(host);
				}
				catch (const std::exception& e) {
					std::cerr << e.what() << std::endl;
					break;
				}
				data.erase(data.begin(), data.begin() + nBytesHandled);
			} while(data.size() > 0);
		}
	} catch (SocketErrors::DisconnectedError&) {}
	std::cout << "Exiting MONR/HEAB communication thread." << std::endl;
}

void TestObject::checkHeabTimeout() {
	using namespace std::chrono;
	std::scoped_lock lock(heabMutex);
	// Check time difference of received HEAB and last HEAB
	auto timeSinceHeab = steady_clock::now() - lastHeabTime;
	if (!awaitingFirstHeab && timeSinceHeab > heartbeatTimeout) {
		std::cerr << "Heartbeat timeout: " << duration_cast<milliseconds>(timeSinceHeab).count()
				  << " ms since last heartbeat exceeds limit of " << heartbeatTimeout.count() << " ms."
				  << std::endl;
		heabTimeout();
	}
}

void TestObject::checkHeabLoop() {
	std::cout << "Started HEAB timeout thread." << std::endl;
	using namespace std::chrono;
	while (this->on) {
		auto t = std::chrono::steady_clock::now();
		checkHeabTimeout();
		// Don't lock the mutex all the time
		std::this_thread::sleep_until(t+expectedHeartbeatPeriod);
	}
	std::cout << "Exiting HEAB timeout thread." << std::endl;
}

void TestObject::onHeabTimeout() {
	disconnect();
	this->state->handleEvent(*this, Events::L);
}

int TestObject::handleMessage(std::vector<char>& dataBuffer) {
	std::lock_guard<std::mutex> lock(this->recvMutex); // Both TCP and UDP threads end up in here
	int bytesHandled = 0;
	int debug = 0;
	struct timeval currentTime;

    currentTime = std::chrono::to_timeval(std::chrono::system_clock::now().time_since_epoch());

	ISOMessageID msgType = getISOMessageType(dataBuffer.data(), dataBuffer.size(), 0);
	// Ugly check here since we don't know if it is UDP or the rest of TRAJ
	if (msgType == MESSAGE_ID_INVALID && this->trajDecoder.ExpectingTrajPoints()) {
		msgType = MESSAGE_ID_TRAJ;
	}

	switch (msgType) {
	case MESSAGE_ID_TRAJ:
		try {
			bytesHandled = this->trajDecoder.DecodeTRAJ(dataBuffer);
		}
		catch(const std::exception& e) {
			throw e;
		}
		if (bytesHandled < 0) {
			throw std::invalid_argument("Error decoding TRAJ");
		};
		if (!this->trajDecoder.ExpectingTrajPoints()) {
			this->state->handleTRAJ(*this);
		}
		break;
	case MESSAGE_ID_OSEM:
		ObjectSettingsType OSEMstruct;
		bytesHandled = decodeOSEMMessage(
					&OSEMstruct,
					dataBuffer.data(),
					dataBuffer.size(),
					nullptr,
					debug);
		if (bytesHandled < 0) {
			throw std::invalid_argument("Error decoding OSEM");
		}
		std::cout << "Received OSEM " << std::endl;
		this->state->handleOSEM(*this, OSEMstruct);
		break;

	case MESSAGE_ID_OSTM:
		ObjectCommandType OSTMdata;
		bytesHandled = decodeOSTMMessage(
					dataBuffer.data(),
					dataBuffer.size(),
					&OSTMdata,
					debug);
		if (bytesHandled < 0) {
			throw std::invalid_argument("Error decoding OSTM");
		}
		this->state->handleOSTM(*this, OSTMdata);
		break;

	case MESSAGE_ID_STRT:
		StartMessageType STRTdata;
		bytesHandled = decodeSTRTMessage(
					dataBuffer.data(),
					dataBuffer.size(),
					&currentTime,
					&STRTdata,
					debug);
		if (bytesHandled < 0) {
			throw std::invalid_argument("Error decoding STRT");
		}
		this->state->handleSTRT(*this, STRTdata);
		break;

	case MESSAGE_ID_HEAB:
		HeabMessageDataType HEABdata;

		bytesHandled = decodeHEABMessage(
					dataBuffer.data(),
					dataBuffer.size(),
					currentTime,
					&HEABdata,
					debug);
		if (bytesHandled < 0) {
			throw std::invalid_argument("Error decoding HEAB");
		}
		this->handleHEAB(HEABdata);
		break;
	default:
		bytesHandled = handleVendorSpecificMessage(msgType, dataBuffer);
		if (bytesHandled < 0) {
			throw std::invalid_argument(std::string("Unable to decode ISO-22133 message with MsgID ")
                                  + std::to_string(msgType));
		}
		bytesHandled = static_cast<int>(dataBuffer.size());
		break;
	}
	return bytesHandled;
}

/**
 * @brief Generates state changes based on control center status
 * @param heab struct HeabMessageDataType
 */
void TestObject::handleHEAB(HeabMessageDataType& heab) {
	using namespace std::chrono;
	// Order matters here, below may change state
	// causing the signal to not be triggered if placed
	// after the handleEvent() calls
	heabSig(heab);

	// Check network delay: difference between
	// timestamp in HEAB and local time
	auto heabTime = seconds(heab.dataTimestamp.tv_sec)
			+ microseconds(heab.dataTimestamp.tv_usec);
	auto networkDelay = system_clock::now().time_since_epoch() - heabTime;
	if (networkDelay > maxSafeNetworkDelay) {
		std::cerr << "Network delay of " << duration_cast<milliseconds>(networkDelay).count()
				  << " ms exceeds safe limit of " << maxSafeNetworkDelay.count() << " ms."
				  << std::endl;
		// TODO do something
	}
	checkHeabTimeout();
	std::scoped_lock lock(heabMutex);
	lastHeabTime = steady_clock::now();
	awaitingFirstHeab = false;

	switch (heab.controlCenterStatus) {
	case CONTROL_CENTER_STATUS_NORMAL_STOP:
		this->state->handleEvent(*this, ISO22133::Events::U);
		break;
	case CONTROL_CENTER_STATUS_ABORT:
		this->state->handleEvent(*this, ISO22133::Events::W);
		break;
	case CONTROL_CENTER_STATUS_TEST_DONE:
		this->state->handleEvent(*this, ISO22133::Events::Y);
		break;
	default:
		break;
	}
	ccStatus = heab.controlCenterStatus;
	return;
}

} //namespace ISO22133
