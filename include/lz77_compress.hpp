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
#include <vector>

#include "container_concept.hpp"

namespace f9ay {
class LZ77 {
public:
    template <int dictSize = 4096, int bufferSize = 10,
              ContainerConcept Container>
    static constexpr auto lz77Encode(const Container& container) {
        std::vector<
            std::tuple<int, int, std::optional<typename Container::value_type>>>
            result;

        auto bufferBegin = container.begin();
        auto bufferEnd = std::next(
            bufferBegin, std::min(bufferSize, static_cast<int>(container.end() -
                                                               bufferBegin)));

        // Create a dictionary with empty size
        auto dictBegin = container.begin();
        auto dictEnd = container.begin();

        while (bufferBegin != container.end()) {
            // extend the buffer to length of bufferSize or until the end of
            // the container
            bool hasFound = false;
            // find the longest match starting from the dictionary's back
            for (auto it = bufferEnd; it != bufferBegin; it--) {
                auto sequenceBegin =
                    std::find_end(dictBegin, dictEnd, bufferBegin, it);

                if (sequenceBegin != dictEnd) {  // found a match

                    hasFound = true;
                    auto matchLength = it - bufferBegin;
                    auto matchOffset =
                        std::distance(sequenceBegin, bufferBegin);

                    bufferBegin = std::next(
                        bufferBegin, std::min(matchLength + 1,
                                              container.end() - bufferBegin));
                    // move the dict window
                    dictEnd = std::next(
                        dictEnd,
                        std::min(matchLength + 1, container.end() - dictEnd));

                    // move the buffer window
                    bufferEnd = std::next(
                        bufferEnd,
                        std::min(matchLength + 1, container.end() - bufferEnd));
                    result.push_back(std::make_tuple(
                        matchOffset, matchLength,
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
                dictBegin =
                    std::next(dictBegin, dictEnd - dictBegin - dictSize);
            }

            if (bufferEnd - bufferBegin > bufferSize) {
                bufferBegin = std::next(bufferBegin,
                                        bufferEnd - bufferBegin - bufferSize);
            }
        }

        return result;
    }

    template <ContainerConcept Container>
    static constexpr auto lz77EncodeFast(const Container& container) {
        std::vector<
            std::tuple<int, int, std::optional<typename Container::value_type>>>
            result;
        if (container.empty()) {
            return result;
        }
        std::vector<int> sa = _buildSuffixArray(container);
        std::vector<int> isa = _buildInverseSuffixArray(sa);
        std::vector<int> psv = _buildPSV(sa);
        std::vector<int> nsv = _buildNSV(sa);

        size_t k = 0;
        const size_t n = container.size();

        while (k < n) {
            k = _LZ_Factor(k, psv, nsv, sa, isa, container, result);
        }

        return result;
    }

    template <PushableContainerConcept Container>
    static constexpr auto lz77decode(auto encoded) {
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

private:
    template <ContainerConcept Container>

    static std::vector<int> _buildSuffixArray(const Container& container) {
        std::vector<int> suffixArray(container.size());
        std::iota(suffixArray.begin(), suffixArray.end(), 0);

        std::sort(suffixArray.begin(), suffixArray.end(),
                  [&container](int a, int b) {
                      return std::lexicographical_compare(
                          container.begin() + a, container.end(),
                          container.begin() + b, container.end());
                  });

        return suffixArray;
    }

    static std::vector<int> _buildInverseSuffixArray(
        const std::vector<int>& suffixArray) {
        std::vector<int> inverseSuffixArray(suffixArray.size());
        for (int i = 0; i < suffixArray.size(); i++) {
            inverseSuffixArray[suffixArray[i]] = i;
        }
        return inverseSuffixArray;
    }

    static std::vector<int> _buildNSV(const std::vector<int>& sa) {
        const int n = static_cast<int>(sa.size());
        std::vector<int> nsv(n, n);  // 預設為n，表示不存在下一個較小值
        std::stack<int> stack;

        // 從右到左遍歷
        for (int i = n - 1; i >= 0; --i) {
            // 移除所有SA值大於等於當前值的項
            while (!stack.empty() && sa[stack.top()] >= sa[i]) {
                stack.pop();
            }

            // 如果堆疊非空，頂部就是NSV
            if (!stack.empty()) {
                nsv[i] = stack.top();
            }

            // 將當前位置壓入堆疊
            stack.push(i);
        }

        return nsv;
    }
    static std::vector<int> _buildPSV(const std::vector<int>& sa) {
        const int n = static_cast<int>(sa.size());
        std::vector<int> psv(n, -1);  // 預設為-1，表示不存在前一個較小值
        std::stack<int> stack;

        // 從左到右遍歷
        for (int i = 0; i < n; ++i) {
            // 移除所有SA值大於等於當前值的項
            while (!stack.empty() && sa[stack.top()] >= sa[i]) {
                stack.pop();
            }

            // 如果堆疊非空，頂部就是PSV
            if (!stack.empty()) {
                psv[i] = stack.top();
            }

            // 將當前位置壓入堆疊
            stack.push(i);
        }

        return psv;
    }  // 實作LZ_Factor演算法
    template <ContainerConcept Container>
    static size_t _LZ_Factor(
        size_t k, const std::vector<int>& psv, const std::vector<int>& nsv,
        const std::vector<int>& sa, const std::vector<int>& isa,
        const Container& container,
        std::vector<std::tuple<
            int, int, std::optional<typename Container::value_type>>>& result) {
        const int n_int = static_cast<int>(container.size());
        int current_sa_idx = isa[k];  // ISA[k] gives the rank of suffix k,
                                      // which is an index into SA, PSV, NSV

        int match_pos_in_string = 0;
        int match_len = 0;

        int psv_sa_idx = psv[current_sa_idx];
        int nsv_sa_idx = nsv[current_sa_idx];

        int lcp_psv =
            (psv_sa_idx != -1)
                ? _computeLCP(static_cast<int>(k), sa[psv_sa_idx], container)
                : 0;
        int lcp_nsv =
            (nsv_sa_idx != n_int)
                ? _computeLCP(static_cast<int>(k), sa[nsv_sa_idx], container)
                : 0;

        if (lcp_psv > lcp_nsv) {
            match_pos_in_string = sa[psv_sa_idx];
            match_len = lcp_psv;
        } else {                // 包括 lcp_nsv >= lcp_psv 的情況
            if (lcp_nsv > 0) {  // 只有當 nsv 提供了有效匹配時才使用
                match_pos_in_string = sa[nsv_sa_idx];
                match_len = lcp_nsv;
            } else {  // 如果 nsv 也是 0，則 len 保持為 0 (或 lcp_psv
                      // 如果它是唯一的非零)
                match_pos_in_string = (lcp_psv > 0)
                                          ? sa[psv_sa_idx]
                                          : 0;  // 預設值，如果 len 最終為 0
                match_len = lcp_psv;  // 如果 lcp_nsv 為 0，則取 lcp_psv
            }
        }
        // 如果兩個 LCP 都是 0，則 match_len 會是 0
        if (lcp_psv == 0 && lcp_nsv == 0) {
            match_len = 0;
        }

        if (match_len == 0) {
            result.push_back(std::make_tuple(0, 0, container[k]));
            return k + 1;
        }

        int offset = static_cast<int>(k) - match_pos_in_string;
        if (k + static_cast<size_t>(match_len) >= container.size()) {
            result.push_back(std::make_tuple(offset, match_len, std::nullopt));
        } else {
            result.push_back(
                std::make_tuple(offset, match_len,
                                container[k + static_cast<size_t>(match_len)]));
        }

        return k + std::max(1, match_len);
    }
    template <ContainerConcept Container>
    static int _computeLCP(int pos1, int pos2, const Container& container) {
        const int n = static_cast<int>(container.size());
        int len = 0;

        while (pos1 + len < n && pos2 + len < n &&
               container[pos1 + len] == container[pos2 + len]) {
            ++len;
        }

        return len;
    }
};
}  // namespace f9ay