// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions Copyright (C) 2015 Adriano Maia <tony@stark.im>
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifdef _WIN32
#include "stdafx.h"
#endif

#include "../include/jsonrpc-lean/jsonformathandler.h"
#include "../include/jsonrpc-lean/formathandler.h"
#include "../include/jsonrpc-lean/server.h"

#include <iostream>
#include <numeric>
#include <string>
#include <memory>
#include <stdint.h>
#include <system_error>

#define BOOST_THREAD_VERSION 4
#include <boost/thread/future.hpp>

class Math {
public:
	int Add(int a, int b) const {
		return a + b;
	}
	
	boost::future<int> AsyncAddInt(int a, int b) const {
		return boost::async([](auto a, auto b){
			return a + b;
		}, a, b);
	}
	
	boost::future<int> AsyncAdd(int a, int b) const {
		return boost::async(&Math::Add, this, a, b);
	}
	
	boost::future<int64_t> AsyncAddSizeT(int a, int b) const {
		return boost::make_ready_future((int64_t)(size_t)10);
	}

	int64_t AddArray(const jsonrpc::Value::Array& a) {
		return std::accumulate(a.begin(), a.end(), int64_t(0),
			[](const int64_t& a, const jsonrpc::Value& b) { return a + b.AsInteger32(); });
	};
};

std::string Concat(const std::string& a, const std::string& b) {
	return a + b;
}

boost::future<std::string> AsyncConcat(const std::string& a, const std::string& b) {
	boost::async([](auto a, auto b){ return a + b; }, a, b);
}

jsonrpc::Value ToBinary(const std::string& s) {
	return jsonrpc::Value(s, true);
}

std::string FromBinary(const jsonrpc::Value& b) {
	return{ b.AsBinary().begin(), b.AsBinary().end() };
}

jsonrpc::Value::Struct ToStruct(const jsonrpc::Value::Array& a) {
	jsonrpc::Value::Struct s;
	for (size_t i = 0; i < a.size(); ++i) {
		s[std::to_string(i)] = jsonrpc::Value(a[i]);
	}
	return s;
}

void PrintNotification(const std::string& a) {
    std::cout << "notification: " << a << std::endl;
}

