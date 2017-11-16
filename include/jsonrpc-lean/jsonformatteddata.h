// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Copyright (C) 2015 Adriano Maia <tony@stark.im>
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

#ifndef JSONRPC_LEAN_JSONREQUESTDATA_H
#define JSONRPC_LEAN_JSONREQUESTDATA_H

#include "formatteddata.h"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { typedef ::std::size_t SizeType; }

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace jsonrpc {

    class JsonFormattedData final : public FormattedData {
    public:
        JsonFormattedData() : Writer(myStringBuffer) {

        }

        const char* GetData() override {
            return myStringBuffer.GetString();
        }

        size_t GetSize() override {
            return myStringBuffer.GetSize();
        }

        rapidjson::Writer<rapidjson::StringBuffer> Writer;

    private:

        rapidjson::StringBuffer myStringBuffer;
        
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_JSONREQUESTDATA_H