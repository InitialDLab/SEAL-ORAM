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

#include "SRORAM.h"

using namespace CryptoPP;

void SRORAM::initialize() {
    AutoSeededRandomPool prng;
    const uint32_t tmp_len = B - AES::BLOCKSIZE - 3 * sizeof(uint32_t);

    for(uint32_t k = 0; k < N; k ++) {
        byte tmp_buffer[tmp_len];
        prng.GenerateBlock(tmp_buffer, tmp_len);
        sbuffer[0] = std::string((const char*)tmp_buffer, tmp_len);

        int32_t bID = k;
        std::string blockID = std::string((const char*)(& bID), sizeof(uint32_t));
        sbuffer[0] = blockID + blockID + sbuffer[0];

        std::string hValue = Util::sha256_hash(blockID, salt);
        uint32_t h = std::hash<std::string>()(hValue);

        hValue = std::string((const char *)(&h), sizeof(uint32_t));
        sbuffer[0] = hValue + sbuffer[0];

        std::string cipher;
        Util::aes_encrypt(sbuffer[0], encKey, cipher);
        sbuffer[0] = cipher;

        conn->insert(k, sbuffer[0]);
    }

    for(uint32_t k = N; k < N + D; k ++) {
        byte tmp_buffer[tmp_len];
        prng.GenerateBlock(tmp_buffer, tmp_len);
        sbuffer[0] = std::string((const char*)tmp_buffer, tmp_len);

        int32_t bID = k;
        std::string blockID = std::string((const char*)(& bID), sizeof(uint32_t));
        int32_t dummy = -1;
        std::string dummyValue = std::string((const char*)(& dummy), sizeof(uint32_t));

        sbuffer[0] = blockID + dummyValue + sbuffer[0];

        std::string hValue = Util::sha256_hash(blockID, salt);
        uint32_t h = std::hash<std::string>()(hValue);
        hValue = std::string((const char *)(&h), sizeof(uint32_t));
        sbuffer[0] = hValue + sbuffer[0];

        std::string cipher;
        Util::aes_encrypt(sbuffer[0], encKey, cipher);
        sbuffer[0] = cipher;

        conn->insert(k, sbuffer[0]);
    }
    for(uint32_t k = N + D; k < C; k ++) {

        byte tmp_buffer[tmp_len];
        prng.GenerateBlock(tmp_buffer, tmp_len);
        sbuffer[0] = std::string((const char*)tmp_buffer, tmp_len);

        int32_t dID = -1;
        std::string blockID = std::string((const char*)(& dID), sizeof(uint32_t));

        sbuffer[0] = blockID + blockID + sbuffer[0];

        std::string hValue = Util::sha256_hash(blockID, salt);
        uint32_t h = std::hash<std::string>()(hValue);
        hValue = std::string((const char *)(&h), sizeof(uint32_t));
        sbuffer[0] = hValue + sbuffer[0];

        std::string cipher;
        Util::aes_encrypt(sbuffer[0], encKey, cipher);
        sbuffer[0] = cipher;

        conn->insert(k, sbuffer[0]);
    }
}

void SRORAM::oddeven_merge(uint32_t lo, uint32_t hi, uint32_t r, uint32_t len, char order) {
    uint32_t step = r * 2;
    if(step < hi - lo) {
        oddeven_merge(lo, hi, step, len, order);
        oddeven_merge(lo + r, hi, step, len, order);
        for(uint32_t i = lo + r; i < hi - r; i += step) {
            if(i + r < len)
                compare_and_swap(i, i + r, order);
        }
    }
    else {
        if(lo + r < len)
            compare_and_swap(lo, lo + r, order);
    }
}

void SRORAM::oddeven_merge_sort_range(uint32_t lo, uint32_t hi, uint32_t len, char order) {
    if(hi - lo >= 1) {
        uint32_t mid = lo + (hi - lo) / 2;
        oddeven_merge_sort_range(lo, mid, len, order);
        oddeven_merge_sort_range(mid + 1, hi, len, order);
        oddeven_merge(lo, hi, 1, len, order);
    }
}

