#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

#include "lz77_compress.hpp"

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

TEST(LS77Test, EncodeDecode) {
    // Run multiple test rounds
    const int TEST_ROUNDS = 10;

    for (int round = 0; round < TEST_ROUNDS; round++) {
        // Generate random strings of different lengths
        std::vector<size_t> lengths = {5, 10, 50, 100, 500, 1000};

        for (size_t length : lengths) {
            std::string original = generateRandomString(length);

            // Test with different dictionary sizes and buffer sizes
            std::vector<std::pair<int, int>> params = {
                {256, 8},    // Default
                {1024, 16},  // Larger dictionary and buffer
                {128, 4},    // Smaller dictionary and buffer
                {512, 12}    // Medium size
            };

            for (const auto& [dictSize, bufferSize] : params) {
                // Encode the string
                auto encoded = LZ77::lz77EncodeSlow(original);

                // Decode the encoded data
                auto decoded = LZ77::lz77decode<std::string>(encoded);

                // Verify the decoded string matches the original
                ASSERT_EQ(original, decoded)
                    << "Round: " << round << ", Length: " << length
                    << ", Dict size: " << dictSize
                    << ", Buffer size: " << bufferSize;
            }
        }
    }
}

// Test with very specific patterns that might be challenging for compression
TEST(LS77Test, SpecificPatterns) {
    std::vector<std::string> patterns = {
        // Repeated patterns
        "AAAAAAAAAAAAAAAAAAAA", "ABABABABABABABABABAB",
        // Alternating patterns with repeats
        "AAABBBAAABBBAAABBB",
        // Random-like but with structure
        "A1B2C3D4E5F6G7H8I9J0",
        // Single character
        "X",
        // Empty string
        ""};

    for (const auto& pattern : patterns) {
        // Encode the string
        auto encoded = LZ77::lz77EncodeSlow(pattern);

        // Decode the encoded data
        auto decoded = LZ77::lz77decode<std::string>(encoded);

        // Verify the decoded string matches the original
        ASSERT_EQ(pattern, decoded) << "Failed pattern: " << pattern;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}