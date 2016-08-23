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

#include <cmath>
#include <cryptopp/osrng.h>
#include "IBSOS.h"
#include "Config.h"
#include "Util.h"
#include "MongoConnector.h"

using namespace CryptoPP;

IBSOS::IBSOS(uint32_t n, uint32_t Alpha) : n_real(n) {
    alpha = Alpha;
    n_cache = (uint32_t)(ceil(sqrt(n_real + alpha * sqrt(n_real))));
    n_blocks = n_cache * n_cache;
    conn = new MongoConnector(server_host, "oram.squareroot");
    ibs_conn = new MongoConnector(server_host, "oram.ibs", true);
    stash.clear();
    insert_buffer.clear();

    AutoSeededRandomPool prng;
    key = new byte[AES::DEFAULT_KEYLENGTH];
    prng.GenerateBlock(key, Util::key_length);
    int32_t dummy_count = 0;
    salt = Util::generate_random_block(Util::aes_block_size);

    for (uint32_t i = 0; i < n_blocks; ++i) {
        std::string tmp  = Util::generate_random_block(B - AES::BLOCKSIZE - sizeof(int32_t));
        int32_t b_id;
        if (i < n_real) b_id = i;
        else {
            --dummy_count;
            b_id = dummy_count;
        }
        std::string s_id = std::string((const char *)(&b_id), sizeof(int32_t));
        std::string cipher;
        Util::aes_encrypt(s_id + tmp, key, cipher);
        insert_buffer.emplace_back(std::make_pair(Util::sha256_hash(s_id, salt), cipher));
        if (insert_buffer.size() == n_cache) {
            conn->insert(insert_buffer);
            insert_buffer.clear();
        }
    }

    n_cache = alpha * n_cache;
    sbuffer = new std::string[n_cache];
    IBS();
    op_counter = 0;
    dummy_counter = 0;
}

IBSOS::~IBSOS() {
    delete[] key;
    delete[] sbuffer;
    delete conn;
    delete ibs_conn;
}

std::string IBSOS::get(const std::string & key) {
    std::string res;
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    access('r', int_key, res);
    return res;
}

void IBSOS::put(const std::string & key, const std::string & value) {
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    std::string value2 = value;
    access('w', int_key, value2);
}

std::string IBSOS::get(const uint32_t & key) {
    std::string res;
    access('r', key, res);
    return res;
}

void IBSOS::put(const uint32_t & key, const std::string & value) {
    std::string value2 = value;
    access('w', key, value2);
}

void IBSOS::IBS() {
    insert_buffer.clear();
    for (auto&& item: stash) {
        std::string s_id = std::string((const char *)(&(item.first)), sizeof(int32_t));
        std::string cipher;
        Util::aes_encrypt(s_id + item.second, key, cipher);
        insert_buffer.emplace_back(std::make_pair(Util::sha256_hash(s_id, salt), cipher));
    }

    conn->insert(insert_buffer);
    stash.clear();
    uint32_t m = (uint32_t)(sqrt(n_blocks));
    ServerConnector::iterator* it = conn->scan();

    salt = Util::generate_random_block(Util::aes_block_size);
    for (uint32_t i = 0; i < m; ++i) {
        for (uint32_t j = 0; j < m; ++j) {
            assert(it->hasNext());
            std::string plain;
            Util::aes_decrypt(it->next(), key, plain);
            sbuffer[j] = plain;
        }

        Util::psuedo_random_permute(sbuffer, m);

        insert_buffer.clear();
        for (uint32_t j = 0; j < m; ++j) {
            int32_t b_id;
            memcpy(&b_id, sbuffer[j].c_str(), sizeof(int32_t));
            std::string s_id = std::string((const char *)(&b_id), sizeof(int32_t));
            std::string cipher;
            Util::aes_encrypt(sbuffer[j] ,key, cipher);
            insert_buffer.emplace_back(std::make_pair(Util::sha256_hash(s_id, salt), cipher));
        }
        ibs_conn->insertWithTag(insert_buffer);
    }
    delete it;
    conn->clear();

    salt = Util::generate_random_block(Util::aes_block_size);
    for (uint32_t i = 0; i < m; ++i) {
        size_t length;
        ibs_conn->findByTag(i, sbuffer, length);
        assert(length == m);
        for (size_t j = 0; j < m; ++j) {
            std::string plain;
            Util::aes_decrypt(sbuffer[j], key, plain);
            sbuffer[j] = plain;
        }

        Util::psuedo_random_permute(sbuffer, m);

        insert_buffer.clear();
        for (uint32_t j = 0; j < m; ++j) {
            int32_t b_id;
            memcpy(&b_id, sbuffer[j].c_str(), sizeof(int32_t));
            std::string s_id = std::string((const char *)(&b_id), sizeof(int32_t));
            std::string cipher;
            Util::aes_encrypt(sbuffer[j] ,key, cipher);
            insert_buffer.emplace_back(std::make_pair(Util::sha256_hash(s_id, salt), cipher));
        }
        conn->insert(insert_buffer);
    }
    ibs_conn->clear();
}

void IBSOS::access(const char& op, const uint32_t& block_id, std::string& data) {
    if (op_counter == n_cache) {
        IBS();
        op_counter = 0;
        dummy_counter = 0;
    }

    if (stash.find(block_id) == stash.end()) stash[block_id] = fetchBlock(block_id);
    else {
        --dummy_counter;
        stash[dummy_counter] = fetchBlock(dummy_counter);
    }
    if (op == 'r') data = stash[block_id];
    else stash[block_id] = data;
    ++op_counter;
}

std::string IBSOS::fetchBlock(int32_t block_id) {
    std::string s_id = std::string((const char *)(&block_id), sizeof(int32_t));
    std::string plain;
    Util::aes_decrypt(conn->fetch(Util::sha256_hash(s_id, salt)), key, plain);
    return plain.substr(sizeof(int32_t));
}