void SRORAM::compare_and_swap(uint32_t a, uint32_t b, char order) {
    sbuffer[0] = readBlock(a);
    sbuffer[1] = readBlock(b);
    if(order == 's') {
        uint32_t hValue1;
        memcpy(&hValue1, sbuffer[0].c_str(), sizeof(uint32_t));
        uint32_t hValue2;
        memcpy(&hValue2, sbuffer[1].c_str(), sizeof(uint32_t));
        if(hValue1 > hValue2) {
            std::string temp = sbuffer[0];
            sbuffer[0] = sbuffer[1];
            sbuffer[1] = temp;
        }
    }
    else if(order == 'r') {
        int32_t bID1;
        memcpy(&bID1, sbuffer[0].substr(sizeof(uint32_t)).c_str(), sizeof(uint32_t));
        int32_t bID2;
        memcpy(&bID2, sbuffer[1].substr(sizeof(uint32_t)).c_str(), sizeof(uint32_t));
        if(bID1 == -1) {
            if(bID2 != -1) {
                std::string temp = sbuffer[0];
                sbuffer[0] = sbuffer[1];
                sbuffer[1] = temp;
            }
        }
        else if (bID2 != -1 && bID1 > bID2) {
            std::string temp = sbuffer[0];
            sbuffer[0] = sbuffer[1];
            sbuffer[1] = temp;
        }
    }
    writeBlock(a, sbuffer[0]);
    writeBlock(b, sbuffer[1]);
}

void SRORAM::obliviSort(uint32_t len, char order) {
    uint32_t test = len;
    uint32_t upper = 0;
    while(test != 0) {
        test >>= 1;
        upper ++;
    }
    oddeven_merge_sort_range(0, (1 << upper) - 1, len, order);
}

void SRORAM::rehash() {
    AutoSeededRandomPool prng;
    byte temp[AES::DEFAULT_KEYLENGTH];
    prng.GenerateBlock(temp, AES::DEFAULT_KEYLENGTH);
    salt = std::string((const char*)temp, AES::DEFAULT_KEYLENGTH);

    for(uint32_t i = 0; i < N + D; i ++) {
        sbuffer[0] = readBlock(i);
        std::string hValue = Util::sha256_hash(sbuffer[0].substr(sizeof(int32_t), sizeof(int32_t)), salt);
        uint32_t h = std::hash<std::string>()(hValue);
        hValue = std::string((const char *)(&h), sizeof(uint32_t));
        sbuffer[0] = hValue + sbuffer[0].substr(sizeof(int32_t));
        writeBlock(i, sbuffer[0]);
    }
}

std::string SRORAM::readBlock(uint32_t blockID) {
    std::string cipher = conn->find(blockID);
    std::string plain;
    Util::aes_decrypt(cipher, encKey, plain);
    return plain;
}

void SRORAM::writeBlock(uint32_t blockID, std::string &data) {
    std::string plain = data;
    std::string cipher;
    Util::aes_encrypt(plain, encKey, cipher);
    conn->update(blockID, cipher);
}

