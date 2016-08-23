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

#ifndef SEAL_ORAM_SRORAM_H
#define SEAL_ORAM_SRORAM_H

#ifndef __SRORAM_H__
#define __SRORAM_H__

#include "Config.h"
#include "Util.h"
#include "ORAM.h"
#include "MongoConnector.h"

#include <cstdio>
#include <cassert>
#include <cstring>

using namespace CryptoPP;

class SRORAM: public ORAM {
public:
    SRORAM(uint32_t numofBlocks) {
        N = numofBlocks;
        D = (uint32_t) ceil(sqrt((double) N));
        C = N + 2 * D;
        conn = new MongoConnector(server_host, "oram.basicsr");
        count = 0;

        AutoSeededRandomPool prng;
        encKey = new byte[AES::DEFAULT_KEYLENGTH];
        prng.GenerateBlock(encKey, AES::DEFAULT_KEYLENGTH);

        byte temp[AES::DEFAULT_KEYLENGTH];
        prng.GenerateBlock(temp, AES::DEFAULT_KEYLENGTH);
        salt = std::string((const char*)temp, AES::DEFAULT_KEYLENGTH);
        sbuffer = new std::string[2];
        initialize();
    }

    ~SRORAM() {
        delete []encKey;
        delete []sbuffer;
        delete conn;
    }


    virtual std::string get(const std::string & key);
    virtual void put(const std::string& key, const std::string & value);

    virtual std::string get(const uint32_t & key);
    virtual void put(const uint32_t & key, const std::string & value);
private:
    void initialize();
    void access(char op, uint32_t blockID, std::string &data);

    std::string readBlock(uint32_t blockID);
    void writeBlock(uint32_t blockID, std::string &data);

    void oddeven_merge(uint32_t lo, uint32_t hi, uint32_t r, uint32_t len, char order);
    void oddeven_merge_sort_range(uint32_t lo, uint32_t hi, uint32_t len, char order);
    void compare_and_swap(uint32_t a, uint32_t b, char order);
    void obliviSort(uint32_t len, char order);
    void rehash();

    ServerConnector* conn;

    uint32_t N;    //# of real blocks
    uint32_t D;    //# of blocks in shelters
    uint32_t C;    //The capacity of the whole server

    uint32_t count;
    byte *encKey;    //Encryption Key
    std::string salt;    //Hash Salt
    std::string *sbuffer;    //Buffer for shuffling
};

#endif


#endif //SEAL_ORAM_SRORAM_H
