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

#include "../include/jsonrpc-lean/client.h"
#include "../include/jsonrpc-lean/fault.h"
#include "../include/jsonrpc-lean/jsonformathandler.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <memory>

void LogArguments() {}

template<typename Head>
void LogArguments(Head&& head) {
    std::cout << head;
}

template<typename Head, typename... Tail>
void LogArguments(Head&& head, Tail&&... tail) {
    std::cout << head << ", ";
    LogArguments(std::forward<Tail>(tail)...);
}

void LogArguments(jsonrpc::Request::Parameters& params) {
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (it != params.begin()) {
            std::cout << ", ";
        }
        std::cout << *it;
    }
}

size_t CallErrors = 0;

template<typename... T>
void LogCall(jsonrpc::Client& client, std::string method, T&&... args) {
    std::cout << method << '(';
    LogArguments(std::forward<T>(args)...);
    std::cout << "):\n>>> ";
    try {
        std::cout << client.BuildRequestData(std::move(method), std::forward<T>(args)...)->GetData();
    } catch (const jsonrpc::Fault& fault) {
        ++CallErrors;
        std::cout << "Error: " << fault.what();
    }
    std::cout << "\n";
}

template<typename... T>
void LogNotificationCall(jsonrpc::Client& client, std::string method, T&&... args) {
    std::cout << method << '(';
    LogArguments(std::forward<T>(args)...);
    std::cout << "):\n>>> ";
    try {
        std::cout << client.BuildNotificationData(std::move(method), std::forward<T>(args)...)->GetData();
    }
    catch (const jsonrpc::Fault& fault) {
        ++CallErrors;
        std::cout << "Error: " << fault.what();
    }
    std::cout << "\n\n";
}

int main(int argc, char** argv) {

	const char addResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":5}";
    const char concatResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"Hello, World!\"}";
    const char addArrayResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":2147484647}";
    const char toStructResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":4,\"result\":{\"0\":12,\"1\":\"foobar\",\"2\":[12,\"foobar\"]}}";	
	
    std::unique_ptr<jsonrpc::FormatHandler> formatHandler(new jsonrpc::JsonFormatHandler());

    try {
        jsonrpc::Client client(*formatHandler);

        LogCall(client, "add", 3, 2);
		jsonrpc::Response parsedResponse = client.ParseResponse(addResponse);
        std::cout << "Parsed response: " << parsedResponse.GetResult().AsInteger32() << std::endl << std::endl;
		
        LogCall(client, "concat", "Hello, ", "World!");
		parsedResponse = client.ParseResponse(concatResponse);
        std::cout << "Parsed response: " << parsedResponse.GetResult().AsString() << std::endl << std::endl;

        jsonrpc::Request::Parameters params;
        {
            jsonrpc::Value::Array a;
            a.emplace_back(1000);
            a.emplace_back(std::numeric_limits<int32_t>::max());
            params.push_back(std::move(a));
        }
        LogCall(client, "add_array", params);
		parsedResponse = client.ParseResponse(addArrayResponse);
        std::cout << "Parsed response: " << parsedResponse.GetResult().AsInteger64() << std::endl << std::endl;

        LogCall(client, "to_binary", "Hello World!"); // once the result here is parsed, the underlying AsString can be just an array of bytes, not necessarily printable characters
        LogCall(client, "from_binary", jsonrpc::Value("Hi!", true)); // "Hi!" can be an array of bytes, not necessarily printable characters
		LogNotificationCall(client, "print_notification", "This is just a notification, no response expected!");

        params.clear();
        {
            jsonrpc::Value::Array a;
            a.emplace_back(12);
            a.emplace_back("foobar");
            a.emplace_back(a);
            params.push_back(std::move(a));
        }
        LogCall(client, "to_struct", params);
		parsedResponse = client.ParseResponse(toStructResponse);
        auto structValue = parsedResponse.GetResult().AsStruct();

        std::cout << "Parsed response: " << std::endl;
        std::cout << "   0 : " << structValue["0"].AsInteger32() << std::endl;
        std::cout << "   1 : " << structValue["1"].AsString() << std::endl;
        std::cout << "   2 : [" << structValue["2"].AsArray()[0] << ", " << structValue["2"].AsArray()[1] << "]" << std::endl;
        std::cout << std::endl;

        params.clear();
        {
            jsonrpc::Value::Array calls;
            {
                jsonrpc::Value::Struct call;
                call["method"] = jsonrpc::Value("add");
                {
                    jsonrpc::Value::Array params;
                    params.emplace_back(23);
                    params.emplace_back(19);
                    call["params"] = std::move(params);
                }
                calls.emplace_back(std::move(call));
            }
            {
                jsonrpc::Value::Struct call;
                call["method"] = jsonrpc::Value("does.NotExist");
                calls.emplace_back(std::move(call));
            }
            {
                jsonrpc::Value::Struct call;
                call["method"] = jsonrpc::Value("concat");
                {
                    jsonrpc::Value::Array params;
                    params.emplace_back("Hello ");
                    params.emplace_back("multicall!");
                    call["params"] = std::move(params);
                }
                calls.emplace_back(std::move(call));
            }
            params.emplace_back(std::move(calls));
        }

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    if (CallErrors > 0) {
        std::cerr << "Error: " << CallErrors << " call(s) failed\n";
        return 1;
    }

    return 0;
}
