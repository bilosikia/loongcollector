/*
 * Copyright 2022 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <cstdint>

#include <string>

// Hash and file signature utility.
namespace logtail {

// Hash(string(@poolIn, @inputBytesNum)) => @md5.
// TODO: Same implementation in sdk module, merge them.
void DoMd5(const uint8_t* poolIn, const uint64_t inputBytesNum, uint8_t md5[16]);
std::string CalcMD5(const std::string& message);

bool SignatureToHash(const std::string& signature, uint64_t& sigHash, uint32_t& sigSize);
bool CheckAndUpdateSignature(const std::string& signature, uint64_t& sigHash, uint32_t& sigSize);
bool CheckFileSignature(const std::string& filePath, uint64_t sigHash, uint32_t sigSize);

int64_t HashString(const std::string& str);
int64_t HashSignatureString(const char* str, size_t strLen);

void HashCombine(size_t& seed, size_t value);

/**
 * This implementation is adapted from the OpenTelemetry project
 * Original source: https://github.com/open-telemetry/opentelemetry-cpp
 * (https://github.com/open-telemetry/opentelemetry-cpp/blob/edfeabe4cefbec2ba3697e41664e76f8bfcee52c/sdk/include/opentelemetry/sdk/common/attributemap_hash.h#L50)
 * Copyright The OpenTelemetry Authors
 * Licensed under the Apache License, Version 2.0 (the "License")
 * See original license at:
 * https://github.com/open-telemetry/opentelemetry-cpp/blob/main/LICENSE
 */
inline void AttrHashCombine(size_t& result, size_t value) {
    result ^= value + 0x9e3779b9 + (result << 6) + (result >> 2);
}
} // namespace logtail
