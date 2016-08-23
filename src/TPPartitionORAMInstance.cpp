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

#include "TPPartitionORAMInstance.h"
#include "Util.h"
#include "MongoConnector.h"

using namespace CryptoPP;

TPPartitionORAMInstance::TPPartitionORAMInstance(const uint32_t& numofReals, size_t& real_ind,
                                                 const uint32_t& PID, std::string* s_buffer, Position* p_map,
                                                 MongoConnector * Conn)
        : PartitionORAMInstance(), PID(PID), pos_map(p_map) {
    n_levels = (uint32_t) (log((double) numofReals) / log(2.0)) + 1;
    if (numofReals < 6) capacity = (uint32_t)ceil(small_capacity_parameter * numofReals);
    else capacity = (uint32_t) ceil(capacity_parameter * numofReals);
    top_level_len = capacity - (1 << n_levels) + 2;
    flag = new bool[capacity];
    sbuffer = s_buffer;
    key = new byte[n_levels * AES::DEFAULT_KEYLENGTH];
    Util::prng.GenerateBlock(key, n_levels * AES::DEFAULT_KEYLENGTH);
    dummy = new uint32_t[(1 << n_levels) - 1];
    filled = new bool[n_levels];
    counter = 0;

    memset(flag, false, sizeof(bool) * capacity);
    char buf[100];
    sprintf(buf, "oram.test%d", PID);

    ns = std::string(buf);
    conn = Conn;
    conn->initialize(ns);

    for (uint32_t i = 0; i < n_levels - 1; ++i) {
        for (uint32_t j = 0; j < (1 << (i + 1)); ++j) {
            sbuffer[j] = Util::generate_random_block(B - AES::BLOCKSIZE - sizeof(uint32_t));
            int32_t dummyID = -1;
            std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
            sbuffer[j] = dID + sbuffer[j];
            dummy[(1 << i) - 1 + j] = j;
        }

        Util::psuedo_random_permute(&(dummy[(1 << i) - 1]), (size_t)(1 << (i + 1)));

        for (uint32_t j = 0; j < (1 << (i + 1)); ++j) {
            std::string cipher;
            Util::aes_encrypt(sbuffer[j], &(key[i * AES::DEFAULT_KEYLENGTH]), cipher);
            sbuffer[j] = cipher;
        }

        loadLevel(i, sbuffer, (size_t)(1 << (i + 1)), true);
    }

    for(uint32_t k = 0; k < numofReals; k ++) {
        sbuffer[k] = Util::generate_random_block(B - AES::BLOCKSIZE - 2 * sizeof(uint32_t));
        std::string dID = std::string((const char *)(& real_ind), sizeof(uint32_t));
        sbuffer[k] = dID + dID + sbuffer[k];

        real_ind ++;
    }

    for(uint32_t k = numofReals; k < top_level_len; k ++) {
        sbuffer[k] = Util::generate_random_block(B - AES::BLOCKSIZE - sizeof(uint32_t));
        int32_t dummyID = -1 + numofReals - k;
        std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
        sbuffer[k] = dID + sbuffer[k];
    }
    Util::psuedo_random_permute(sbuffer, top_level_len);

    for(uint32_t k = 0; k < top_level_len; k ++) {
        std::string bID = sbuffer[k].substr(0, sizeof(uint32_t));
        int32_t bID2;
        memcpy(&bID2, bID.c_str(), sizeof(uint32_t));
        if (bID2 < 0) {
            if (bID2 >= -(1 << (n_levels - 1)))
                dummy[((1 << (n_levels - 1)) - 1) - bID2 - 1] = k;
            int32_t dummyID2 = -1;
            sbuffer[k] = std::string((const char*)(& dummyID2), sizeof(uint32_t)) + sbuffer[k].substr(sizeof(uint32_t));
        } else {
            pos_map[bID2].part = PID;
            pos_map[bID2].level = n_levels - 1;
            pos_map[bID2].offset = k;
        }
        std::string cipher;
        Util::aes_encrypt(sbuffer[k], &(key[(n_levels - 1) * AES::DEFAULT_KEYLENGTH]), cipher);
        sbuffer[k] = cipher;
    }

    loadLevel(n_levels - 1, sbuffer, top_level_len, true);

    memset(filled, false, sizeof(bool) * (n_levels - 1));
    filled[n_levels- 1] = true;
}

TPPartitionORAMInstance::~TPPartitionORAMInstance() {
    delete[] flag;
    conn->finalize(ns);
    delete[] key;
    delete[] dummy;
    delete[] filled;
}

std::string TPPartitionORAMInstance::get(const int32_t& block_id) {
    std::string res;
    for (uint32_t l = 0; l < n_levels; l ++) {
        if (filled[l]) {
            if (block_id != -1 && pos_map[block_id].level == l) {
                std::string cipher;
                cipher = readBlock(l, pos_map[block_id].offset);
                Util::aes_decrypt(cipher, &(key[l * AES::DEFAULT_KEYLENGTH]), res);

            } else readBlock(l, dummy[(1 << l) - 1 + (counter % (1 << l))]);
        }
    }
    //pos_map.erase(block_id);
    return res;
}

