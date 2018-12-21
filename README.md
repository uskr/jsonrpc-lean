
# jsonrpc-lean

An [LGPL](https://www.gnu.org/licenses/lgpl-3.0.en.html) licensed, client/server, transport-agnostic, JSON-RPC library for C++11.

The initial jsonrpc-lean implementation is derived from xsonrpc (https://github.com/erijo/xsonrpc) by Erik Johansson (https://github.com/erijo/). Much thanks to him for doing all the heavy work.

The main idea behind branching xsonrpc and building a new project is to have a simple, transport-agnostic, header-only implementation, with as little external dependencies as possible. 

Anyone can use this lib by just adding the include folder to their include path and they are good to go.

Currently, the only dependency is rapidjson (https://github.com/miloyip/rapidjson), which is also a header-only implementation. Add rapidjson to your include path, and done.

Another advantage of removing the dependencies is that now it is easy to compile and use on most platforms that support c++11, without much work.

## Supporting Asynchronous Calls

Exposing methods which do not block the server is possible on the basis of [C++ Futures](http://www.modernescpp.com/index.php/component/content/article/44-blog/multithreading/multithreading-c-17-and-c-20/279-std-future-extensions?Itemid=239) Extension Concurrency TS which are already available via the [Boost Futures v.4implementation](https://www.boost.org/doc/libs/1_67_0/doc/html/thread/synchronization.html#thread.synchronization.futures). The idea is that methods which take longer to complete return a `future` of the result which is later collected and replied with in a continuation method `.then()`. This way the JSON-RPC server does not block it's thread while computing the response to a call.


The future should carry a type convertible to `jsonrpc::Value`. The non-blocking operation of the server can be reached by replacing `Server::HandleRequest` with `Server::asyncHandleRequest` and processing the response' `FormattedData` in the future callback: 

```C++
std::string incomingRequest;
ioStream >> incomingRequest; // read from a data source
server.asyncHandleRequest(incomingRequest)
.then([=](auto futureDataPtr){
   iostream << futureDataPtr.get()->GetData();  // write to the data sink
});
```

This can be done in general for all incoming requests, because the implementation of `Server::asyncHandleRequest()` can deal with synchronous plain-value-returning methods too. Except for [Lambda methods](#asynchronous-lambdas) no other changes are required by the implementation.

### Asynchronous Lambdas

For now rvalue references of asynchronous lambda are not supported and need to be wrapped with `std::function`. Also these `std::functions`s are not properly disassembled by the template machinery in `Dispatcher::AddMethod` and thus need to register via a custom method `Dispatcher::AddAsyncLambda`.

Asynchronous free static functions and member methods should register to the dispatcher in the same manner as the synchronous versions by `Dispatcher::AddMethod`.

### Additional Dependencies

Until the [TS for Extensions for Concurrency](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4107.html) are implemented by C++2a, asynchronous call handling depends on it's implementation in [Boost Thread](https://www.boost.org/doc/libs/1_67_0/doc/html/thread.html) v. 4. Thus building and linking to `boost_thread` (with `BOOST_THREAD_VERSION=4`) and `boost_system` (a dependency) is required. Linking to POSIX threads or the Windows counterpart is also needed for multi-threaded programs.

## Examples

A simple server that process JSON-RPC requests:

```C++
#include "jsonrpc-lean/server.h"

class Math {
public:
	int Add(int a, int b) {
		return a + b;
	}

	boost::future<int> AsyncAddInt(int a, int b) const {
		return boost::async([](auto a, auto b){
			return a + b;
		}, a, b);
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
   dispatcher.AddMethod("async_add_int", &Math::AsyncAddInt, math)
   dispatcher.AddMethod("add_array", &Math::AddArray, math); 

   // if it is just a regular function (non-member or static), you can you the 2 parameter AddMethod
   dispatcher.AddMethod("concat", &Concat);
   dispatcher.AddMethod("to_struct", &ToStruct);
   dispatcher.AddMethod("print_notification", &PrintNotification);

   std::function<boost::future<std::string>(std::string)> sReverse = [](std::string in) -> boost::future<std::string> { 
      std::string res;
      return boost::make_ready_future(res.assign(in.rbegin(), in.rend())); 
   };
   dispatcher.AddAsyncLambda("async_reverse", sReverse);

   // on a real world, these requests come from your own transport implementation (sockets, http, ipc, named-pipes, etc)
   const char addRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add\",\"id\":0,\"params\":[3,2]}";
   const char addIntAsyncRequest[] = R"({"jsonrpc":"2.0","method":"async_add_int","id":11,"params":[300,200]})";
   const char concatRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"concat\",\"id\":1,\"params\":[\"Hello, \",\"World!\"]}";
   const char addArrayRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"add_array\",\"id\":2,\"params\":[[1000,2147483647]]}";
   const char toStructRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"to_struct\",\"id\":5,\"params\":[[12,\"foobar\",[12,\"foobar\"]]]}";
   const char printNotificationRequest[] = "{\"jsonrpc\":\"2.0\",\"method\":\"print_notification\",\"params\":[\"This is just a notification, no response expected!\"]}";
   const char asyncReverseRequest[] = R"({"jsonrpc":"2.0","method":"async_reverse","id":13,"params":["xyz"]})";

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

   std::cout << "test async wrapper around sync\nrequest: " << addRequest << std::endl;
   server.asyncHandleRequest(addRequest)
	.then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr)
	{
		 std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl; // {"jsonrpc":"2.0","id":0,"result":5}
	});

   std::cout << "request: " << addIntAsyncRequest << std::endl;
   server.asyncHandleRequest(addIntAsyncRequest)
   .then([](boost::shared_future<std::shared_ptr<jsonrpc::FormattedData>> futureDataPtr){
    std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl; // {"jsonrpc":"2.0","id":11,"result":500}
   });

   std::cout << "request: " << asyncReverseRequest << std::endl;
   server.asyncHandleRequest(asyncReverseRequest)
   .then([](auto futureDataPtr){        // can use auto parameter type in C++14     
     std::cout << "response: " << futureDataPtr.get()->GetData() << std::endl; // {"jsonrpc":"2.0","id":13,"result":"zyx"}
   });

    // Sleep in the main thread to allow the threaded requests to be processed.
    boost::this_thread::sleep_for(boost::chrono::seconds(2));

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

