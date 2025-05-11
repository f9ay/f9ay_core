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

struct HuffmanNode {
    size_t frequency;
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;
};

template <ContainerConcept Container>
class HuffmanCoding {
public:
    HuffmanCoding() = default;
    ~HuffmanCoding() = default;

    void buildTree(Container& data) {
        std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>>
            frequencies;

        for (auto& element : data) {
            auto findResultIterator = _elementToFrequencyMap.find(element);
            if (findResultIterator == _elementToFrequencyMap.end()) {
                _elementToFrequencyMap[element] = 1;
            } else {
                _elementToFrequencyMap[element]++;
            }
        }

        for (auto& p : _elementToFrequencyMap) {
            auto element = p.first;
            auto frequency = p.second;

            _frequencyToElementMap[frequency] = element;

            std::println("Element: {}, Frequency: {}", element, frequency);

            frequencies.push(frequency);

            auto node = std::make_unique<HuffmanNode>();
            node->frequency = frequency;
            node->left = nullptr;
            node->right = nullptr;
            _nodesMap[frequency] = std::move(node);
        }

        _recursiveBuildTree(frequencies);
    }

    std::vector<std::vector<std::byte>> encode() {
        if (_root == nullptr) {
            throw std::runtime_error("Huffman tree not built");
        }

        std::vector<std::vector<std::byte>> encodedData;

        for (const auto& [frequency, element] : _frequencyToElementMap) {
            std::vector<std::byte> encodedBytes;

            _getCodeRecursively(_root, frequency, encodedBytes);

            std::print("frequency : {0} code : ", frequency);
            for (auto& byte : encodedBytes) {
                std::print("{0}", (int)byte);
            }
            std::println("");
            encodedData.push_back(encodedBytes);
        }

        return encodedData;
    }

    void decode(const std::vector<std::byte>& data) {
        // Implement the decoding logic
    }

private:
    void _recursiveBuildTree(auto& frequencies) {
        if (frequencies.empty()) {
            throw std::runtime_error("No frequencies available to build tree");
        }
        if (frequencies.size() == 1) {
            // only one element left, this is the root
            auto rootFrequency = frequencies.top();
            frequencies.pop();
            auto rootNode = std::move(_nodesMap[rootFrequency]);
            _root = std::move(rootNode);
            return;
        }

        // take two smallest frequencies
        auto leftFrequency = frequencies.top();
        frequencies.pop();
        auto rightFrequency = frequencies.top();
        frequencies.pop();

        auto leftNode = std::move(_nodesMap[leftFrequency]);
        auto rightNode = std::move(_nodesMap[rightFrequency]);
        auto newNode = std::make_unique<HuffmanNode>();
        newNode->frequency = leftFrequency + rightFrequency;

        newNode->left = std::move(leftNode);
        newNode->right = std::move(rightNode);
        frequencies.push(newNode->frequency);

        std::println("Left: {}, Right: {}, New Node: {}", leftFrequency,
                     rightFrequency, newNode->frequency);

        _nodesMap[newNode->frequency] = std::move(newNode);

        _recursiveBuildTree(frequencies);
    }

    void _getCodeRecursively(std::unique_ptr<HuffmanNode>& node,
                             const size_t freq,
                             std::vector<std::byte>& encodedData) {
        if (node == nullptr) {
            return;
        }

        if (node->left == nullptr && node->right == nullptr) {
            return;
        }

        if (node->left->frequency == freq) {
            encodedData.push_back(std::byte{0});
            _getCodeRecursively(node->left, freq, encodedData);
        } else {
            encodedData.push_back(std::byte{1});
            _getCodeRecursively(node->right, freq, encodedData);
        }
    }

    std::unique_ptr<HuffmanNode> _root;
    std::unordered_map<size_t, typename Container::value_type>
        _frequencyToElementMap;
    std::unordered_map<typename Container::value_type, size_t>
        _elementToFrequencyMap;
    std::unordered_map<size_t, std::unique_ptr<HuffmanNode>> _nodesMap;
};