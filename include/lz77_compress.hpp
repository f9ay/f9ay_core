#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "container_concept.hpp"

namespace f9ay {
template <int dictSize = 4096, int bufferSize = 10, ContainerConcept Container>
inline constexpr auto lz77Encode(const Container& container) {
    std::vector<
        std::tuple<int, int, std::optional<typename Container::value_type>>>
        result;

    auto bufferBegin = container.begin();
    auto bufferEnd = std::next(
        bufferBegin,
        std::min(bufferSize, static_cast<int>(container.end() - bufferBegin)));

    // Create a dictionary with empty size
    auto dictBegin = container.begin();
    auto dictEnd = container.begin();

    while (bufferBegin != container.end()) {
        // extend the buffer to length of bufferSize or until the end of the
        // container
        bool hasFound = false;
        // find the longest match starting from the dictionary's back
        for (auto it = bufferEnd; it != bufferBegin; it--) {
            auto sequenceBegin =
                std::find_end(dictBegin, dictEnd, bufferBegin, it);

            if (sequenceBegin != dictEnd) {  // found a match

                hasFound = true;
                auto matchLength = it - bufferBegin;
                auto matchOffset = std::distance(sequenceBegin, bufferBegin);

                bufferBegin = std::next(
                    bufferBegin,
                    std::min(matchLength + 1, container.end() - bufferBegin));
                // move the dict window
                dictEnd = std::next(
                    dictEnd,
                    std::min(matchLength + 1, container.end() - dictEnd));

                // move the buffer window
                bufferEnd = std::next(
                    bufferEnd,
                    std::min(matchLength + 1, container.end() - bufferEnd));
                result.push_back(
                    std::make_tuple(matchOffset, matchLength,
                                    (it != container.end() && it != bufferEnd)
                                        ? std::make_optional(*it)
                                        : std::nullopt));

                break;
            }
        }

        if (!hasFound) {
            result.push_back(std::make_tuple(0, 0, *bufferBegin));
            bufferBegin++;
            bufferEnd = std::next(
                bufferBegin,
                std::min(bufferSize,
                         static_cast<int>(container.end() - bufferBegin)));
            dictEnd++;
        }

        if (dictEnd - dictBegin > dictSize) {
            dictBegin = std::next(dictBegin, dictEnd - dictBegin - dictSize);
        }

        if (bufferEnd - bufferBegin > bufferSize) {
            bufferBegin =
                std::next(bufferBegin, bufferEnd - bufferBegin - bufferSize);
        }
    }

    return result;
}

template <PushableContainerConcept Container>
inline constexpr auto lz77decode(auto encoded) {
    Container result;

    for (auto [offset, length, value] : encoded) {
        if (length == 0) {
            result.push_back(value.value());
        } else {
            result.insert(result.end(), result.end() - offset,
                          result.end() - offset + length);
            if (value.has_value()) {
                result.push_back(value.value());
            }
        }
    }

    return result;
}
}  // namespace f9ay