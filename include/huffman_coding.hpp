// #pragma once
// #include <array>
// #include <cstddef>
// #include <exception>
// #include <list>
// #include <memory>
// #include <queue>
// #include <unordered_map>
// #include <vector>
//
// #include "container_concept.hpp"
// #include "matrix_concept.hpp"
//
// namespace f9ay {
// template <typename T>
// struct HuffmanNode {
//     HuffmanNode(size_t freq, T val) : frequency(freq), data(std::make_optional(val)), left(nullptr), right(nullptr)
//     {}
//
//     HuffmanNode(size_t freq, std::unique_ptr<HuffmanNode<T>>& left, std::unique_ptr<HuffmanNode<T>>& right)
//         : frequency(freq), left(std::move(left)), right(std::move(right)), data(std::nullopt) {}
//
//     size_t frequency;
//     std::optional<T> data;
//     std::unique_ptr<HuffmanNode<T>> left;
//     std::unique_ptr<HuffmanNode<T>> right;
//
//     bool operator==(const HuffmanNode& other) const {
//         return frequency == other.frequency && data == other.data && left == other.left && right == other.right;
//     }
//     bool operator!=(const HuffmanNode& other) const {
//         return !(*this == other);
//     }
// };
// //
// // template <typename T>
// // class HuffmanCoding {
// // public:
// //     std::unordered_map<T, std::vector<std::byte>> table;
// //     template <ContainerConcept Container>
// //     decltype(auto) add(const Container& data) {
// //         // count every element frequency
// //         // and build the map
// //         for (auto& element : data) {
// //             if (uint32_t(element) >= 256) {
// //                 throw std::runtime_error("Element is greater than max");
// //             }
// //             // table[element] = to_vec(element);
// //         }
// //
// //         return *this;
// //     }
// //
// //     void build() {
// //         for (uint32_t i = 0; i < 0xFFu; i++) {
// //             table[i] = to_vec(i);
// //         }
// //     }
// //
// //     std::vector<std::byte> getMapping(T data) const {
// //         if (!table.contains(data)) {
// //             throw std::runtime_error("not found");
// //         }
// //         return table.at(data);
// //     }
// //
// //     std::unordered_map<T, std::vector<std::byte>> getCodeMap() {
// //         return table;
// //     }
// //
// //     static std::vector<std::byte> to_vec(uint32_t x) {
// //         // if (x == 0) return {static_cast<std::byte>(0)};
// //         std::vector<std::byte> vec(16);
// //         for (int i = 0; i < vec.size(); i++) {
// //             vec[i] = static_cast<std::byte>((x >> i) & 1);
// //         }
// //         std::ranges::reverse(vec);
// //         return vec;
// //     }
// // };
//
// template <typename T>
// class HuffmanCoding {
// public:
//     HuffmanCoding() = default;
//     ~HuffmanCoding() = default;
//
//     HuffmanCoding(HuffmanCoding&& other) noexcept {
//         if (this == &other) {
//             return;
//         }
//         _root = std::move(other._root);
//         _codeMap = std::move(other._codeMap);
//         _elementToFrequencyMap = std::move(other._elementToFrequencyMap);
//     }
//
//     template <ContainerConcept Container>
//     decltype(auto) add(const Container& data) {
//         // count every element frequency
//         // and build the map
//         for (auto& element : data) {
//             if (uint32_t(element) >= 256) {
//                 throw std::runtime_error("Element is greater than max");
//             }
//             auto findResultIterator = _elementToFrequencyMap.find(element);
//             if (findResultIterator == _elementToFrequencyMap.end()) {
//                 _elementToFrequencyMap[element] = 1;
//             } else {
//                 ++_elementToFrequencyMap[element];
//             }
//         }
//
//         return *this;
//     }
//
//     void build() {
//         using NodePtr = std::unique_ptr<HuffmanNode<T>>;
//         std::priority_queue<NodePtr, std::vector<NodePtr>, CompareNodes> nodesPq;
//
//         // create nodes for each element
//         // and push them to the priority queue
//         // this will be the leaf nodes
//         for (auto& [element, frequency] : _elementToFrequencyMap) {
//             auto node = std::make_unique<HuffmanNode<T>>(frequency, element);
//             nodesPq.push(std::move(node));
//         }
//
//         // build the tree
//         _recursiveBuildTree(nodesPq);
//
//         // generate code map
//         _getCodeRecursively(_root, {});
//     }
//
//     // decode the data with the huffman code
//     // it will recursively decode the data
//     // and return the decoded data
//     T decode(const std::vector<std::byte>& data) {
//         if (_root == nullptr) {
//             throw std::runtime_error("Huffman tree not built");
//         }
//
//         return _decodeRecursively(data, (size_t)0, _root);
//     }
//
//     std::vector<std::byte> getMapping(T data) const {
//         if (!_codeMap.contains(data)) {
//             throw std::runtime_error("not found");
//         }
//         return _codeMap.at(data);
//     }
//
//     std::unordered_map<T, std::vector<std::byte>> getCodeMap() {
//         if (_root == nullptr) {
//             throw std::runtime_error("Huffman tree not built");
//         }
//         return _codeMap;
//     }
//
// private:
//     // Custom comparator for the priority queue
//     // if lhs > rhs, then rhs will pop earlier than lhs
//     struct CompareNodes {
//         using NodePtr = std::unique_ptr<HuffmanNode<T>>;
//         bool operator()(const NodePtr& lhs, const NodePtr& rhs) const {
//             return lhs->frequency > rhs->frequency;
//         }
//     };
//
//     // recursive function to build the huffman tree
//     void _recursiveBuildTree(auto& nodePq) {
//         using NodePtr = std::unique_ptr<HuffmanNode<T>>;
//         // if there is no nodePq, then we can't build the tree
//         if (nodePq.empty()) {
//             throw std::runtime_error("No nodePq available to build tree");
//         }
//         // if there is only one node left, then this is the root
//         if (nodePq.size() == 1) {
//             // only one element left, this is the root
//             _root = std::move(const_cast<NodePtr&>(nodePq.top()));
//             nodePq.pop();
//             return;
//         }
//
//         // take two smallest frequencies
//         auto leftNode = std::move(const_cast<NodePtr&>(nodePq.top()));
//         nodePq.pop();
//         auto rightNode = std::move(const_cast<NodePtr&>(nodePq.top()));
//         nodePq.pop();
//
//         // create a new node with the sum of the two frequencies
//         // and push it to the priority queue
//         auto newNode =
//             std::make_unique<HuffmanNode<T>>(leftNode->frequency + rightNode->frequency, leftNode, rightNode);
//
//         nodePq.push(std::move(newNode));
//
//         // recursively build the tree
//         _recursiveBuildTree(nodePq);
//     }
//
//     // using the tree that we built, we can get the code for each element
//     // use a recursive function to get the code for each element
//
//     /**
//      * @brief Recursively traverses the Huffman tree to generate the Huffman
//      * code for a specific element.
//      *
//      * This function performs a DFS of the Huffman tree.
//      * When moving to the left child, a bit '0' (represented as `std::byte{0}`)
//      * is appended to the current code. When moving to the right child, a bit
//      * '1' (represented as `std::byte{1}`) is appended. If a leaf node is
//      * reached and its data matches the target `element`, the accumulated
//      * `encodedData` path (which represents the Huffman code) is stored in the
//      * `_codeMap` for that element.
//      *
//      *
//      * @param node A reference to a unique_ptr to the current `HuffmanNode`
//      * being processed in the traversal.
//      * @param element The target element for which the Huffman code is to be
//      * generated.
//      * @param encodedData A `std::vector<std::byte>` that accumulates the bits
//      * of the Huffman code during the recursive traversal. It is passed by
//      * value.
//      */
//     void _getCodeRecursively(std::unique_ptr<HuffmanNode<T>>& node, std::vector<std::byte> encodedData) {
//         if (_root->left == nullptr && _root->right == nullptr) {
//             _codeMap[node->data.value()] = {std::byte{0}};
//
//             return;
//         }
//
//         // leaf node
//         if (node->left == nullptr && node->right == nullptr) {
//             // if the target element is found, then we can store it in the map
//
//             if (node->data.has_value()) {
//                 _codeMap[node->data.value()] = encodedData;
//             }
//
//             return;
//         }
//
//         // DFS
//         if (node->left) {
//             encodedData.push_back(std::byte{0});
//             _getCodeRecursively(node->left, encodedData);
//             encodedData.pop_back();
//         }
//         if (node->right) {
//             encodedData.push_back(std::byte{1});
//             _getCodeRecursively(node->right, encodedData);
//         }
//
//         // if left is not leaf node and right is not leaf node
//     }
//
//     /// @brief Recursively decodes a sequence of bits using the Huffman tree.
//     /// @param data The encoded data (Huffman code) to decode.
//     /// @param index The current index in the encoded data.
//     /// @param node The current node in the Huffman tree.
//     /// @return The decoded element.
//     typename T _decodeRecursively(std::vector<std::byte> data, size_t index, std::unique_ptr<HuffmanNode<T>>& node) {
//         if (!node) {
//             throw std::runtime_error("Invalid code");
//         }
//
//         if (index >= data.size()) {
//             if (node->data.has_value()) {
//                 return node->data.value();
//             } else {
//                 throw std::runtime_error("Invalid data");
//             }
//         }
//
//         if (data[index] == std::byte{0}) {
//             return _decodeRecursively(data, index + 1, node->left);
//
//         } else {
//             return _decodeRecursively(data, index + 1, node->right);
//         }
//     }
//     std::unique_ptr<HuffmanNode<T>> _root;
//     std::unordered_map<T, std::vector<std::byte>> _codeMap;
//     std::unordered_map<T, size_t> _elementToFrequencyMap;
// };
// }  // namespace f9ay
