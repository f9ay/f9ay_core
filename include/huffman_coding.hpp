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

namespace f9ay {
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

template <ContainerConcept Container>
class HuffmanCoding {
public:
    HuffmanCoding() = default;
    ~HuffmanCoding() = default;

    // user need to build the tree first
    // then they can use getCodeMap to get the code map
    // and they can use
    void buildTree(const Container& data) {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        std::priority_queue<NodePtr, std::vector<NodePtr>, CompareNodes>
            nodesPq;

        // count every element frequency
        // and build the map
        for (auto& element : data) {
            auto findResultIterator = _elementToFrequencyMap.find(element);
            if (findResultIterator == _elementToFrequencyMap.end()) {
                _elementToFrequencyMap[element] = 1;
            } else {
                _elementToFrequencyMap[element]++;
            }
        }

        // create nodes for each element
        // and push them to the priority queue
        // this will be the leaf nodes
        for (auto& [element, frequency] : _elementToFrequencyMap) {
            auto node =
                std::make_unique<HuffmanNode<typename Container::value_type>>(
                    frequency, element);
            nodesPq.push(std::move(node));
        }

        // build the tree
        _recursiveBuildTree(nodesPq);

        // generate code map
        for (const auto& [element, frequency] : _elementToFrequencyMap) {
            // get the code for each element
            // pass an empty vector to the function
            _getCodeRecursively(_root, element, {});
        }
    }

    // decode the data with the huffman code
    // it will recursively decode the data
    // and return the decoded data
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
    // Custom comparator for the priority queue
    // if lhs > rhs, then rhs will pop earlier than lhs
    struct CompareNodes {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        bool operator()(const NodePtr& lhs, const NodePtr& rhs) const {
            return lhs->frequency > rhs->frequency;
        }
    };

    // recursive function to build the huffman tree
    void _recursiveBuildTree(auto& nodePq) {
        using NodePtr =
            std::unique_ptr<HuffmanNode<typename Container::value_type>>;
        // if there is no nodePq, then we can't build the tree
        if (nodePq.empty()) {
            throw std::runtime_error("No nodePq available to build tree");
        }
        // if there is only one node left, then this is the root
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

        // create a new node with the sum of the two frequencies
        // and push it to the priority queue
        auto newNode =
            std::make_unique<HuffmanNode<typename Container::value_type>>(
                leftNode->frequency + rightNode->frequency, leftNode,
                rightNode);

        nodePq.push(std::move(newNode));

        // recursively build the tree
        _recursiveBuildTree(nodePq);
    }

    // using the tree that we built, we can get the code for each element
    // use a recursive function to get the code for each element

    /**
     * @brief Recursively traverses the Huffman tree to generate the Huffman
     * code for a specific element.
     *
     * This function performs a DFS of the Huffman tree.
     * When moving to the left child, a bit '0' (represented as `std::byte{0}`)
     * is appended to the current code. When moving to the right child, a bit
     * '1' (represented as `std::byte{1}`) is appended. If a leaf node is
     * reached and its data matches the target `element`, the accumulated
     * `encodedData` path (which represents the Huffman code) is stored in the
     * `_codeMap` for that element.
     *
     *
     * @param node A reference to a unique_ptr to the current `HuffmanNode`
     * being processed in the traversal.
     * @param element The target element for which the Huffman code is to be
     * generated.
     * @param encodedData A `std::vector<std::byte>` that accumulates the bits
     * of the Huffman code during the recursive traversal. It is passed by
     * value.
     */
    void _getCodeRecursively(
        std::unique_ptr<HuffmanNode<typename Container::value_type>>& node,
        const typename Container::value_type& element,
        std::vector<std::byte> encodedData) {
        // leaf node
        if (node->left == nullptr && node->right == nullptr) {
            // if the target element is found, then we can store it in the map
            if (node->data.value() == element) {
                _codeMap[element] = encodedData;
            }
            return;
        }

        // DFS
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

    /// @brief Recursively decodes a sequence of bits using the Huffman tree.
    /// @param data The encoded data (Huffman code) to decode.
    /// @param index The current index in the encoded data.
    /// @param node The current node in the Huffman tree.
    /// @return The decoded element.
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
}  // namespace f9ay