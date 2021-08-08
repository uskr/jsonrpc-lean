// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_WRITER_H
#define JSONRPC_LEAN_WRITER_H

#include <string>
#include <memory>
#include "formatteddata.h"

struct tm;

namespace jsonrpc {

    class Value;

    class Writer {
    public:
        virtual ~Writer() {}

        // Result
        virtual std::shared_ptr<FormattedData> GetData() = 0;

        // Document
        virtual void StartDocument() = 0;
        virtual void EndDocument() = 0;

        // Request
        virtual void StartRequest(const std::string& methodName,
            const Value& id) = 0;
        virtual void EndRequest() = 0;
        virtual void StartParameter() = 0;
        virtual void EndParameter() = 0;

        // Response
        virtual void StartResponse(const Value& id) = 0;
        virtual void EndResponse() = 0;
        virtual void StartFaultResponse(const Value& id) = 0;
        virtual void EndFaultResponse() = 0;
        virtual void WriteFault(int32_t code, const std::string& string) = 0;

        // Values
        virtual void StartArray() = 0;
        virtual void EndArray() = 0;
        virtual void StartStruct() = 0;
        virtual void EndStruct() = 0;
        virtual void StartStructElement(const std::string& name) = 0;
        virtual void EndStructElement() = 0;
        virtual void WriteBinary(const char* data, size_t size) = 0;
        virtual void WriteNull() = 0;
        virtual void Write(bool value) = 0;
        virtual void Write(double value) = 0;
        virtual void Write(int32_t value) = 0;
        virtual void Write(int64_t value) = 0;
        virtual void Write(const std::string& value) = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_WRITER_H
