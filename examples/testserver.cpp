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

#include "stdafx.h"

#include "../include/jsonrpc-lean/jsonformathandler.h"
#include "../include/jsonrpc-lean/formathandler.h"
#include "../include/jsonrpc-lean/server.h"

#include <iostream>
#include <numeric>
#include <string>
#include <stdint.h>

class Math {
public:
	int Add(int a, int b) {
		return a + b;
	}

	int64_t AddArray(const jsonrpc::Value::Array& a) {
		return std::accumulate(a.begin(), a.end(), int64_t(0),
			[](const int64_t& a, const jsonrpc::Value& b) { return a + b.AsInteger32(); });
	};
};

std::string Concat(const std::string& a, const std::string& b) {
	return a + b;
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

	dispatcher.GetMethod("add")
		.SetHelpText("Add two integers")
		.AddSignature(jsonrpc::Value::Type::INTEGER_32, jsonrpc::Value::Type::INTEGER_32, jsonrpc::Value::Type::INTEGER_32);

	bool run = true;
	//dispatcher.AddMethod("exit", [&]() { run = false; }).SetHidden();
	
	const char addRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add\",\"id\":0,\"params\":[3,2]}";
	const char concatRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"concat\",\"id\":1,\"params\":[\"Hello, \",\"World!\"]}";
	const char addArrayRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add_array\",\"id\":2,\"params\":[[1000,2147483647]]}";
	const char toBinaryRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_binary\",\"id\":3,\"params\":[\"Hello World!\"]}";
	const char toStructRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_struct\",\"id\":5,\"params\":[[12,\"foobar\",[12,\"foobar\"]]]}";

	std::string outputBuffer;
	std::cout << "request: " << addRequest << std::endl;
	server.HandleRequest(addRequest, outputBuffer);
	std::cout << "response: " << outputBuffer << std::endl;

	outputBuffer = "";
	std::cout << "request: " << concatRequest << std::endl;
	server.HandleRequest(concatRequest, outputBuffer);
	std::cout << "response: " << outputBuffer << std::endl;

	outputBuffer = "";
	std::cout << "request: " << addArrayRequest << std::endl;
	server.HandleRequest(addArrayRequest, outputBuffer);
	std::cout << "response: " << outputBuffer << std::endl;

	outputBuffer = "";
	std::cout << "request: " << toBinaryRequest << std::endl;
	server.HandleRequest(toBinaryRequest, outputBuffer);
	std::cout << "response: " << outputBuffer << std::endl;

	outputBuffer = "";
	std::cout << "request: " << toStructRequest << std::endl;
	server.HandleRequest(toStructRequest, outputBuffer);
	std::cout << "response: " << outputBuffer << std::endl;
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
