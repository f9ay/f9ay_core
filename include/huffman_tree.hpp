#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <unordered_map>

struct huffman_coeff {
    uint16_t value;
    uint16_t length;
};

template <typename Char_T>
struct std::formatter<huffman_coeff, Char_T> : std::formatter<std::string, Char_T> {
    auto format(const huffman_coeff& coe, auto& ctx) const {
        auto out = ctx.out();
        std::string s = std::bitset<16>(coe.value).to_string();
        std::ranges::reverse(s);
        s.resize(coe.length);
        std::ranges::reverse(s);
        out = std::format_to(out, "{}", s);
        return out;
    }
};

class Huffman_tree {
public:
    struct tree_node {
        int freq = 0;
        std::optional<int> value;
        int l = -1, r = -1;
        tree_node(int f, int v) : freq(f), value(v) {}
        tree_node(int f, int l, int r) : freq(f), l(l), r(r) {}
        auto operator<=>(const tree_node& other) const {
            return freq <=> other.freq;
        }
    };

    std::unordered_map<uint16_t, uint32_t> freq_table;
    decltype(auto) add(const auto& vec) {
        for (auto& v : vec) {
            ++freq_table[v];
        }
        return *this;
    }

    void add_one(const auto& val) {
        ++freq_table[val];
    }

    std::vector<tree_node> nodes;

    struct node_ptr {
        int index;
    };

    node_ptr father_of_all = {-1};

    void build() {
        // for (int i = 0; i < 0xFFu; i++) {
        //     ++freq_table[i];
        // }

        std::println("freq check {}", freq_table);

        ////////////
        std::priority_queue<node_ptr, std::vector<node_ptr>, std::function<bool(const node_ptr&, const node_ptr&)>> pq(
            [this](const node_ptr& a, const node_ptr& b) {
                return nodes[a.index] > nodes[b.index];
            });
        for (auto& [value, freq] : freq_table) {
            nodes.emplace_back(freq, value);
        }
        for (int i = 0; i < nodes.size(); i++) {
            pq.push({i});
        }
        while (pq.size() > 1) {
            auto min1 = pq.top();
            pq.pop();
            auto min2 = pq.top();
            pq.pop();
            auto new_father = tree_node{
                nodes[min1.index].freq + nodes[min2.index].freq,
                min1.index,
                min2.index,
            };
            nodes.push_back(new_father);
            pq.emplace(nodes.size() - 1);  // last index
        }
        // pq.size() == 1 means father of all
        father_of_all = pq.top();
    }

    void dump() {
        dfs(father_of_all.index, 0);
    }

    std::optional<std::vector<std::pair<uint16_t, uint16_t>>> value_length_pair;
    // value length
    std::vector<std::pair<uint16_t, uint16_t>>& get_value_length_pair() {
        if (value_length_pair.has_value()) {
            return value_length_pair.value();
        }
        std::vector<std::pair<uint16_t, uint16_t>> result;
        dfs_set_len(result, father_of_all.index, 0);
        value_length_pair = std::move(result);
        return value_length_pair.value();
    }

    void dfs_set_len(std::vector<std::pair<uint16_t, uint16_t>>& result, int index, int len) {
        if (nodes[index].value.has_value()) {
            if (len == 0) {
                result.push_back({nodes[index].value.value(), 1});
            } else {
                result.push_back({nodes[index].value.value(), len});
            }
        } else {
            if (nodes[index].l == -1 || nodes[index].r == -1) {
                throw "NMSL";
            }
            dfs_set_len(result, nodes[index].l, len + 1);
            dfs_set_len(result, nodes[index].r, len + 1);
        }
    }

    std::optional<std::vector<std::pair<uint16_t, uint16_t>>> standard_huffman_table;
    // value len
    decltype(auto) get_standard_huffman_table() {
        if (standard_huffman_table.has_value()) {
            return standard_huffman_table.value();
        }
        auto table = get_value_length_pair();
        std::ranges::sort(table, [](const auto& a, const auto& b) {
            if (a.second == b.second) {
                return a.first < b.first;
            }
            return a.second < b.second;
        });
        standard_huffman_table = std::move(table);
        return standard_huffman_table.value();
    }

    std::optional<std::unordered_map<uint16_t, huffman_coeff>> standard_huffman_mapping;
    decltype(auto) get_standard_huffman_mapping() {
        if (standard_huffman_mapping.has_value()) {
            return standard_huffman_mapping.value();
        }
        auto standard_table = get_standard_huffman_table();
        std::unordered_map<uint16_t, huffman_coeff> result;

        result[standard_table[0].first] = huffman_coeff(0, standard_table[0].second);
        uint16_t current = 0;
        uint16_t current_len = standard_table[0].second;
        for (auto& [val, len] : standard_table | std::views::drop(1)) {
            result[val].length = len;
            current += 1;
            if (len > current_len) {
                current <<= 1;
            }
            current_len = len;
            result[val].value = current;
        }

        standard_huffman_mapping = std::move(result);
        return standard_huffman_mapping.value();
    }

    huffman_coeff getMapping(uint16_t value) {
        return get_standard_huffman_mapping().at(value);
    }

    void dfs(int index, int depth) {
        if (index < 0 || index >= nodes.size()) {
            return;
        }
        auto& node = nodes[index];
        if (node.l != -1 && node.r != -1) {
            dfs(node.l, depth + 1);
            // dfs(node.r, depth + 1);
        }
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";  // Indentation for depth
        }
        if (node.value.has_value()) {
            std::cout << "Value: " << node.value.value() << ", Frequency: " << node.freq << ", Depth: " << depth
                      << "\n";
        } else {
            std::cout << "Internal Node, Frequency: " << node.freq << ", Depth: " << depth << "\n";
        }
        if (node.l != -1 && node.r != -1) {
            // dfs(node.l, depth + 1);
            dfs(node.r, depth + 1);
        }
    }
};
