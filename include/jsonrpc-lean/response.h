// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_RESPONSE_H
#define JSONRPC_LEAN_RESPONSE_H

#include "value.h"

#define BOOST_THREAD_VERSION 4
#include <boost/thread/future.hpp>

namespace jsonrpc {

    class Writer;
	 
	 typedef std::unique_ptr<Writer> WriterPtr;
	 typedef std::shared_ptr<FormattedData> FormattedDataPtr;

    class Response {
    public:
        Response(Value value, Value id) : myResult(std::move(value)),
            myIsFault(false),
            myFaultCode(0),
			  myFaultDataString(""),
            myId(std::move(id)) {
        }

        Response(int32_t faultCode, std::string faultString, Value id, const std::string faultDataString = "") : myIsFault(true),
            myFaultCode(faultCode),
            myFaultString(std::move(faultString)),
				myFaultDataString(faultDataString),
            myId(std::move(id)) {
        }

        void Write(Writer& writer) const {
            writer.StartDocument();
            if (myIsFault) {
                writer.StartFaultResponse(myId);
                writer.WriteFault(myFaultCode, myFaultString, myFaultDataString);
                writer.EndFaultResponse();
            } else {
                writer.StartResponse(myId);
                myResult.Write(writer);
                writer.EndResponse();
            }
            writer.EndDocument();
        }
		  
		  boost::future<FormattedDataPtr> asyncWrite(WriterPtr writer) const 
		  {
			  writer->StartDocument();
			  
            if (myIsFault) 
				{	
                writer->StartFaultResponse(myId);
                writer->WriteFault(myFaultCode, myFaultString);
                writer->EndFaultResponse();
					 writer->EndDocument();
					 return boost::make_ready_future(writer->GetData());
            }
				
				if (myResult.IsFuture())
				{
					Value requestId(myId);
					
					return myResult.AsFuture()
						.then([writer = std::move(writer), myId = std::move(requestId)](boost::shared_future<Value> futureResult)
						{
							try
							{
								const Value& result = futureResult.get();
								
								writer->StartResponse(myId);
								result.Write(*writer);
								writer->EndResponse();
							}
							catch (const std::system_error& ex) {
								writer->StartFaultResponse(myId);
								writer->WriteFault(ex.code().value(), ex.what(), ex.code().category().name());
								writer->EndFaultResponse();
							}
							catch (const std::exception& ex) 
							{
								writer->StartFaultResponse(myId);
								writer->WriteFault(0, ex.what());
								writer->EndFaultResponse();
						   }
						   catch (...) {
								writer->StartFaultResponse(myId);
								writer->WriteFault(0, "unknown error");
								writer->EndFaultResponse();
						   }
							
							 writer->EndDocument();
							
							return writer->GetData();
						});
				}
				
				writer->StartResponse(myId);
				myResult.Write(*writer);
				writer->EndResponse();
            writer->EndDocument();
				return boost::make_ready_future(writer->GetData());
		  }

        Value& GetResult() { return myResult; }
        bool IsFault() const { return myIsFault; }

        void ThrowIfFault() const {
            if (!IsFault()) {
                return;
            }

            switch (static_cast<Fault::ReservedCodes>(myFaultCode)) {
            case Fault::RESERVED_CODE_MIN:
            case Fault::RESERVED_CODE_MAX:
            case Fault::SERVER_ERROR_CODE_MIN:
                break;
            case Fault::PARSE_ERROR:
                throw ParseErrorFault(myFaultString);
            case Fault::INVALID_REQUEST:
                throw InvalidRequestFault(myFaultString);
            case Fault::METHOD_NOT_FOUND:
                throw MethodNotFoundFault(myFaultString);
            case Fault::INVALID_PARAMETERS:
                throw InvalidParametersFault(myFaultString);
            case Fault::INTERNAL_ERROR:
                throw InternalErrorFault(myFaultString);
            }

            if (myFaultCode >= Fault::SERVER_ERROR_CODE_MIN
                && myFaultCode <= Fault::SERVER_ERROR_CODE_MAX) {
                throw ServerErrorFault(myFaultCode, myFaultString);
            }

            if (myFaultCode >= Fault::RESERVED_CODE_MIN
                && myFaultCode <= Fault::RESERVED_CODE_MAX) {
                throw PreDefinedFault(myFaultCode, myFaultString);
            }

            throw Fault(myFaultString, myFaultCode);
        }

        const Value& GetId() const { return myId; }

    private:
        Value myResult;
        bool myIsFault;
        int32_t myFaultCode;
        std::string myFaultString, myFaultDataString;
        Value myId;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_RESPONSE_H
