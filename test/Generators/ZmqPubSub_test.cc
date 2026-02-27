#define BOOST_TEST_MODULE ZmqPubSub_test
#include <boost/test/unit_test.hpp>

#include "TRACE/tracemf.h"
#define TRACE_NAME "ZmqPubSub_test"

#include "zmq.hpp"

BOOST_AUTO_TEST_SUITE(ZmqPubSub_test)

BOOST_AUTO_TEST_CASE(ZmqContext)
{
	zmq::context_t context;
	context.close();
}

BOOST_AUTO_TEST_CASE(Socket)
{
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::pub);
	socket.close();
	context.close();
}

BOOST_AUTO_TEST_CASE(Subscribe)
{
	zmq::context_t context;
	zmq::socket_t socket(context, zmq::socket_type::sub);
	socket.set(zmq::sockopt::subscribe, "test");
	socket.close();
	context.close();
}

BOOST_AUTO_TEST_CASE(Publish)
{
	zmq::context_t context;
	zmq::socket_t pub_socket(context, zmq::socket_type::pub);
	zmq::socket_t sub_socket(context, zmq::socket_type::sub);

	pub_socket.bind("inproc://test");
	sub_socket.connect("inproc://test");

	sub_socket.set(zmq::sockopt::subscribe, "test");
	std::string message = "test message";
	pub_socket.send(zmq::buffer("test"), zmq::send_flags::sndmore);
	pub_socket.send(zmq::buffer(message), zmq::send_flags::none);

	std::string received_topic;
	received_topic.resize(4);
	std::string received_message;
	received_message.resize(message.size());

	auto hdr_ret = sub_socket.recv(zmq::buffer(received_topic), zmq::recv_flags::none);
	BOOST_REQUIRE(hdr_ret.has_value());
	BOOST_REQUIRE_EQUAL(hdr_ret.value().size, 4);

	auto ret = sub_socket.recv(zmq::buffer(received_message), zmq::recv_flags::none);
	BOOST_REQUIRE(ret.has_value());
	BOOST_REQUIRE_EQUAL(ret.value().size, message.size());
	BOOST_REQUIRE_EQUAL(message, received_message);

	sub_socket.close();
	pub_socket.close();
	context.close();
}

BOOST_AUTO_TEST_SUITE_END()
