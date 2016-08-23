//
// Created by Dong Xie on 7/10/2016.
//

#include <mongo/client/dbclient.h>
#include <cryptopp/osrng.h>
#include "ORAM.h"
#include "SRORAM.h"
#include "Config.h"

using namespace mongo;
using namespace CryptoPP;

int main() {
    mongo::client::initialize();

    srand((uint32_t)time(NULL));

    uint32_t N = 1024;
    ORAM* oram = new SRORAM(N);

    for(uint32_t i = 0; i < N; i ++) {
        char str[12];
        sprintf(str, "%zu\n", (size_t)i);
        std::string key(str);

        std::string value;
        const uint32_t tmp_len = B - AES::BLOCKSIZE - 2 * sizeof(uint32_t);
        byte tmp_buffer[tmp_len];
        AutoSeededRandomPool prng;
        prng.GenerateBlock(tmp_buffer, tmp_len);
        value = std::string((const char *)tmp_buffer, tmp_len);
        int32_t blockID = i;
        std::string bID = std::string((const char *)(& blockID), sizeof(uint32_t));
        value = bID + value;

        oram->put(key, value);
    }

    size_t roundNum = 5;
    for(size_t r = 0; r < roundNum; r ++) {
        for(size_t i = 0; i < N; i ++) {
            char str[12];
            sprintf(str, "%zu\n", i);
            std::string key(str);
            std::string block = oram->get(key);
            uint32_t value;
            memcpy((& value), block.c_str(), sizeof(uint32_t));
            printf("%d ", value);
        }
        printf("\n=========================================================================\n");
    }

    for(uint32_t i = 0; i < N; i ++) {
        char str[12];
        sprintf(str, "%zu\n", (size_t)i);
        std::string key(str);

        std::string value;
        const uint32_t tmp_len = B - AES::BLOCKSIZE - 2 * sizeof(uint32_t);
        byte tmp_buffer[tmp_len];
        AutoSeededRandomPool prng;
        prng.GenerateBlock(tmp_buffer, tmp_len);
        value = std::string((const char *)tmp_buffer, tmp_len);
        int32_t blockID = N - 1 - i;
        std::string bID = std::string((const char *)(& blockID), sizeof(uint32_t));
        value = bID + value;

        oram->put(key, value);
    }

    for(size_t i = 0; i < N; i ++) {
        char str[12];
        sprintf(str, "%zu\n", i);
        std::string key(str);
        std::string block = oram->get(key);
        uint32_t value;
        memcpy((& value), block.c_str(), sizeof(uint32_t));
        printf("%d ", value);
    }

    printf("\n=========================================================================\n");

    delete oram;
    mongo::client::shutdown();
    return 0;
}