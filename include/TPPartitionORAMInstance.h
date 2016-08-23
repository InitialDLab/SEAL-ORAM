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
// Created by Dong Xie on 7/13/2016.
//

#ifndef SEAL_ORAM_TPPARTITIONORAMINSTANCE_H
#define SEAL_ORAM_TPPARTITIONORAMINSTANCE_H

#include <cstring>
#include <cryptopp/config.h>
#include <unordered_map>
#include "Config.h"
#include "ServerConnector.h"
#include "PartitionORAMInstance.h"
#include "MongoConnector.h"

class TPPartitionORAMInstance: public PartitionORAMInstance {
public:
    TPPartitionORAMInstance(const uint32_t& numofReals, size_t& real_ind, const uint32_t& PID, std::string* sbuffer, Position* p_map, MongoConnector * Conn);
    virtual ~TPPartitionORAMInstance();

    virtual std::string get(const int32_t& block_id);
    virtual void put(const int32_t& block_id, const std::string& data);

private:
    std::string readBlock(uint32_t level, uint32_t offset);

    void fetchLevel(uint32_t level, std::string* sbuffer, uint32_t beginIndex, size_t & length);
    void loadLevel(uint32_t level, std::string * sbuffer, size_t length, bool insert);


    uint32_t n_levels;
    uint32_t top_level_len;
    uint32_t capacity;
    uint32_t PID;

    uint32_t* dummy;
    byte* key;
    std::string* sbuffer;
    bool* filled;
    size_t counter;
    Position* pos_map;

    MongoConnector *conn;
    bool* flag;

    std::string ns;
};

#endif //SEAL_ORAM_TPPARTITIONORAMINSTANCE_H
