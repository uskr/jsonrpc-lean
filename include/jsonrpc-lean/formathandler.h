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

#ifndef JSONRPC_LEAN_FORMATHANDLER_H
#define JSONRPC_LEAN_FORMATHANDLER_H

#include <memory>
#include <string>

namespace jsonrpc {

    class Reader;
    class Writer;

    class FormatHandler {
    public:
        virtual ~FormatHandler() {}

        virtual bool CanHandleRequest(const std::string& contentType) = 0;
        virtual std::string GetContentType() = 0;
        virtual bool UsesId() = 0;
        virtual std::unique_ptr<Reader> CreateReader(const std::string& data) = 0;
        virtual std::unique_ptr<Writer> CreateWriter() = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_FORMATHANDLER_H
