#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <iostream>
#include <list>
#include <numeric>
#include <optional>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "container_concept.hpp"

namespace f9ay {
template <typename T, size_t N>
struct ArrayHash {
    std::size_t operator()(const std::array<T, N>& arr) const {
        std::size_t h = 0;
        // A common way to combine hashes
        // Inspired by Boost's hash_combine
        for (const auto& elem : arr) {
            h ^= std::hash<T>()(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};
class LZ77 {
public:
    // fast version of lz77
    // we use a windows that size is 3
    // and find it in the hash table
    // the hash table contains only one iterator
    // to the container
    // then we only find the longest match from that iterator
    template <int dictSize = 4096, int hashKeyLen = 3, int maxMatchLen = 258, ContainerConcept Container>
    static auto lz77EncodeFast(const Container& container) {
        std::vector<std::tuple<int, int, std::optional<typename Container::value_type>>> result;

        std::unordered_map<std::array<typename Container::value_type, hashKeyLen>, decltype(container.begin()),
                           ArrayHash<typename Container::value_type, hashKeyLen>>
            hashTable;

        auto bufferBegin = container.begin();

        while (bufferBegin != container.end()) {
            // extend the buffer to length of bufferSize or until the end of
            // the container
            std::array<typename Container::value_type, hashKeyLen> hashKey;

            if (std::distance(bufferBegin, container.end()) >= hashKeyLen) {
                std::copy(bufferBegin, bufferBegin + hashKeyLen, hashKey.begin());

                auto it = hashTable.find(hashKey);

                if (it != hashTable.end()) {
                    // first push it back to the hash table
                    // find longest match
                    auto dictMatchBegin = it->second;

                    // check if the match > 32768 (deflate limit)
                    int offset = std::distance(dictMatchBegin, bufferBegin);

                    if (offset > 32768) {
                        // don't allow offset to be greater than 32768
                        // so just push back the literal value
                        // and go to the next character
                        result.emplace_back(0, 0, *bufferBegin);
                        bufferBegin++;
                        continue;
                    }
                    int maxPossibleLength =
                        std::min({maxMatchLen, static_cast<int>(std::distance(dictMatchBegin, container.end())),
                                  static_cast<int>(std::distance(bufferBegin, container.end()))});
                    // save the hash key to the hash table
                    hashTable[hashKey] = bufferBegin;

                    auto [dictMatchEnd, lookheadEnd] = std::mismatch(
                        dictMatchBegin, dictMatchBegin + maxPossibleLength, bufferBegin,
                        bufferBegin + maxPossibleLength);

                    // let bufferBegin point to the next character
                    // after the match

                    int length = std::distance(dictMatchBegin, dictMatchEnd);

                    if (length < 3) {
                        // if the length is less than 3
                        // then we just push back the literal value
                        result.emplace_back(0, 0, *bufferBegin);
                        bufferBegin++;
                        continue;
                    }
                    if (lookheadEnd == container.end()) {
                        // if the lookahead end is the end of the container
                        // then we need to push back the last character
                        // and break
                        result.emplace_back(offset, length, std::nullopt);
                        break;
                    }

                    bufferBegin = lookheadEnd;
                    // push back the match
                    // and the next character
                    result.emplace_back(
                        offset, length,
                        (bufferBegin != container.end()) ? std::make_optional(*bufferBegin) : std::nullopt);

                    bufferBegin++;

                } else {
                    // push the hash key to the hash table
                    hashTable[hashKey] = bufferBegin;
                    // no match found
                    // push back the literal value
                    result.emplace_back(0, 0, *bufferBegin);
                    if (bufferBegin != container.end()) {
                        bufferBegin++;
                    }
                }

            } else {
                // not enough data to create a hash key
                // so just push back literal value
                for (; bufferBegin != container.end(); ++bufferBegin) {
                    result.emplace_back(0, 0, *bufferBegin);
                }
                break;
            }
        }

        return result;
    }

    // slow version of lz77
    // we use a windows that size is 3
    // and find it in the hash table
    // the hash table is a list of iterators
    // to the container
    // then we iterate over the list of iterators
    // and find the longest match
    // to get best compression
    template <int dictSize = 4096, int hashKeyLen = 3, int maxMatchLen = 258, ContainerConcept Container>
    static auto lz77EncodeSlow(const Container& container) {
        std::unordered_map<std::array<typename Container::value_type, hashKeyLen>,
                           std::vector<decltype(container.begin())>,
                           ArrayHash<typename Container::value_type, hashKeyLen>>
            hashTable;

        std::vector<std::tuple<int, int, std::optional<typename Container::value_type>>> result;

        auto bufferBegin = container.begin();

        while (bufferBegin != container.end()) {
            std::array<typename Container::value_type, hashKeyLen> hashKey;

            if (std::distance(bufferBegin, container.end()) >= hashKeyLen) {
                std::copy(bufferBegin, bufferBegin + hashKeyLen, hashKey.begin());
                auto it = hashTable.find(hashKey);

                if (it != hashTable.end()) {
                    int maxLength = 0;
                    int offset = 0;
                    decltype(container.begin()) maxMatchEnd;

                    for (auto dictMatchbegin : it->second) {
                        if(std::distance(dictMatchbegin, bufferBegin) > 32768) {
                            continue;
                        }
                        int maxPossibleLength =
                            std::min({maxMatchLen, static_cast<int>(std::distance(dictMatchbegin, container.end())),
                                      static_cast<int>(std::distance(bufferBegin, container.end()))});
                        auto [dictMatchEnd, lookheadEnd] = std::mismatch(
                            dictMatchbegin, dictMatchbegin + maxPossibleLength, bufferBegin,
                            bufferBegin + maxPossibleLength);

                        int length = std::distance(dictMatchbegin, dictMatchEnd);
                        if (length > maxLength) {
                            maxLength = length;
                            offset = std::distance(dictMatchbegin, bufferBegin);
                            maxMatchEnd = lookheadEnd;
                        }
                    }

                    // first push current back to hash table
                    hashTable[hashKey].emplace_back(bufferBegin);

                    // then emplace the longest match
                    if (maxLength >= 3) {
                        result.emplace_back(
                            offset, maxLength,
                            (maxMatchEnd != container.end()) ? std::make_optional(*maxMatchEnd) : std::nullopt);
                        bufferBegin = maxMatchEnd;
                    } else {
                        result.emplace_back(0, 0, *bufferBegin);
                    }

                    if (bufferBegin != container.end()) {
                        bufferBegin++;
                    }
                } else {
                    // push the hash key to the hash table
                    hashTable[hashKey].emplace_back(bufferBegin);
                    // no match found
                    // push back the literal value
                    result.emplace_back(0, 0, *bufferBegin);
                    if (bufferBegin != container.end()) {
                        bufferBegin++;
                    }
                }
            } else {
                // not enough data to create a hash key
                // so just push back literal value
                for (; bufferBegin != container.end(); ++bufferBegin) {
                    result.emplace_back(0, 0, *bufferBegin);
                }
                break;
            }
        }
        return result;
    }

    template <PushableContainerConcept Container>
    static auto lz77decode(auto encoded) {
        Container result;

        for (auto [offset, length, value] : encoded) {
            if (length == 0) {
                result.push_back(value.value());
            } else {
                // if length > offset then we need to repeat from front
                if (length > offset) {
                    auto start = result.end() - offset;
                    auto end = result.end();
                    result.insert(result.end(), start, end);

                    // add rest of the length
                    auto startIt = result.end() - offset;
                    std::vector<typename Container::value_type> temp(startIt, result.end());

                    for (size_t i = 0; i < length - offset; ++i) {
                        result.push_back(temp[i % offset]);
                    }
                } else {
                    result.insert(result.end(), result.end() - offset, result.end() - offset + length);
                }
                if (value.has_value()) {
                    result.push_back(value.value());
                }
            }
        }

        return result;
    }
};
}  // namespace f9ay