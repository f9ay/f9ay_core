#pragma once
#include <algorithm>
#include <concepts>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

template <typename T>
concept ContainerConcept = requires(T t, const T ct) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;
    { t.size() } -> std::same_as<typename T::size_type>;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { ct.begin() } -> std::same_as<typename T::const_iterator>;
    { ct.end() } -> std::same_as<typename T::const_iterator>;
    { t[0] };
    { ct[0] };
};

template <ContainerConcept Container, int dictSize = 256, int bufferSize = 8>
constexpr std::vector<
    std::tuple<int, int, std::optional<typename Container::value_type>>>
ls77Encode(const Container& container) {
    auto bufferBegin = container.begin();
    auto bufferEnd = std::next(
        bufferBegin,
        std::min(bufferSize, static_cast<int>(container.end() - bufferBegin)));

    std::vector<
        std::tuple<int, int, std::optional<typename Container::value_type>>>
        result;

    // Create a dictionary with empty size
    auto dictBegin = container.begin();
    auto dictEnd = container.begin();

    while (bufferBegin != container.end()) {
        // extend the buffer to length of bufferSize or until the end of the
        // container
        bool hasFound = false;
        // find the longest match starting from the dictionary's back
        for (auto it = bufferEnd; it != bufferBegin; it--) {
            auto SequenceBegin =
                std::find_end(dictBegin, dictEnd, bufferBegin, it);

            if (SequenceBegin != dictEnd) {  // found a match

                hasFound = true;
                auto matchLength = it - bufferBegin;
                auto matchOffset = std::distance(SequenceBegin, bufferBegin);

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
                result.emplace_back(matchOffset, matchLength,
                                    (it != container.end() && it != bufferEnd)
                                        ? std::make_optional(*it)
                                        : std::nullopt);
                break;
            }
        }

        if (!hasFound) {
            result.emplace_back(0, 0, *bufferBegin);
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

template <ContainerConcept Container>
constexpr Container ls77decode(
    const std::vector<
        std::tuple<int, int, std::optional<typename Container::value_type>>>&
        encoded) {
    Container result;
    for (const auto& [offset, length, value] : encoded) {
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