void RunServer() {
	Math math;
	jsonrpc::Server server;
	
	jsonrpc::JsonFormatHandler jsonFormatHandler;
	server.RegisterFormatHandler(jsonFormatHandler);

	auto& dispatcher = server.GetDispatcher();
	// if it is a member method, you must use this 3 parameter version, passing an instance of an object that implements it
	dispatcher.AddMethod("add", &Math::Add, math);
	dispatcher.AddMethod("add_array", &Math::AddArray, math); 
	
	// if it is just a regular function (non-member or static), you can you the 2 parameter AddMethod
	dispatcher.AddMethod("concat", &Concat);
	dispatcher.AddMethod("to_binary", &ToBinary);
	dispatcher.AddMethod("from_binary", &FromBinary);
	dispatcher.AddMethod("to_struct", &ToStruct);
	dispatcher.AddMethod("print_notification", &PrintNotification);
	dispatcher.AddMethod("async_add", &Math::AsyncAdd, math);
	dispatcher.AddMethod("async_add_size_t", &Math::AsyncAddSizeT, math);
	dispatcher.AddMethod("async_add_int", &Math::AsyncAddInt, math);
	dispatcher.AddMethod("async_concat", AsyncConcat);
	
	std::function<boost::future<std::string>(std::string)> sReverse = [](std::string in) -> boost::future<std::string> { 
		std::string res;
		return boost::make_ready_future(res.assign(in.rbegin(), in.rend())); 
	};
	
	dispatcher.AddAsyncLambda("async_reverse", sReverse);
	
	// EACCESS is 13 (see http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html)
	dispatcher.AddMethod("fail", [](){ 
		throw std::system_error(std::make_error_code(std::errc::permission_denied), "specific error message"); 
	});
	
	std::function<boost::future<int>()> asyncFail = []() -> boost::future<int> { 
		return boost::make_exceptional_future<int>(std::system_error(
			std::make_error_code(std::errc::permission_denied), 
			"specific error message")
		); 
	};
	dispatcher.AddAsyncLambda("asyncFail", asyncFail);

	std::function<boost::future<int>(int)> asyncThrow = [](int x) -> boost::future<int> { 
		throw std::out_of_range("Exception description from inside an async lambda method.");
		return boost::make_ready_future<int>( x * x );
	};
	dispatcher.AddAsyncLambda("asyncThrow", asyncThrow);
	
	dispatcher.GetMethod("add")
		.SetHelpText("Add two integers")
		.AddSignature(jsonrpc::Value::Type::INTEGER_32, jsonrpc::Value::Type::INTEGER_32, jsonrpc::Value::Type::INTEGER_32);

	//bool run = true;
	//dispatcher.AddMethod("exit", [&]() { run = false; }).SetHidden();
	
	const char addRequest[] = R"({"jsonrpc":"2.0","method":"add","id":0,"params":[3,2]})";
	const char concatRequest[] = R"({"jsonrpc":"2.0","method":"concat","id":1,"params":["Hello, ","33"]})";
	const char addArrayRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add_array\",\"id\":2,\"params\":[[1000,2147483647]]}";
	const char toBinaryRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_binary\",\"id\":3,\"params\":[\"Hello World!\"]}";
	const char toStructRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_struct\",\"id\":4,\"params\":[[12,\"foobar\",[12,\"foobar\"]]]}";
	const char printNotificationRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"print_notification\",\"params\":[\"This is just a notification, no response expected!\"]}";
	const char addAsyncRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"async_add\",\"id\":10,\"params\":[30,20]}";
	const char addIntAsyncRequest[] = R"({"jsonrpc":"2.0","method":"async_add_int","id":11,"params":[300,200]})";
	const char addIntAsyncToSizeTRequest[] = R"({"jsonrpc":"2.0","method":"async_add_size_t","id":14,"params":[300,200]})";
	const char asyncConcatRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"concat\",\"id\":12,\"params\":[\"Hello, \",\"World!\"]}";
	const char asyncReverseRequest[] = R"({"jsonrpc":"2.0","method":"async_reverse","id":13,"params":["xyz"]})";
	const char failRequest[] = R"({"jsonrpc":"2.0","method":"fail","id":20})";
	const char asyncFailRequest[] = R"({"jsonrpc":"2.0","method":"asyncFail","id":21})";
	const char asyncThrowRequest[] = R"({"jsonrpc":"2.0","method":"asyncThrow","id":22, "params":[5]})";

	std::shared_ptr<jsonrpc::FormattedData> outputFormatedData;
	
    std::cout << "request: " << addRequest << std::endl;
    outputFormatedData = server.HandleRequest(addRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;
	 
	 std::cout << "test async wrapper around sync request: " << addRequest << std::endl;
    server.asyncHandleRequest(addRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr)
	{
		 std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
   

    outputFormatedData.reset();
    std::cout << "request: " << concatRequest << std::endl;
    outputFormatedData = server.HandleRequest(concatRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;

    outputFormatedData.reset();
    std::cout << "request: " << addArrayRequest << std::endl;
    outputFormatedData = server.HandleRequest(addArrayRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;

    outputFormatedData.reset();
    std::cout << "request: " << toBinaryRequest << std::endl;
    outputFormatedData = server.HandleRequest(toBinaryRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;

    outputFormatedData.reset();
    std::cout << "request: " << toStructRequest << std::endl;
    outputFormatedData = server.HandleRequest(toStructRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;

    outputFormatedData.reset();
    std::cout << "request: " << printNotificationRequest << std::endl;
    outputFormatedData = server.HandleRequest(printNotificationRequest);
    std::cout << "response size: " << outputFormatedData->GetSize() << std::endl;
	 
	 outputFormatedData.reset();
    std::cout << "request: " << failRequest << std::endl;
    outputFormatedData = server.HandleRequest(failRequest);
    std::cout << "response: " << outputFormatedData->GetData() << std::endl;
	 
	 std::cout << "request: " << addAsyncRequest << std::endl;
    server.asyncHandleRequest(addAsyncRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
		 
	 std::cout << "request: " << addIntAsyncRequest << std::endl;
    server.asyncHandleRequest(addIntAsyncRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	
	 std::cout << "request: " << asyncConcatRequest << std::endl;
    server.asyncHandleRequest(asyncConcatRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	
	std::cout << "request: " << asyncReverseRequest << std::endl;
    server.asyncHandleRequest(asyncReverseRequest)
	.then([](auto futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	
	 std::cout << "request: " << addIntAsyncToSizeTRequest << std::endl;
    server.asyncHandleRequest(addIntAsyncToSizeTRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	
	std::cout << "request: " << asyncFailRequest << std::endl;
   server.asyncHandleRequest(asyncFailRequest)
	.then([](auto futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	
	std::cout << "request: " << asyncThrowRequest << std::endl;
   server.asyncHandleRequest(asyncThrowRequest)
	.then([](auto futureDataPtr){
		std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl;
	});
	 
	 boost::this_thread::sleep_for(boost::chrono::seconds(2));
}

int main() {
	try {
		RunServer();
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}

	return 0;
}
