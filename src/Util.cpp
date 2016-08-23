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

#include "Util.h"

using namespace CryptoPP;

size_t Util::key_length = (size_t)AES::DEFAULT_KEYLENGTH;
size_t Util::aes_block_size = (size_t)AES::BLOCKSIZE;
AutoSeededRandomPool Util::prng;
CFB_Mode<AES>::Encryption Util::encrypt_handler;
CFB_Mode<AES>::Decryption Util::decrypt_handler;
std::random_device Util::rd;
std::mt19937 Util::gen(Util::rd());

void Util::aes_encrypt(const std::string& plain, const byte* key, std::string& cipher) {
    byte iv[aes_block_size];
    encrypt_handler.GetNextIV(prng, iv);

    encrypt_handler.SetKeyWithIV(key, key_length, iv, aes_block_size);
    byte cipher_text[plain.length()];
    encrypt_handler.ProcessData(cipher_text, (byte*) plain.c_str(), plain.length());
    cipher = std::string((const char*)iv, aes_block_size) + std::string((const char*)cipher_text, plain.length());
}

void Util::aes_decrypt(const std::string& cipher, const byte* key, std::string& plain) {
    decrypt_handler.SetKeyWithIV(key, key_length, (byte*)cipher.c_str(), aes_block_size);
    size_t cipher_length = cipher.length() - aes_block_size;
    byte plain_text[cipher_length];
    decrypt_handler.ProcessData(plain_text, (byte*)cipher.substr(aes_block_size).c_str(), cipher_length);
    plain = std::string((const char*)plain_text, cipher_length);
}

std::string Util::sha256_hash(const std::string& key, const std::string& salt) {
    byte buf[SHA256::DIGESTSIZE];
    SHA256().CalculateDigest(buf, (byte*) ((key + salt).c_str()), key.length() + salt.length());
    return std::string((const char*)buf, (size_t)SHA256::DIGESTSIZE);
}

std::string Util::generate_random_block(const size_t& length) {
    byte buf[length];
    prng.GenerateBlock(buf, length);
    return std::string((const char*)buf, length);
}