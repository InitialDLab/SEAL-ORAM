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

#ifndef SEAL_ORAM_IBSOS_H
#define SEAL_ORAM_IBSOS_H

#include <unordered_map>
#include <cryptopp/config.h>
#include "ServerConnector.h"
#include "ORAM.h"

class IBSOS: public ORAM {
public:
    IBSOS(uint32_t n, uint32_t Alpha);
    virtual ~IBSOS();

    virtual std::string get(const std::string & key);
    virtual void put(const std::string & key, const std::string & value);

    virtual std::string get(const uint32_t & key);
    virtual void put(const uint32_t & key, const std::string & value);
private:
    void IBS();

    void access(const char& op, const uint32_t& block_id, std::string& data);

    std::string fetchBlock(int32_t block_id);

    uint32_t alpha;

    uint32_t n_blocks;
    uint32_t n_real;
    uint32_t n_cache;
    uint32_t dummy_counter;
    uint32_t op_counter;

    std::unordered_map<int32_t, std::string> stash;
    byte* key;
    std::string* sbuffer;
    std::string salt;
    std::vector< std::pair<std::string, std::string> > insert_buffer;

    ServerConnector* conn;
    ServerConnector* ibs_conn;
};

#endif //SEAL_ORAM_IBSOS_H
