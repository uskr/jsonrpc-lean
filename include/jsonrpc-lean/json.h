// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_JSON_H
#define JSONRPC_LEAN_JSON_H

namespace jsonrpc {
    namespace json {

        const char JSONRPC_NAME[] = "jsonrpc";
        const char JSONRPC_VERSION_2_0[] = "2.0";

        const char METHOD_NAME[] = "method";
        const char PARAMS_NAME[] = "params";
        const char ID_NAME[] = "id";

        const char RESULT_NAME[] = "result";

        const char ERROR_NAME[] = "error";
        const char ERROR_CODE_NAME[] = "code";
        const char ERROR_MESSAGE_NAME[] = "message";

    } // namespace json
} // namespace jsonrpc

#endif // JSONRPC_LEAN_JSON_H
