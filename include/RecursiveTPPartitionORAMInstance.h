//
// Created by Dong Xie on 7/16/2016.
//

#ifndef SEAL_ORAM_RECURSIVETPPARTITIONORAMINSTANCE_H
#define SEAL_ORAM_RECURSIVETPPARTITIONORAMINSTANCE_H

#include <cstring>
#include <cryptopp/config.h>
#include <unordered_map>
#include "Config.h"
#include "ServerConnector.h"
#include "PartitionORAMInstance.h"
#include "MongoConnector.h"
#include "PartitionORAM.h"

class RecursiveTPPartitionORAMInstance: public PartitionORAMInstance {
public:
    RecursiveTPPartitionORAMInstance(const uint32_t& numofReals, size_t& real_ind, const uint32_t& PID,
                                     std::string* sbuffer, ORAM* p_map, MongoConnector * Conn);
    virtual ~RecursiveTPPartitionORAMInstance();

    virtual std::string get(const int32_t& block_id);
    virtual void put(const int32_t& block_id, const std::string& data);
private:
    std::string readBlock(uint32_t level, uint32_t offset);

    void fetchLevel(uint32_t level, std::string* sbuffer, uint32_t beginIndex, size_t & length);
    void loadLevel(uint32_t level, std::string * sbuffer, size_t length, bool insert);

    void updatePosMap(const uint32_t& id, const uint32_t& part, const uint32_t& level, const uint32_t& offset);
    void getPosMap(const uint32_t& id, uint32_t& part, uint32_t& level, uint32_t& offset);

    uint32_t n_levels;
    uint32_t top_level_len;
    uint32_t capacity;
    uint32_t PID;
    uint32_t pos_base = (B - Util::aes_block_size - 2 * sizeof(uint32_t)) / (3 * sizeof(uint32_t));

    uint32_t* dummy;
    byte* key;
    std::string* sbuffer;
    bool* filled;
    size_t counter;
    ORAM* pos_map;

    MongoConnector *conn;
    bool* flag;

    std::string ns;
};

#endif //SEAL_ORAM_RECURSIVETPPARTITIONORAMINSTANCE_H
