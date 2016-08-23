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
// Mongo Server Connector
//

#ifndef SEAL_ORAM_MONGOCONNECTOR_H
#define SEAL_ORAM_MONGOCONNECTOR_H

#include <mongo/client/dbclient.h>
#include "ServerConnector.h"

using namespace mongo;

class MongoConnector: public ServerConnector {
public:
    MongoConnector(const std::string& url, const std::string& collection_name, const bool flag = false);
    MongoConnector(const std::string& url);
    virtual ~MongoConnector();

    struct iterator: public ServerConnector::iterator {
        iterator(std::unique_ptr<DBClientCursor> c): cursor(std::move(c)) {}

        virtual ~iterator() {}

        virtual bool hasNext() {
            return cursor->more();
        }

        virtual std::string next() {
            BSONObj p = cursor->next();
            int len;
            const char* raw_data = p.getField("data").binData(len);
            return std::string(raw_data, (size_t)len);
        }

        std::unique_ptr<DBClientCursor> cursor;
    };

    virtual void clear(const std::string& ns = "");
    virtual void insert(const uint32_t& id, const std::string& encrypted_block, const std::string& ns = "");
    virtual void insert(const std::vector< std::pair<uint32_t, std::string> >& blocks, const std::string& ns = "");
    virtual void insert(const std::string* sbuffer, const uint32_t& low, const size_t& len, const std::string& ns = "");
    virtual void insert(const std::vector< std::pair<std::string, std::string> >& blocks, const std::string& ns = "");
    virtual void insertWithTag(const std::vector< std::pair<std::string, std::string> >& blocks, const std::string& ns = "");
    virtual iterator* scan();
    virtual std::string find(const uint32_t& id, const std::string& ns = "");
    virtual void find(const std::vector<uint32_t>& ids, std::string* sbuffer, size_t& length, const std::string& ns = "");
    virtual std::string fetch(const std::string& id, const std::string& ns = "");
    virtual void find(const uint32_t& low, const uint32_t& high, std::vector<std::string>& blocks, const std::string& ns = "");
    virtual void findByTag(const uint32_t& tag, std::string* sbuffer, size_t& length, const std::string& ns = "");
    virtual void update(const uint32_t& id, const std::string& data, const std::string& ns = "");
    virtual void update(const std::string* sbuffer, const uint32_t& low, const size_t& len, const std::string& ns = "");
    virtual void update(const std::vector< std::pair<uint32_t, std::string> > blocks, const std::string& ns = "");

    void initialize(const std::string& ns);
    void finalize(const std::string& ns);

private:
    DBClientConnection mongo;
    std::string collection_name;
    bool by_tag;
};

#endif //SEAL_ORAM_MONGOCONNECTOR_H
