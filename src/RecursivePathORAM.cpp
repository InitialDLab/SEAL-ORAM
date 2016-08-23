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
// Created by Dong Xie on 7/16/2016.
//

#include "RecursivePathORAM.h"

#include <cmath>
#include <cryptopp/osrng.h>
#include "MongoConnector.h"
#include "Util.h"

using namespace CryptoPP;

RecursivePathORAM::RecursivePathORAM(const uint32_t& n) {
    height = (uint32_t)ceil(log2((double)n)) + 1;
    n_blocks = (uint32_t)1 << (height - 1);
    stash.clear();
    stash_posmap.clear();
    pos_map = new PathORAM(n / pos_base + 1);
    conn = new MongoConnector(server_host, "oram.newrecursivepath");

    key = new byte[Util::key_length];
    Util::prng.GenerateBlock(key, Util::key_length);
    sbuffer = new std::string[height * PathORAM_Z];

    for (uint32_t i = 0; i < (1 << (height - 1)) - 1; ++i) {
        for(size_t j = 0; j < PathORAM_Z; j ++) {
            std::string tmp  = Util::generate_random_block(B - Util::aes_block_size - sizeof(uint32_t));
            int32_t dummyID = -1;
            std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
            std::string cipher;
            Util::aes_encrypt(dID + tmp, key, cipher);
            conn->insert(i * PathORAM_Z + j, cipher);
        }
    }
    uint32_t cur_id = 0;
    for (uint32_t i = (1 << (height - 1)) - 1; i < (1 << height) - 1; ++i) {
        if (cur_id < n) {
            //need to save pos_map
            std::string tmp  = Util::generate_random_block(B - Util::aes_block_size - 3 * sizeof(uint32_t));
            std::string bID = std::string((const char *)(& cur_id), sizeof(uint32_t));
            std::string cipher;
            Util::aes_encrypt(bID + bID + bID + tmp, key, cipher);
            conn->insert(i * PathORAM_Z, cipher);
            checkAndUpdatePosMap(cur_id, cur_id);
            cur_id ++;
        } else {
            std::string tmp  = Util::generate_random_block(B - Util::aes_block_size - sizeof(uint32_t));
            int32_t dummyID = -1;
            std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
            std::string cipher;
            Util::aes_encrypt(dID + tmp, key, cipher);
            conn->insert(i * PathORAM_Z, cipher);
        }

        for(size_t j = 1; j < PathORAM_Z; j ++) {
            std::string tmp  = Util::generate_random_block(B - Util::aes_block_size - sizeof(uint32_t));
            int32_t dummyID = -1;
            std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
            std::string cipher;
            Util::aes_encrypt(dID + tmp, key, cipher);
            conn->insert(i * PathORAM_Z + j, cipher);
        }
    }
}

RecursivePathORAM::~RecursivePathORAM() {
    delete pos_map;
    delete[] key;
    delete[] sbuffer;
    delete conn;
}

std::string RecursivePathORAM::get(const std::string & key) {
    std::string res;
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    access('r', int_key, res);
    return res;
}

void RecursivePathORAM::put(const std::string & key, const std::string & value) {
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    std::string value2 = value;
    access('w', int_key, value2);
}

std::string RecursivePathORAM::get(const uint32_t & key) {
    std::string res;
    access('r', key, res);
    return res;
}

void RecursivePathORAM::put(const uint32_t & key, const std::string & value) {
    std::string value2 = value;
    access('w', key, value2);
}

