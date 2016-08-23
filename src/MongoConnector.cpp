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

#include <cstdio>
#include "MongoConnector.h"

MongoConnector::MongoConnector(const std::string& url, const std::string& collection_name, const bool flag)
        :ServerConnector(), mongo(), collection_name(collection_name) {
    mongo.connect(url);
    mongo.createCollection(collection_name);
    by_tag = flag;
    if (!by_tag) mongo.createIndex(collection_name, BSON("id" << 1));
    else mongo.createIndex(collection_name, BSON("tag" << 1));
}

MongoConnector::MongoConnector(const std::string& url)
        :ServerConnector(), mongo(), collection_name("") {
    mongo.connect(url);
}

void MongoConnector::initialize(const std::string& ns) {
    mongo.createCollection(ns);
    mongo.createIndex(ns, BSON("id" << 1));
}


void MongoConnector::finalize(const std::string& ns) {
    mongo.dropIndex(ns, BSON("id" << 1));
    mongo.dropCollection(ns);
}

MongoConnector::~MongoConnector() {
    if (collection_name == "") return;
    if (!by_tag) mongo.dropIndex(collection_name, BSON("id" << 1));
    else mongo.dropIndex(collection_name, BSON("tag" << 1));
    mongo.dropCollection(collection_name);
}

void MongoConnector::clear(const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    mongo.dropCollection(mongo_collection);
    mongo.createCollection(mongo_collection);
    if (!by_tag) mongo.createIndex(mongo_collection, BSON("id" << 1));
    else mongo.createIndex(mongo_collection, BSON("tag" << 1));
}

void MongoConnector::insert(const uint32_t& id, const std::string& encrypted_block, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    BinDataType bin_type = BinDataGeneral;
    BSONObj obj = BSONObjBuilder().append("id", id)
            .appendBinData("data", (int)encrypted_block.size(), bin_type, (const void*)encrypted_block.c_str()).obj();
    mongo.insert(mongo_collection, obj);
}

void MongoConnector::insert(const std::vector< std::pair<uint32_t, std::string> >& blocks, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::vector<BSONObj> objs;
    BinDataType bin_type = BinDataGeneral;
    for (size_t i = 0; i < blocks.size(); ++i)
        objs.push_back(BSONObjBuilder().append("id", blocks[i].first)
                               .appendBinData("data", (int)blocks[i].second.size(), bin_type,
                                              (const void*)blocks[i].second.c_str()).obj());
    mongo.insert(mongo_collection, objs);
}

void MongoConnector::insert(const std::string* sbuffer, const uint32_t& low, const size_t& len, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::vector<BSONObj> objs;
    BinDataType bin_type = BinDataGeneral;
    for (uint32_t i = low; i < low + len; ++i) {
        objs.push_back(BSONObjBuilder().append("id", i)
                               .appendBinData("data", (int)sbuffer[i - low].length(), bin_type,
                                              (const void*)sbuffer[i - low].c_str()).obj());
    }
    mongo.insert(mongo_collection, objs);
}

void MongoConnector::insert(const std::vector< std::pair<std::string, std::string> >& blocks, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::vector<BSONObj> objs;
    BinDataType bin_type = BinDataGeneral;
    for (size_t i = 0; i < blocks.size(); ++i)
        objs.push_back(BSONObjBuilder().append("id", blocks[i].first)
                               .appendBinData("data", (int)blocks[i].second.size(), bin_type,
                                              (const void*)blocks[i].second.c_str()).obj());
    mongo.insert(mongo_collection, objs);
}

void MongoConnector::insertWithTag(const std::vector< std::pair<std::string, std::string> >& blocks, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::vector<BSONObj> objs;
    BinDataType bin_type = BinDataGeneral;
    for (uint32_t i = 0; i < blocks.size(); ++i)
        objs.push_back(BSONObjBuilder().append("id", blocks[i].first)
                               .appendBinData("data", (int)blocks[i].second.size(), bin_type,
                                              (const void*)blocks[i].second.c_str())
                               .append("tag", i).obj());
    mongo.insert(mongo_collection, objs);
}

MongoConnector::iterator* MongoConnector::scan() {
    return new iterator(std::move((std::unique_ptr<DBClientCursor>)mongo.query(collection_name, BSONObj())));
}

std::string MongoConnector::find(const uint32_t& id, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::unique_ptr<DBClientCursor> cursor = mongo.query(mongo_collection, MONGO_QUERY("id" << id));
    while (cursor->more()) {
        BSONObj p = cursor->next();
        int len;
        const char* raw_data = p.getField("data").binData(len);
        return std::string(raw_data, (size_t)len);
    }
    return std::string();
}

void MongoConnector::find(const std::vector<uint32_t>& ids, std::string* sbuffer, size_t& length, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    BSONArrayBuilder id_arr;
    for (auto item: ids) id_arr.append(item);
    std::unique_ptr<DBClientCursor> cursor = mongo.query(mongo_collection, BSON("id" << BSON("$in" << id_arr.arr())));
    length = 0;
    while (cursor->more()) {
        BSONObj p = cursor->next();
        int block_len;
        const char* raw_data = p.getField("data").binData(block_len);
        sbuffer[length] = std::string(raw_data, (size_t)block_len);
        ++length;
    }
}

std::string MongoConnector::fetch(const std::string& id, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    BSONObj p = mongo.findAndRemove(mongo_collection, BSON("id" << id));
    int len;
    const char* raw_data = p.getField("data").binData(len);
    return std::string(raw_data, (size_t)len);
}

void MongoConnector::find(const uint32_t& low, const uint32_t& high, std::vector<std::string>& blocks, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    char buf[100];
    sprintf(buf, "{id: {$gte: %d, $lte: %d}}", low, high);
    std::unique_ptr<DBClientCursor> cursor = mongo.query(mongo_collection, Query(buf).sort("id"));
    while (cursor->more()) {
        BSONObj p = cursor->next();
        int len;
        const char* raw_data = p.getField("data").binData(len);
        blocks.push_back(std::string(raw_data, (size_t)len));
    }
}

void MongoConnector::findByTag(const uint32_t& tag, std::string* sbuffer, size_t& length, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    std::unique_ptr<DBClientCursor> cursor = mongo.query(mongo_collection, MONGO_QUERY("tag" << tag));
    length = 0;
    while (cursor->more()) {
        BSONObj p = cursor->next();
        int len;
        const char* raw_data = p.getField("data").binData(len);
        sbuffer[length] = std::string(raw_data, (size_t) len);
        length++;
    }
}

void MongoConnector::update(const uint32_t& id, const std::string& data, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    BinDataType bin_type = BinDataGeneral;
    mongo.update(mongo_collection, BSON("id" << id),
                 BSON("$set" << BSONObjBuilder().appendBinData("data", (int)data.length(), bin_type,
                                                               (const void*)data.c_str()).obj()));
}

void MongoConnector::update(const std::string* sbuffer, const uint32_t& low, const size_t& len, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    char buf[100];
    sprintf(buf, "{id: {$gte: %d, $lte: %ld}}", low, low + len - 1);
    mongo.remove(mongo_collection, Query(buf));
    insert(sbuffer, low, len, ns);
}

void MongoConnector::update(const std::vector< std::pair<uint32_t, std::string> > blocks, const std::string& ns) {
    std::string mongo_collection = (ns == "") ? collection_name : ns;
    BSONArrayBuilder id_arr;
    for (auto item: blocks) id_arr.append(item.first);
    mongo.remove(mongo_collection, BSON("id" << BSON("$in" << id_arr.arr())));
    insert(blocks);
}