/*
    op: 'r' means read, 'w' means write;
    blockID: block ID;
    data: block content
*/
void SRORAM::access(char op, uint32_t blockID, std::string &data) {

    if(count == 0) {
        obliviSort(N + D, 's');
    }

    bool find = false;
    for(uint32_t i = N + D; i < N + D + count; i ++) {
        std::string temp = readBlock(i);
        int32_t bID;
        memcpy(&bID, temp.substr(sizeof(uint32_t)).c_str(), sizeof(int32_t));
        if(!find && bID == blockID) {
            find = true;
            sbuffer[0] = temp;
            if(op == 'r') {
                writeBlock(i, temp);
            }
            else if(op == 'w') {
                temp = temp.substr(0, sizeof(uint32_t)) + std::string((const char *)(&blockID), sizeof(uint32_t)) + data;
                writeBlock(i, temp);
            }
        }
        else {
            writeBlock(i, temp);
        }
    }

    if(find) {
        bool find_dummy = false;

        uint32_t dummyID = N + count;
        std::string hKey = std::string((const char *)(&dummyID), sizeof(uint32_t));
        std::string hValue = Util::sha256_hash(hKey, salt);
        uint32_t h = std::hash<std::string>()(hValue);
        hValue = std::string((const char *)(&h), sizeof(uint32_t));
        uint32_t hV;
        memcpy(&hV, hValue.c_str(), sizeof(uint32_t));

        //Binary Search
        uint32_t lo = 0;
        uint32_t hi = N + D - 1;
        uint32_t mid = -1;
        std::string temp;
        while(hi > lo + 1) {
            mid = (lo + hi) / 2;
            temp = readBlock(mid);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));
            if(hV == hV2) {
                find_dummy = true;
                break;
            }
            else if(hV < hV2) {
                hi = mid;
            }
            else lo = mid;
        }
        if(!find_dummy) {
            temp = readBlock(lo);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));
            if(hV == hV2) {
                find_dummy = true;
                mid = lo;
            }
        }
        if(!find_dummy) {
            temp = readBlock(hi);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));
            if(hV == hV2) {
                find_dummy = true;
                mid = hi;
            }
        }

        writeBlock(mid, temp);
    }
    else {
        bool find_real = false;
        std::string hKey = std::string((const char *)(&blockID), sizeof(uint32_t));
        std::string hValue = Util::sha256_hash(hKey, salt);
        uint32_t hV = std::hash<std::string>()(hValue);

        //Binary Search
        uint32_t lo = 0;
        uint32_t hi = N + D - 1;
        uint32_t mid = -1;
        std::string temp;
        while(hi > lo + 1) {
            mid = (lo + hi) / 2;
            temp = readBlock(mid);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));

            if(hV == hV2) {
                find_real = true;
                sbuffer[0] = temp;
                break;
            }
            else if(hV < hV2) {
                hi = mid;
            }
            else {
                lo = mid;
            }
        }
        if(!find_real) {
            temp = readBlock(lo);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));
            if(hV == hV2) {
                find_real = true;
                sbuffer[0] = temp;
                mid = lo;
            }
        }
        if(!find_real) {
            temp = readBlock(hi);
            uint32_t hV2;
            memcpy(&hV2, temp.c_str(), sizeof(uint32_t));
            if(hV == hV2) {
                find_real = true;
                sbuffer[0] = temp;
                mid = hi;
            }
        }

        std::string temp2;
        AutoSeededRandomPool prng;
        const uint32_t tmp_len = B - AES::BLOCKSIZE - 3 * sizeof(uint32_t);
        byte tmp_buffer[tmp_len];
        prng.GenerateBlock(tmp_buffer, tmp_len);
        temp2 = std::string((const char*)tmp_buffer, tmp_len);

        int32_t dID = -1;
        std::string dummyID = std::string((const char*)(& dID), sizeof(uint32_t));
        temp = temp.substr(0, sizeof(uint32_t)) + dummyID + dummyID + temp2;
        writeBlock(mid, temp);
    }

    uint32_t index = N + D + count;
    if(find) {
        std::string temp;
        AutoSeededRandomPool prng;
        const uint32_t tmp_len = B - AES::BLOCKSIZE - 3 * sizeof(uint32_t);
        byte tmp_buffer[tmp_len];
        prng.GenerateBlock(tmp_buffer, tmp_len);
        temp = std::string((const char*)tmp_buffer, tmp_len);

        int32_t dID = -1;
        std::string dummyID = std::string((const char*)(& dID), sizeof(uint32_t));
        prng.GenerateBlock(tmp_buffer, sizeof(uint32_t));
        std::string hashKey = std::string((const char*)tmp_buffer, sizeof(uint32_t));
        temp = hashKey + dummyID + dummyID + temp;

        writeBlock(index, temp);
    }
    else {
        if(op == 'r') {
            writeBlock(index, sbuffer[0]);
        }
        else if(op == 'w') {
            sbuffer[0] = sbuffer[0].substr(0, sizeof(uint32_t)) + std::string((const char *)(&blockID), sizeof(uint32_t)) + data;
            writeBlock(index, sbuffer[0]);
        }
    }

    if(op == 'r') {
        data = sbuffer[0].substr(sizeof(uint32_t) * 2);
    }

    count++;
    if(count == D) {

        obliviSort(C, 'r');

        rehash();
        count = 0;
    }
}

std::string SRORAM::get(const std::string & key) {
    std::string res;
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    access('r', int_key, res);
    return res;
}

void SRORAM::put(const std::string & key, const std::string & value) {
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    std::string value2 = value;
    access('w', int_key, value2);
}

std::string SRORAM::get(const uint32_t & key) {
    std::string res;
    access('r', key, res);
    return res;
}

void SRORAM::put(const uint32_t & key, const std::string & value) {
    std::string value2 = value;
    access('w', key, value2);
}