void RecursivePathORAM::access(const char& op, const uint32_t& block_id, std::string& data) {
    uint32_t new_x = Util::rand_int(n_blocks);
    std::string str_newx = std::string((const char *)(& new_x), sizeof(uint32_t));
    uint32_t x = checkAndUpdatePosMap(block_id, new_x);

    size_t length;
    fetchAlongPath(x, sbuffer, length);
    for (size_t i = 0; i < length; ++i) {
        std::string plain;
        Util::aes_decrypt(sbuffer[i], key, plain);
        int32_t b_id;
        memcpy(&b_id, plain.c_str(), sizeof(uint32_t));
        if (b_id != -1) {
            std::string content;
            if (b_id == block_id) {
                content = plain.substr(sizeof(uint32_t), sizeof(uint32_t)) + str_newx + plain.substr(3 * sizeof(uint32_t));
            }
            else content = plain.substr(sizeof(uint32_t));
            stash[b_id] = content;
            uint32_t tmp_pos_map;
            memcpy(&tmp_pos_map, content.substr(sizeof(uint32_t)).c_str(), sizeof(uint32_t));
            stash_posmap[b_id] = tmp_pos_map;
        }
    }

    //avoid the case that block_id is not in the retrieved path
    stash_posmap[block_id] = new_x;
    if (op == 'r') {
        data = stash[block_id];
    }
    std::string content = data.substr(0, sizeof(uint32_t)) + str_newx + data.substr(2 * sizeof(uint32_t));
    stash[block_id] = content;

    for (uint32_t i = 0; i < height; ++i) {
        uint32_t tot = 0;
        uint32_t base = i * PathORAM_Z;
        std::unordered_map<uint32_t, std::string>::iterator j, tmp;
        j = stash.begin();
        while (j != stash.end() && tot < PathORAM_Z) {
            if (check(stash_posmap[j->first], x, i)) {
                std::string b_id = std::string((const char *)(&(j->first)), sizeof(uint32_t));
                sbuffer[base + tot] = b_id + j->second;
                tmp = j; ++j;
                stash_posmap.erase(tmp->first);
                stash.erase(tmp);
                ++tot;
            } else ++j;
        }
        for (int k = tot; k < PathORAM_Z; ++k) {
            std::string tmp_block  = Util::generate_random_block(B - Util::aes_block_size - sizeof(uint32_t));
            int32_t dummyID = -1;
            std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
            sbuffer[base + k] = dID + tmp_block;
        }
    }

    for (size_t i = 0; i < height * PathORAM_Z; ++i) {
        std::string cipher;
        Util::aes_encrypt(sbuffer[i], key, cipher);
        sbuffer[i] = cipher;
    }
    loadAlongPath(x, sbuffer, height * PathORAM_Z);
}

bool RecursivePathORAM::check(int x, int y, int l) {
    return (x >> l) == (y >> l);
}

void RecursivePathORAM::fetchAlongPath(const uint32_t& x, std::string* sbuffer, size_t& length) {
    uint32_t cur_pos = x + (1 << (height - 1));
    std::vector<uint32_t> ids;
    while (cur_pos > 0) {
        for (uint32_t i = 0; i < PathORAM_Z; ++i)
            ids.push_back((cur_pos - 1) * PathORAM_Z + i);
        cur_pos >>= 1;
    }
    conn->find(ids, sbuffer, length);
}

void RecursivePathORAM::loadAlongPath(const uint32_t& x, const std::string* sbuffer, const size_t& length) {
    uint32_t cur_pos = x + (1 << (height - 1));
    uint32_t offset = 0;
    insert_buffer.clear();
    while (cur_pos > 0) {
        for (uint32_t i = 0; i < PathORAM_Z; ++i)
            insert_buffer.emplace_back(std::make_pair((cur_pos - 1) * PathORAM_Z + i, sbuffer[offset + i]));
        offset += PathORAM_Z;
        cur_pos >>= 1;
    }
    conn->update(insert_buffer);
}

uint32_t RecursivePathORAM::checkAndUpdatePosMap(const uint32_t& id, const uint32_t& pos) {
    std::string cur = pos_map->get(id / pos_base);
    uint32_t ans;
    memcpy(&ans, cur.c_str() + sizeof(uint32_t) * (id % pos_base), sizeof(uint32_t));
    cur.replace(sizeof(uint32_t) * (id % pos_base), sizeof(uint32_t), (const char*)&pos, sizeof(uint32_t));
    pos_map->put(id / pos_base, cur);
    return ans;
}

uint32_t RecursivePathORAM::getPosMap(const uint32_t& id) {
    std::string cur = pos_map->get(id / pos_base);
    uint32_t ans;
    memcpy(&ans, cur.c_str() + sizeof(uint32_t) * (id % pos_base), sizeof(uint32_t));
    return ans;
}