void TPPartitionORAMInstance::put(const int32_t& block_id, const std::string& data) {
    uint32_t level = n_levels - 1;
    for (uint32_t l = 0; l < n_levels - 1; ++l) {
        if (!filled[l]) {
            level = l - 1;
            break;
        }
    }
    uint32_t begin_id = 0;
    uint32_t cur_id = begin_id;
    int32_t dID;
    size_t length;
    if (level != -1) {
        for (uint32_t l = 0; l <= level; l ++) {
            fetchLevel(l, sbuffer, begin_id, length);
            cur_id = begin_id;
            for (size_t i = begin_id; i < begin_id + length; i ++) {
                std::string plainBlocks;
                Util::aes_decrypt(sbuffer[i], &(key[l * AES::DEFAULT_KEYLENGTH]), plainBlocks);
                sbuffer[i] = plainBlocks;
                memcpy(&dID, sbuffer[i].c_str(), sizeof(uint32_t));
                if (pos_map[dID].part == PID && dID != -1 && dID != block_id && pos_map[dID].level == l) {
                    sbuffer[cur_id] = sbuffer[i];
                    cur_id ++;
                }
            }
            begin_id = cur_id;
        }
    }
    sbuffer[cur_id] = data;
    cur_id++;
    if (level != n_levels - 1) ++level;
    //AutoSeededRandomPool prng;
    Util::prng.GenerateBlock(&(key[level * Util::key_length]), Util::key_length);
    size_t sbuffer_len;
    if (level != n_levels - 1) sbuffer_len = (size_t)(1 << (level  + 1));
    else sbuffer_len = top_level_len;
    for (uint32_t i = cur_id; i < sbuffer_len; i ++) {
        sbuffer[i] = Util::generate_random_block(B - AES::BLOCKSIZE - sizeof(uint32_t));
        int32_t dummyID = cur_id - i - 1;
        std::string s_id = std::string((const char *)(& dummyID), sizeof(uint32_t));
        sbuffer[i] = s_id + sbuffer[i];
    }
    Util::psuedo_random_permute(sbuffer, sbuffer_len);

    for (uint32_t i = 0; i < sbuffer_len; i ++) {
        std::string bID = sbuffer[i].substr(0, sizeof(uint32_t));
        int32_t bID2;
        memcpy(&bID2, bID.c_str(), sizeof(uint32_t));
        if (bID2 < 0) {
            if (bID2 >= -(1 << level)) dummy[((1 << level) - 1) - bID2 - 1] = i;
            int32_t s_id = -1;
            sbuffer[i] = std::string((const char*)(& s_id), sizeof(uint32_t)) + sbuffer[i].substr(sizeof(uint32_t));
        } else {
            pos_map[bID2].part = PID;
            pos_map[bID2].level = level;
            pos_map[bID2].offset = i;
        }

        std::string cipher;
        Util::aes_encrypt(sbuffer[i], &(key[level * AES::DEFAULT_KEYLENGTH]), cipher);
        sbuffer[i] = cipher;
    }

    loadLevel(level, sbuffer, sbuffer_len, false);

    for (size_t i = 0; i < level; i ++)
        filled[i] = false;
    filled[level] = true;
    counter  = (counter + 1) % (1 << (n_levels - 1));
}

std::string TPPartitionORAMInstance::readBlock(uint32_t level, uint32_t offset) {
    uint32_t index = (1 << (level + 1)) - 2 + offset;
    flag[index] = true;
    return conn->find(index, ns);
}

void TPPartitionORAMInstance::fetchLevel(uint32_t level, std::string* sbuffer, uint32_t beginIndex, size_t & length) {
    uint32_t begin = (uint32_t)(1 << (level + 1)) - 2;
    uint32_t end;
    if (level != n_levels - 1) {
        end = begin + (1 << (level + 1));
    } else end = capacity;

    std::vector<std::string> blocks;

    conn->find(begin, end - 1, blocks, ns);

    length = 0;
    for (size_t i = 0; i < blocks.size(); i ++)
        if (!(flag[begin + i])) {
            sbuffer[beginIndex + length] = blocks[i];
            length++;
        }
    memset(&(flag[begin]), false, sizeof(bool) * (end - begin));
}

void TPPartitionORAMInstance::loadLevel(uint32_t level, std::string * sbuffer, size_t length, bool insert) {
    uint32_t begin = (uint32_t)(1 << (level + 1)) - 2;

    if (insert) conn->insert(sbuffer, begin, length, ns);
    else conn->update(sbuffer, begin, length, ns);

    memset(flag, false, sizeof(bool) * length);
}
