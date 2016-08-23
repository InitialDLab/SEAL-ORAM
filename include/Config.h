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

#ifndef SEAL_ORAM_CONFIG_H
#define SEAL_ORAM_CONFIG_H

#include <cstdint>
#include <string>

const uint32_t B = 4096;
const std::string server_host = "localhost";

/* The basic binary tree ORAM */
const uint32_t Gamma = 2; //The eviction rate
const uint32_t Alpha = 2; //The ratio of the size of each bucket to log N

/* Path ORAM */
const uint32_t PathORAM_Z = 4;

/* TP-ORAM */
const double R = 0.9;
const double capacity_parameter = 4.6;
const double small_capacity_parameter = 6.0;
/* # of partitions can be modified in PartitionORAM.h */

#endif //SEAL_ORAM_CONFIG_H
