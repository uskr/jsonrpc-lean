// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_REQUEST_DATA_H
#define JSONRPC_LEAN_REQUEST_DATA_H

namespace jsonrpc {

    class FormattedData {
    public:
        virtual ~FormattedData() {}

        // Data
        virtual const char* GetData() = 0;
        virtual size_t GetSize() = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_REQUEST_DATA_H