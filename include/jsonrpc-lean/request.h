// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef JSONRPC_LEAN_REQUEST_H
#define JSONRPC_LEAN_REQUEST_H

#include "value.h"

#include <deque>
#include <string>

namespace jsonrpc {

    class Writer;

    class Request {
    public:
        typedef std::deque<Value> Parameters;

        Request(std::string methodName, Parameters parameters, Value id)
            : myMethodName(std::move(methodName)),
            myParameters(std::move(parameters)),
            myId(std::move(id)) {
            // Empty
        }

        const std::string& GetMethodName() const { return myMethodName; }
        const Parameters& GetParameters() const { return myParameters; }
        const Value& GetId() const { return myId; }

        void Write(Writer& writer) const {
            Write(myMethodName, myParameters, myId, writer);
        }

        static void Write(const std::string& methodName, const Parameters& params, const Value& id, Writer& writer) {
            writer.StartDocument();
            writer.StartRequest(methodName, id);
            for (auto& param : params) {
                writer.StartParameter();
                param.Write(writer);
                writer.EndParameter();
            }
            writer.EndRequest();
            writer.EndDocument();
        }

    private:
        std::string myMethodName;
        Parameters myParameters;
        Value myId;
    };

} // namespace jsonrpc

#endif //JSONRPC_LEAN_REQUEST_H
