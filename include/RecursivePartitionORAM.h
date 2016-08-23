//
// Created by Dong Xie on 7/16/2016.
//

#ifndef SEAL_ORAM_RECURSIVEPARTITIONORAM_H
#define SEAL_ORAM_RECURSIVEPARTITIONORAM_H

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <cmath>
#include "PartitionORAMInstance.h"
#include "RecursiveTPPartitionORAMInstance.h"
#include "Util.h"
#include "Config.h"
#include "ORAM.h"
#include "MongoConnector.h"

using namespace CryptoPP;

template<class T>
class RecursivePartitionORAM: public ORAM {
public:
    RecursivePartitionORAM(const uint32_t& n, const double& rate);
    virtual ~RecursivePartitionORAM();

    virtual std::string get(const std::string & key);
    virtual void put(const std::string & key, const std::string & value);

    virtual std::string get(const uint32_t& block_id);
    virtual void put(const uint32_t& block_id, const std::string& data);
private:
    /********/
    void evict(uint32_t pid);
    /********/
    void sequentialEvict();

    void access(char op, uint32_t block_id, std::string & data);

    void updatePosMap(const uint32_t& id, const uint32_t& part, const uint32_t& level, const uint32_t& offset);
    uint32_t getPosMapPart(const uint32_t& id);

    uint32_t n_blocks;
    uint32_t n_partitions;
    uint64_t counter;
    double rate;
    uint32_t cur_partition;
    ORAM* pos_map;
    uint32_t pos_base = (B - Util::aes_block_size - 2 * sizeof(uint32_t)) / (3 * sizeof(uint32_t));

    MongoConnector* conn;

    /******/
    std::string *sbuffer;
    /******/
    std::unordered_map <uint32_t, std::string>* slots;
    PartitionORAMInstance** partitions;
};

template<class T>
RecursivePartitionORAM<T>::RecursivePartitionORAM(const uint32_t& n, const double& rate) : ORAM(), n_blocks(n), rate(rate) {
    n_partitions = (uint32_t) ceil(sqrt((double) n_blocks));
    uint32_t numofReals = (uint32_t) ceil(((double) n_blocks) / n_partitions);
    n_blocks = numofReals * n_partitions;

    conn = new MongoConnector(server_host);

    /******/
    partitions = new PartitionORAMInstance*[n_partitions];
    /******/

    counter = 0;
    cur_partition = 0;
    slots = new std::unordered_map<uint32_t, std::string>[n_partitions];

    pos_map = new PartitionORAM<TPPartitionORAMInstance>(n_blocks / pos_base + 1, rate);

    if (numofReals < 6) sbuffer = new std::string[(uint32_t) ceil(small_capacity_parameter * numofReals)];
    else sbuffer = new std::string[(uint32_t) ceil(capacity_parameter * numofReals)];

    size_t real_ind = 0;
    //size_t last_ind = 0;
    for (uint32_t i = 0; i < n_partitions; ++i) {
        partitions[i] = new T(numofReals, real_ind, i, sbuffer, pos_map, conn);
    }
}

template<class T>
RecursivePartitionORAM<T>::~RecursivePartitionORAM() {
    for (size_t i = 0; i < n_partitions; ++i)
        delete partitions[i];
    delete[] slots;
    delete[] partitions;
    delete pos_map;

    delete conn;
    delete[] sbuffer;
}

template<class T>
std::string RecursivePartitionORAM<T>::get(const std::string & key) {
    std::string res;
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    access('r', int_key, res);
    return res;
}

template<class T>
void RecursivePartitionORAM<T>::put(const std::string & key, const std::string & value) {
    uint32_t int_key;
    sscanf(key.c_str(), "%d", &int_key);
    std::string value2 = value;
    access('w', int_key, value2);
}

template<class T>
std::string RecursivePartitionORAM<T>::get(const uint32_t & key) {
    std::string res;
    access('r', key, res);
    return res;
}

template<class T>
void RecursivePartitionORAM<T>::put(const uint32_t & key, const std::string & value) {
    std::string value2 = value;
    access('w', key, value2);
}

template<class T>
void RecursivePartitionORAM<T>::evict(uint32_t pid) {
    if (!slots[pid].empty()) {
        auto got = slots[pid].begin();
        partitions[pid]->put(got->first, got->second);
        slots[pid].erase(got);
    } else {
        std::string dummy = Util::generate_random_block(B - AES::BLOCKSIZE - sizeof(uint32_t));
        int32_t dummyID = -1;
        std::string dID = std::string((const char *)(& dummyID), sizeof(uint32_t));
        dummy = dID + dummy;
        partitions[pid] -> put(-1, dummy);
    }
}

template<class T>
void RecursivePartitionORAM<T>::sequentialEvict() {
    if ((uint64_t) (counter * rate) > (uint64_t) ((counter - 1) * rate)) {
        evict(cur_partition);
        cur_partition = (cur_partition + 1) % n_partitions;
    }
}

template<class T>
void RecursivePartitionORAM<T>::access(char op, uint32_t block_id, std::string & data) {
    uint32_t r = (uint32_t)Util::rand_int(n_partitions);
    uint32_t p = getPosMapPart(block_id);
    auto got = slots[p].find(block_id);
    std::string data2;
    if (got != slots[p].end()) {
        data2 = got->second;
        slots[p].erase(got);
        partitions[p]->get(-1);
    } else data2 = partitions[p]->get(block_id);

    updatePosMap(block_id, r, -1, -1);
    uint32_t block_id2 = block_id;
    std::string bID = std::string((const char*)(& block_id2), sizeof(uint32_t));
    if (op == 'w') data2 = bID + data;
    else if (op == 'r') data = data2.substr(sizeof(uint32_t));
    slots[r][block_id] = data2;

    evict(p);

    counter++;
    sequentialEvict();
}

template<class T>
void RecursivePartitionORAM<T>::updatePosMap(const uint32_t& id, const uint32_t& part, const uint32_t& level, const uint32_t& offset) {
    std::string cur = pos_map->get(id / pos_base);
    cur.replace(sizeof(uint32_t) * 3 * (id % pos_base), sizeof(uint32_t), (const char*)&part, sizeof(uint32_t));
    cur.replace(sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t), sizeof(uint32_t), (const char*)&level, sizeof(uint32_t));
    cur.replace(sizeof(uint32_t) * 3 * (id % pos_base) + sizeof(uint32_t) * 2, sizeof(uint32_t), (const char*)&offset, sizeof(uint32_t));
    pos_map->put(id / pos_base, cur);
}

template<class T>
uint32_t RecursivePartitionORAM<T>::getPosMapPart(const uint32_t& id) {
    std::string cur = pos_map->get(id / pos_base);
    uint32_t ans;
    memcpy(&ans, cur.c_str() + sizeof(uint32_t) * 3 * (id % pos_base), sizeof(uint32_t));
    return ans;
}

#endif //SEAL_ORAM_RECURSIVEPARTITIONORAM_H
