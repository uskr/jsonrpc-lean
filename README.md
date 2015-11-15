
# jsonrpc-lean

An [LGPL](http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) licensed, client/server, transport-agnostic, JSON-RPC library for C++11.

The initial jsonrpc-lean implementation is derived from xsonrpc (https://github.com/erijo/xsonrpc) by Erik Johansson (https://github.com/erijo/). Much thanks to him for doing all the heavy work.

The main idea behind branching xsonrpc and building a new project is to have a simple, transport-agnostic, header-only implementation, with as little external dependencies as possible. 

Anyone can use this lib by just adding the include folder to their include path and they are good to go.

Currently, the only dependency is rapidjson (https://github.com/miloyip/rapidjson), which is also a header-only implementation. Add rapidjson to your include path, and done.

Another advantage of removing the dependencies is that now it is easy to compile and use on most platforms that support c++11, without much work.

## Examples

A simple server that process JSON-RPC requests:

```C++
#include "jsonrpc-lean/server.h"

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

int main() {
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
	dispatcher.AddMethod("to_struct", &ToStruct);
	dispatcher.AddMethod("print_notification", &PrintNotification);

	// on a real world, these requests come from your own transport implementation (sockets, http, ipc, named-pipes, etc)
	const char addRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add\",\"id\":0,\"params\":[3,2]}";
	const char concatRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"concat\",\"id\":1,\"params\":[\"Hello, \",\"World!\"]}";
	const char addArrayRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add_array\",\"id\":2,\"params\":[[1000,2147483647]]}";
	const char toStructRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_struct\",\"id\":5,\"params\":[[12,\"foobar\",[12,\"foobar\"]]]}";
	const char printNotificationRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"print_notification\",\"params\":[\"This is just a notification, no response expected!\"]}";

	std::shared_ptr<jsonrpc::FormattedData> outputFormattedData;
    std::cout << "request: " << addRequest << std::endl;
    outputFormattedData = server.HandleRequest(addRequest);
    std::cout << "response: " << outputFormattedData->GetData() << std::endl;

    outputFormattedData.reset();
    std::cout << "request: " << concatRequest << std::endl;
    outputFormattedData = server.HandleRequest(concatRequest);
    std::cout << "response: " << outputFormattedData->GetData() << std::endl;

    outputFormattedData.reset();
    std::cout << "request: " << addArrayRequest << std::endl;
    outputFormattedData = server.HandleRequest(addArrayRequest);
    std::cout << "response: " << outputFormattedData->GetData() << std::endl;

    outputFormattedData.reset();
    std::cout << "request: " << toStructRequest << std::endl;
    outputFormattedData = server.HandleRequest(toStructRequest);
    std::cout << "response: " << outputFormattedData->GetData() << std::endl;
	
	outputFormatedData.reset();
    std::cout << "request: " << printNotificationRequest << std::endl;
    outputFormatedData = server.HandleRequest(printNotificationRequest);
    std::cout << "response size: " << outputFormatedData->GetSize() << std::endl;

	return 0;
}
```

A client capable of generating requests for the server above could look like this:

```C++
#include "jsonrpc-lean/client.h"

int main(int argc, char** argv) {

	const char addResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":5}";
    const char concatResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"Hello, World!\"}";
    const char addArrayResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":2147484647}";
    const char toStructResponse[] = "{\"jsonrpc\":\"2.0\",\"id\":4,\"result\":{\"0\":12,\"1\":\"foobar\",\"2\":[12,\"foobar\"]}}";

    std::unique_ptr<jsonrpc::FormatHandler> formatHandler(new jsonrpc::JsonFormatHandler());
	jsonrpc::Client client(*formatHandler);

	std::shared_ptr<FormattedData> jsonRequest = client.BuildRequestData("add", 3, 2);
	std::cout << jsonRequest->GetData(); // this will output the json-rpc request string
	jsonrpc::Response parsedResponse = client.ParseResponse(addResponse);
    std::cout << "Parsed response: " << parsedResponse.GetResult().AsInteger32() << std::endl << std::endl;
	
	jsonRequest.reset(client.BuildRequestData("concat", "Hello, ", "World!"));
	std::cout << jsonRequest->GetData();
	parsedResponse = client.ParseResponse(concatResponse);
        std::cout << "Parsed response: " << parsedResponse.GetResult().AsString() << std::endl << std::endl;

	jsonrpc::Request::Parameters params;
	{
		jsonrpc::Value::Array a;
		a.emplace_back(1000);
		a.emplace_back(std::numeric_limits<int32_t>::max());
		params.push_back(std::move(a));
	}
	jsonRequest.reset(client.BuildRequestData("add_array", params));
	std::cout << jsonRequest->GetData(); 
	parsedResponse = client.ParseResponse(addArrayResponse);
    std::cout << "Parsed response: " << parsedResponse.GetResult().AsInteger64() << std::endl << std::endl;

	params.clear();
	{
		jsonrpc::Value::Array a;
		a.emplace_back(12);
		a.emplace_back("foobar");
		a.emplace_back(a);
		params.push_back(std::move(a));
	}
	jsonRequest.reset(client.BuildRequestData("to_struct", params));
	std::cout << jsonRequest->GetData(); 
	parsedResponse = client.ParseResponse(toStructResponse);
	auto structValue = parsedResponse.GetResult().AsStruct();
	std::cout << "Parsed response: " << std::endl;
	std::cout << "   0 : " << structValue["0"].AsInteger32() << std::endl;
	std::cout << "   1 : " << structValue["1"].AsString() << std::endl;
	std::cout << "   2 : [" << structValue["2"].AsArray()[0] << ", " << structValue["2"].AsArray()[1] << "]" << std::endl;
	
	jsonRequest.reset(client.BuildNotificationData("print_notification", "This is just a notification, no response expected!"));
	std::cout << jsonRequest->GetData();

    return 0;
}
```

## Usage Requirements

To use jsonrpc-lean on your project, all you need is:

* A C++11 capable compiler (GCC 5.0+ (Linux), XCode/Clang (OSX 10.7+), MSVC 14.0+ (Visual Studio 2015))
* [rapidjson](https://github.com/miloyip/rapidjson) (Only need the include folder on your include path, don't worry about compiling it)

