/*
 *  Copyright 2016 by SEAL-ORAM Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

//
// Created by Dong Xie on 7/6/2016.
//

#ifndef SEAL_ORAM_PATHORAM_H
#define SEAL_ORAM_PATHORAM_H

#include <unordered_map>
#include <cryptopp/config.h>
#include "ServerConnector.h"
#include "ORAM.h"
#include "Util.h"

class PathORAM: public ORAM {
public:
    PathORAM(const uint32_t& n);
    virtual ~PathORAM();

    virtual std::string get(const std::string & key);
    virtual void put(const std::string & key, const std::string & value);

    virtual std::string get(const uint32_t & key);
    virtual void put(const uint32_t & key, const std::string & value);
private:
    void access(const char& op, const uint32_t& block_id, std::string& data);
    bool check(int x, int y, int l);

    void fetchAlongPath(const uint32_t& x, std::string* sbuffer, size_t& length);
    void loadAlongPath(const uint32_t& x, const std::string* sbuffer, const size_t& length);

    std::unordered_map<uint32_t, std::string> stash;
    uint32_t *pos_map;
    std::vector< std::pair<uint32_t, std::string> > insert_buffer;

    byte* key;
    std::string* sbuffer;
    uint32_t n_blocks;
    uint32_t height;

    ServerConnector* conn;
};

#endif //SEAL_ORAM_PATHORAM_H
