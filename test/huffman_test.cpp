#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>
#include <unordered_set>

#include "huffman_coding.hpp"

using namespace f9ay;

// Helper function to generate random strings
std::string generateRandomString(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()-_=+[]{}|;:,.<>?/";

    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

    std::string result(length, '\0');
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[dist(rng)];
    }

    return result;
}

// Helper function to encode a string using Huffman coding
std::vector<std::byte> encodeString(const std::string& input, HuffmanCoding<char>& huffman) {
    huffman.add(input);
    huffman.build();
    
    std::vector<std::byte> encoded;
    for (char c : input) {
        auto charCode = huffman.getMapping(c);
        encoded.insert(encoded.end(), charCode.begin(), charCode.end());
    }
    
    return encoded;
}

// Helper function to decode using Huffman coding
std::string decodeToString(const std::vector<std::byte>& encoded, HuffmanCoding<char>& huffman) {
    std::string result;
    size_t index = 0;
    
    while (index < encoded.size()) {
        char decodedChar = huffman.decode(std::vector<std::byte>(encoded.begin() + index, encoded.end()));
        result += decodedChar;
        
        // Find how many bytes were consumed for this character
        auto charCode = huffman.getMapping(decodedChar);
        index += charCode.size();
    }
    
    return result;
}

TEST(HuffmanTest, EncodeDecode) {
    // Run multiple test rounds
    const int TEST_ROUNDS = 10;

    for (int round = 0; round < TEST_ROUNDS; round++) {
        // Generate random strings of different lengths
        std::vector<size_t> lengths = {5, 10, 50, 100, 500, 1000};

        for (size_t length : lengths) {
            std::string original = generateRandomString(length);
            
            try {
                HuffmanCoding<char> huffman;
                
                // Build Huffman tree
                huffman.add(original);
                huffman.build();
                
                // Verify that all characters have codes
                std::unordered_set<char> uniqueChars(original.begin(), original.end());
                auto codeMap = huffman.getCodeMap();
                
                for (char c : uniqueChars) {
                    ASSERT_TRUE(codeMap.find(c) != codeMap.end())
                        << "Character '" << c << "' not found in code map"
                        << " Round: " << round << ", Length: " << length;
                }
                
                // Test encoding and decoding each character individually
                for (char c : uniqueChars) {
                    auto encoded = huffman.getMapping(c);
                    char decoded = huffman.decode(encoded);
                    
                    ASSERT_EQ(c, decoded)
                        << "Character encoding/decoding failed for '" << c << "'"
                        << " Round: " << round << ", Length: " << length;
                }
                
            } catch (const std::exception& e) {
                FAIL() << "Exception thrown: " << e.what()
                       << " Round: " << round << ", Length: " << length
                       << " Original: " << original;
            }
        }
    }
}

// Test with very specific patterns that might be challenging for Huffman coding
// TEST(HuffmanTest, SpecificPatterns) {
//     std::vector<std::string> patterns = {
//         // Repeated patterns
//         "AAAAAAAAAAAAAAAAAAAA",
//         "ABABABABABABABABABAB",
//         // Alternating patterns with repeats
//         "AAABBBAAABBBAAABBB",
//         // Random-like but with structure
//         "A1B2C3D4E5F6G7H8I9J0",
//         // Single character
//         "X",
//         // Two characters with different frequencies
//         "AAAAAB",
//         "AAABBBCCCDDDEEEFFFGGGHHHIIIJJJKKKLLLMMMNNNOOOPPPQQQRRR"
//     };

//     for (const auto& pattern : patterns) {
//         try {
//             HuffmanCoding<char> huffman;
            
//             if (pattern.empty()) {
//                 // Empty string should throw an exception or handle gracefully
//                 EXPECT_THROW(huffman.add(pattern), std::exception);
//                 continue;
//             }
            
//             // Build Huffman tree
//             huffman.add(pattern);
//             huffman.build();
            
//             // Test each unique character
//             std::unordered_set<char> uniqueChars(pattern.begin(), pattern.end());
            
//             for (char c : uniqueChars) {
//                 auto encoded = huffman.getMapping(c);
//                 char decoded = huffman.decode(encoded);
                
//                 ASSERT_EQ(c, decoded) << "Failed pattern: " << pattern 
//                                      << ", Character: '" << c << "'";
//             }
            
//             // Verify code map completeness
//             auto codeMap = huffman.getCodeMap();
//             ASSERT_EQ(codeMap.size(), uniqueChars.size())
//                 << "Code map size doesn't match unique characters count for pattern: " << pattern;
            
//         } catch (const std::exception& e) {
//             FAIL() << "Exception thrown for pattern '" << pattern << "': " << e.what();
//         }
//     }
// }

// // Test edge cases
// TEST(HuffmanTest, EdgeCases) {
//     // Test single character repeated
//     {
//         std::string singleChar = "AAAAAAAAAA";
//         HuffmanCoding<char> huffman;
        
//         huffman.add(singleChar);
//         huffman.build();
        
//         auto encoded = huffman.getMapping('A');
//         char decoded = huffman.decode(encoded);
        
//         ASSERT_EQ('A', decoded);
        
//         // For single character, code should be simple
//         ASSERT_FALSE(encoded.empty());
//     }
    
//     // Test two characters with equal frequency
//     {
//         std::string twoChars = "AABBCCDD";
//         HuffmanCoding<char> huffman;
        
//         huffman.add(twoChars);
//         huffman.build();
        
//         auto codeMap = huffman.getCodeMap();
        
//         // Should have codes for A, B, C, D
//         ASSERT_EQ(codeMap.size(), 4);
        
//         for (char c : {'A', 'B', 'C', 'D'}) {
//             ASSERT_TRUE(codeMap.find(c) != codeMap.end());
            
//             auto encoded = huffman.getMapping(c);
//             char decoded = huffman.decode(encoded);
//             ASSERT_EQ(c, decoded);
//         }
//     }
// }

// // Test frequency-based optimization
// TEST(HuffmanTest, FrequencyOptimization) {
//     // Create a string where 'A' appears much more frequently than other characters
//     std::string text = std::string(100, 'A') + std::string(10, 'B') + std::string(5, 'C') + std::string(1, 'D');
    
//     HuffmanCoding<char> huffman;
//     huffman.add(text);
//     huffman.build();
    
//     auto codeMap = huffman.getCodeMap();
    
//     // A should have the shortest code (highest frequency)
//     auto codeA = huffman.getMapping('A');
//     auto codeB = huffman.getMapping('B');
//     auto codeC = huffman.getMapping('C');
//     auto codeD = huffman.getMapping('D');
    
//     // Most frequent character should have shortest or equal length code
//     ASSERT_LE(codeA.size(), codeB.size());
//     ASSERT_LE(codeA.size(), codeC.size());
//     ASSERT_LE(codeA.size(), codeD.size());
    
//     // Verify all characters decode correctly
//     ASSERT_EQ('A', huffman.decode(codeA));
//     ASSERT_EQ('B', huffman.decode(codeB));
//     ASSERT_EQ('C', huffman.decode(codeC));
//     ASSERT_EQ('D', huffman.decode(codeD));
// }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}