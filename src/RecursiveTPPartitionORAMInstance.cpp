//
// Created by Dong Xie on 7/16/2016.
//

#include "RecursiveTPPartitionORAMInstance.h"
#include "Util.h"
#include "MongoConnector.h"

using namespace CryptoPP;

RecursiveTPPartitionORAMInstance::RecursiveTPPartitionORAMInstance(const uint32_t& numofReals, size_t& real_ind,
                                                                   const uint32_t& PID, std::string* s_buffer,
                                                                   ORAM* p_map, MongoConnector * Conn)
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
    sprintf(buf, "oram.recursive_test%d", PID);

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
        }
        else updatePosMap(bID2, PID, n_levels - 1, k);

        std::string cipher;
        Util::aes_encrypt(sbuffer[k], &(key[(n_levels - 1) * AES::DEFAULT_KEYLENGTH]), cipher);
        sbuffer[k] = cipher;
    }

    loadLevel(n_levels - 1, sbuffer, top_level_len, true);

    memset(filled, false, sizeof(bool) * (n_levels - 1));
    filled[n_levels- 1] = true;
}

RecursiveTPPartitionORAMInstance::~RecursiveTPPartitionORAMInstance() {
    delete[] flag;

    conn->finalize(ns);

    delete[] key;
    delete[] dummy;
    delete[] filled;
}

std::string RecursiveTPPartitionORAMInstance::get(const int32_t& block_id) {
    uint32_t cur_part, cur_level, cur_offset;
    getPosMap(block_id, cur_part, cur_level, cur_offset);

    std::string res;
    for (uint32_t l = 0; l < n_levels; l ++) {
        if (filled[l]) {
            if (block_id != -1 && cur_level == l) {
                std::string cipher = readBlock(l, cur_offset);
                Util::aes_decrypt(cipher, &(key[l * AES::DEFAULT_KEYLENGTH]), res);
            } else readBlock(l, dummy[(1 << l) - 1 + (counter % (1 << l))]);
        }
    }
    return res;
}

void RecursiveTPPartitionORAMInstance::put(const int32_t& block_id, const std::string& data) {
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

            uint32_t cur_part, cur_level, cur_offset;

            uint32_t n_getPosMaps = 0;
            for (size_t i = begin_id; i < begin_id + length; i ++) {
                std::string plainBlocks;
                Util::aes_decrypt(sbuffer[i], &(key[l * AES::DEFAULT_KEYLENGTH]), plainBlocks);
                sbuffer[i] = plainBlocks;
                memcpy(&dID, sbuffer[i].c_str(), sizeof(uint32_t));
                if (dID != -1 && dID != block_id) {
                    getPosMap(dID, cur_part, cur_level, cur_offset);
                    n_getPosMaps ++;
                    if (cur_part == PID && cur_level == l) {
                        sbuffer[cur_id] = sbuffer[i];
                        cur_id ++;
                    }
                }
            }
            begin_id = cur_id;

            uint32_t level_length;
            if(l < n_levels)
                level_length = 1 << l;
            else level_length = top_level_len;
            int32_t dummyInt = -1;
            while (n_getPosMaps <= (level_length + 1) / 2) {
                getPosMap(dummyInt, cur_part, cur_level, cur_offset);
                n_getPosMaps ++;
            }
        }
    }
    sbuffer[cur_id] = data;
    cur_id++;
    if (level != n_levels - 1) ++level;
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


    uint32_t n_updatePosMaps = 0;
    for (uint32_t i = 0; i < sbuffer_len; i ++) {
        std::string bID = sbuffer[i].substr(0, sizeof(uint32_t));
        int32_t bID2;
        memcpy(&bID2, bID.c_str(), sizeof(uint32_t));
        if (bID2 < 0) {
            if (bID2 >= -(1 << level)) dummy[((1 << level) - 1) - bID2 - 1] = i;
            int32_t s_id = -1;
            sbuffer[i] = std::string((const char*)(& s_id), sizeof(uint32_t)) + sbuffer[i].substr(sizeof(uint32_t));
        }
        else {
            updatePosMap(bID2, PID, level, i);
            n_updatePosMaps ++;
        }

        std::string cipher;
        Util::aes_encrypt(sbuffer[i], &(key[level * AES::DEFAULT_KEYLENGTH]), cipher);
        sbuffer[i] = cipher;
    }

    uint32_t level_length;
    if(level < n_levels)
        level_length = 1 << level;
    else level_length = top_level_len;
    int32_t dummyInt = -1;
    while (n_updatePosMaps <= (level_length + 1) / 2) {
        updatePosMap(dummyInt, dummyInt, dummyInt, dummyInt);
        n_updatePosMaps ++;
    }

    loadLevel(level, sbuffer, sbuffer_len, false);

    for (size_t i = 0; i < level; i ++)
        filled[i] = false;
    filled[level] = true;
    counter  = (counter + 1) % (1 << (n_levels - 1));
}

std::string RecursiveTPPartitionORAMInstance::readBlock(uint32_t level, uint32_t offset) {
    uint32_t index = (1 << (level + 1)) - 2 + offset;
    flag[index] = true;
    return conn->find(index, ns);;
}

void RecursiveTPPartitionORAMInstance::fetchLevel(uint32_t level, std::string* sbuffer, uint32_t beginIndex, size_t & length) {
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

void RecursiveTPPartitionORAMInstance::loadLevel(uint32_t level, std::string * sbuffer, size_t length, bool insert) {
    uint32_t begin = (uint32_t)(1 << (level + 1)) - 2;

    if (insert) conn->insert(sbuffer, begin, length, ns);
    else conn->update(sbuffer, begin, length, ns);

    memset(flag, false, sizeof(bool) * length);
}

void RecursiveTPPartitionORAMInstance::updatePosMap(const uint32_t& id, const uint32_t& part, const uint32_t& level, const uint32_t& offset) {
    int32_t ID = id;
    if(ID >= 0) {
        std::string cur = pos_map->get(id / pos_base);
        cur.replace(sizeof(uint32_t) * 3 * (id % pos_base), sizeof(uint32_t), (const char*)&part, sizeof(uint32_t));
        cur.replace(sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t), sizeof(uint32_t), (const char*)&level, sizeof(uint32_t));
        cur.replace(sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t) * 2, sizeof(uint32_t), (const char*)&offset, sizeof(uint32_t));
        pos_map->put(id / pos_base, cur);
    }
    else{
        uint32_t dummyInt = -1;
        getPosMap(ID, dummyInt, dummyInt, dummyInt);
    }
}

void RecursiveTPPartitionORAMInstance::getPosMap(const uint32_t& id, uint32_t& part, uint32_t& level, uint32_t& offset) {
    int32_t ID = id;
    if(ID >= 0) {
        std::string cur = pos_map->get(id / pos_base);
        memcpy(&part, cur.c_str() + sizeof(uint32_t) * 3 * (id % pos_base), sizeof(uint32_t));
        memcpy(&level, cur.c_str() + sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t), sizeof(uint32_t));
        memcpy(&offset, cur.c_str() + sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t) * 2, sizeof(uint32_t));
    }
    else {
        pos_map->get(0);
    }
}
