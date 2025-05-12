#pragma once
#include <array>
#include <cstddef>
#include <exception>
#include <list>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "container_concept.hpp"
#include "matrix_concept.hpp"
// class HuffmanBitStruct {
// public:
//     HuffmanBitStruct(const std::vector<std::byte>& bits) {
//         std::unique_ptr<std::byte[]> data(new std::byte[bits.size() / 8]);
//         _bitBuffer = bits.size() % 8;

//         auto dataPtr = data.get();
//         for (int i = 0; i < bits.size(); i++) {
//             if (bits[i] == std::byte{1}) {
//             }
//         }
//     }

//     HuffmanBitStruct(const HuffmanBitStruct& other) = default;
//     HuffmanBitStruct(HuffmanBitStruct&& other) = default;
//     HuffmanBitStruct& operator=(const HuffmanBitStruct& other) = default;
//     HuffmanBitStruct& operator=(HuffmanBitStruct&& other) = default;

// private:
//     std::unique_ptr<std::byte[]> _data;
//     int _bitBuffer;
// };

template <typename T>
struct HuffmanNode {
    HuffmanNode(size_t freq, T val)
        : frequency(freq),
          data(std::make_optional(val)),
          left(nullptr),
          right(nullptr) {}

    HuffmanNode(size_t freq, std::unique_ptr<HuffmanNode<T>>& left,
                std::unique_ptr<HuffmanNode<T>>& right)
        : frequency(freq),
          left(std::move(left)),
          right(std::move(right)),
          data(std::nullopt) {}

    size_t frequency;
    std::optional<T> data;
    std::unique_ptr<HuffmanNode<T>> left;
    std::unique_ptr<HuffmanNode<T>> right;

    bool operator==(const HuffmanNode& other) const {
        return frequency == other.frequency && data == other.data &&
               left == other.left && right == other.right;
    }
    bool operator!=(const HuffmanNode& other) const {
        return !(*this == other);
    }
};

namespace std {
template <typename T>
struct hash<HuffmanNode<T>> {
    size_t operator()(const HuffmanNode<T>& node) const {
        return std::hash<size_t>()(node.frequency) ^
               std::hash<std::optional<T>>()(node.data) ^
               std::hash<std::unique_ptr<HuffmanNode<T>>>()(node.left) ^
               std::hash<std::unique_ptr<HuffmanNode<T>>>()(node.right);
    }
};
}  // namespace std

template <ContainerConcept Container>
class HuffmanCoding {
public:
    HuffmanCoding() = default;
    ~HuffmanCoding() = default;

    void buildTree(const Container& data) {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        std::priority_queue<NodePtr, std::vector<NodePtr>, CompareNodes>
            nodesPq;

        for (auto& element : data) {
            auto findResultIterator = _elementToFrequencyMap.find(element);
            if (findResultIterator == _elementToFrequencyMap.end()) {
                _elementToFrequencyMap[element] = 1;
            } else {
                _elementToFrequencyMap[element]++;
            }
        }

        for (auto& [element, frequency] : _elementToFrequencyMap) {
            std::println("Element: {}, Frequency: {}", element, frequency);

            auto node =
                std::make_unique<HuffmanNode<typename Container::value_type>>(
                    frequency, element);
            nodesPq.push(std::move(node));
        }

        _recursiveBuildTree(nodesPq);
    }

    std::unordered_map<typename Container::value_type, std::vector<std::byte>>
    encode() {
        if (_root == nullptr) {
            throw std::runtime_error("Huffman tree not built");
        }

        for (const auto& [element, frequency] : _elementToFrequencyMap) {
            _getCodeRecursively(_root, element, frequency, {});
        }

        return _codeMap;
    }

    Container::value_type decode(const std::vector<std::byte>& data) {
        if (_root == nullptr) {
            throw std::runtime_error("Huffman tree not built");
        }

        return _decodeRecursively(data, (size_t)0, _root);
    }

    std::unordered_map<typename Container::value_type, std::vector<std::byte>>
    getCodeMap() {
        if (_root == nullptr) {
            throw std::runtime_error("Huffman tree not built");
        }
        return _codeMap;
    }

private:
    struct CompareNodes {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        bool operator()(const NodePtr& lhs, const NodePtr& rhs) const {
            return lhs->frequency > rhs->frequency;
        }
    };

    void _recursiveBuildTree(auto& nodePq) {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        if (nodePq.empty()) {
            throw std::runtime_error("No nodePq available to build tree");
        }
        if (nodePq.size() == 1) {
            // only one element left, this is the root
            _root = std::move(const_cast<NodePtr&>(nodePq.top()));
            nodePq.pop();
            return;
        }

        // take two smallest frequencies
        auto leftNode = std::move(const_cast<NodePtr&>(nodePq.top()));
        nodePq.pop();
        auto rightNode = std::move(const_cast<NodePtr&>(nodePq.top()));
        nodePq.pop();

        auto newNode =
            std::make_unique<HuffmanNode<typename Container::value_type>>(
                leftNode->frequency + rightNode->frequency, leftNode,
                rightNode);

        nodePq.push(std::move(newNode));

        _recursiveBuildTree(nodePq);
    }

    void _getCodeRecursively(
        std::unique_ptr<HuffmanNode<typename Container::value_type>>& node,
        const typename Container::value_type& element, const size_t freq,
        std::vector<std::byte> encodedData) {
        if (node->left == nullptr && node->right == nullptr) {
            if (node->data.value() == element) {
                _codeMap[element] = encodedData;
            }
            return;
        }

        if (node->left) {
            encodedData.push_back(std::byte{0});
            _getCodeRecursively(node->left, element, freq, encodedData);
            encodedData.pop_back();
        }
        if (node->right) {
            encodedData.push_back(std::byte{1});
            _getCodeRecursively(node->right, element, freq, encodedData);
        }

        // if left is not leaf node and right is not leaf node
    }

    typename Container::value_type _decodeRecursively(
        std::vector<std::byte> data, size_t index,
        std::unique_ptr<HuffmanNode<typename Container::value_type>>& node) {
        if (!node) {
            throw std::runtime_error("Invalid code");
        }

        if (index >= data.size()) {
            if (node->data.has_value()) {
                return node->data.value();
            } else {
                throw std::runtime_error("Invalid data");
            }
        }

        if (data[index] == std::byte{0}) {
            return _decodeRecursively(data, index + 1, node->left);

        } else {
            return _decodeRecursively(data, index + 1, node->right);
        }
    }
    std::unique_ptr<HuffmanNode<typename Container::value_type>> _root;
    std::unordered_map<typename Container::value_type, std::vector<std::byte>>
        _codeMap;
    std::unordered_map<typename Container::value_type, size_t>
        _elementToFrequencyMap;
};