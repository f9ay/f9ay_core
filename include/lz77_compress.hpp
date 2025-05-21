#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <iostream>
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
    template <int dictSize = 4096, int hashKeyLen = 3,
              ContainerConcept Container>
    static auto lz77Encode(const Container& container) {
        std::vector<
            std::tuple<int, int, std::optional<typename Container::value_type>>>
            result;

        std::unordered_map<
            std::array<typename Container::value_type, hashKeyLen>,
            decltype(container.begin()),
            ArrayHash<typename Container::value_type, hashKeyLen>>
            hashTable;

        auto bufferBegin = container.begin();

        while (bufferBegin != container.end()) {
            // extend the buffer to length of bufferSize or until the end of
            // the container
            std::array<typename Container::value_type, hashKeyLen> hashKey;

            if (std::distance(bufferBegin, container.end()) >= hashKeyLen) {
                std::copy(bufferBegin, bufferBegin + hashKeyLen,
                          hashKey.begin());

                auto it = hashTable.find(hashKey);

                if (it != hashTable.end()) {
                    // first push it back to the hash table
                    // find longest match
                    auto matchBegin = it->second;

                    // check if the match > 32768 (deflate limit)
                    int offset = std::distance(matchBegin, bufferBegin);

                    if (offset > 32768) {
                        // don't allow offset to be greater than 32768
                        // so just push back the literal value
                        // and go to the next character
                        result.emplace_back(0, 0,
                                            std::make_optional(*bufferBegin));
                        bufferBegin++;
                        continue;
                    }
                    // save the hash key to the hash table
                    hashTable[hashKey] = bufferBegin;
                    // create two pointer to find longest match
                    auto matchPointer = matchBegin;
                    auto tempBufferPointer = bufferBegin;
                    // find longest match
                    while (tempBufferPointer != container.end() &&
                           *matchPointer == *tempBufferPointer) {
                        ++matchPointer;
                        ++tempBufferPointer;
                    }

                    // let bufferBegin point to the next character
                    // after the match

                    int length = std::distance(matchBegin, matchPointer);

                    bufferBegin = tempBufferPointer;
                    // push back the match
                    // and the next character
                    result.emplace_back(offset, length,
                                        (bufferBegin != container.end())
                                            ? std::make_optional(*bufferBegin)
                                            : std::nullopt);
                    if (bufferBegin != container.end()) {
                        bufferBegin++;
                    }

                } else {
                    // push the hash key to the hash table
                    hashTable[hashKey] = bufferBegin;
                    // no match found
                    // push back the literal value
                    result.emplace_back(0, 0, *bufferBegin);
                    bufferBegin++;
                }

            } else {
                // not enough data to create a hash key
                // so just push back literal value
                for (; bufferBegin != container.end(); ++bufferBegin) {
                    result.emplace_back(0, 0, std::make_optional(*bufferBegin));
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
                    auto restHead = result.end() - offset;
                    for (int i = 0; i < length - offset; i++) {
                        if (restHead != result.end()) {
                            result.push_back(*restHead);
                            ++restHead;
                        }
                    }
                } else {
                    result.insert(result.end(), result.end() - offset,
                                  result.end() - offset + length);
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