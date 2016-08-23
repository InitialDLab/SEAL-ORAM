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
// Created by Dong Xie on 7/10/2016.
//

#ifndef SEAL_ORAM_BINARYORAM_H
#define SEAL_ORAM_BINARYORAM_H

#include <unordered_map>
#include <cryptopp/config.h>
#include "ServerConnector.h"
#include "ORAM.h"
#include "Util.h"

class BinaryORAM: public ORAM {
public:
    BinaryORAM(const uint32_t& n);
    virtual ~BinaryORAM();

    virtual std::string get(const std::string & key);
    virtual void put(const std::string & key, const std::string & value);

    virtual std::string get(const uint32_t & key);
    virtual void put(const uint32_t & key, const std::string & value);
private:
    void access(const char& op, const uint32_t& block_id, std::string& data);

    void readBlock(const uint32_t level, const uint32_t bucket, const uint32_t piece);
    void writeBlock(const uint32_t level, const uint32_t bucket, const uint32_t piece);

    void fetchAlongPath(const uint32_t& block_id, const uint32_t& x);
    void evict(const uint32_t level, const uint32_t bucket);
    void evict();

    uint32_t *pos_map;

    byte* key;
    std::string* sbuffer;
    uint32_t n_buckets; //The number of buckets in the largest level
    uint32_t height; //The height of the binary tree
    uint32_t bucket_size; //The size of each bucket

    ServerConnector* conn;
};

#endif //SEAL_ORAM_BINARYORAM_H
