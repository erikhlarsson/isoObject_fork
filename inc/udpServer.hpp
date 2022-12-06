#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/system_error.hpp>
#include <iostream>
#include <vector>

using namespace boost::asio;
using ip::udp;

/**
 * @brief UDP server based on Boost::asio. Contains the basic functions needed for ISO22133 UDP communication
 *
 */
class UdpServer {
   public:
	UdpServer(uint32_t port) : socket(context, udp::endpoint(udp::v4(), port)) {
		setBufferSize(defaultBufferSize);
	};

	void setBufferSize(size_t size) { dataBuffer.resize(size); };

	void disconnect() {
		// This call may throw but shutdown is still successful.
		try {
			socket.shutdown(socket_base::shutdown_receive);
		} catch (const boost::system::system_error& e) {} 
	};

	std::vector<char> receive() {
		try {
			auto nBytes = socket.receive_from(buffer(dataBuffer), senderEndpoint);
			setBufferSize(nBytes);
			std::vector<char> result(dataBuffer);
			setBufferSize(defaultBufferSize);
			return result;
		} catch (const std::exception& e) {
			std::cerr << "Exception in receive UDP: " << e.what() << '\n';
			throw e;
		}
	};

	size_t send(std::vector<char> data, size_t nbytes) {
		try {
			std::vector<char> sendBuffer(data);
			sendBuffer.resize(nbytes);
			auto bytesSent = socket.send_to(buffer(sendBuffer), senderEndpoint);
			return bytesSent;
		} catch (const std::exception& e) {
			std::cerr << "Exception in send UDP: " << e.what() << '\n';
			throw e;
		}
	};

   private:
	std::vector<char> dataBuffer;
	size_t defaultBufferSize = 4096;

	io_context context;
	udp::socket socket;
	udp::endpoint senderEndpoint;
};