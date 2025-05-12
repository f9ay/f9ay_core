#include <gtest/gtest.h>

#include <map>
#include <random>
#include <string>
#include <vector>

#include "huffman_coding.hpp"  // 請確保路徑正確 (建議與磁碟上的檔名大小寫一致，例如 "Huffman_coding.hpp")

// 輔助函式：產生隨機字串 (類似 ls77_test.cpp 中的函式)
std::string generateRandomString(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        " !@#$%^&*()-_=+[]{}|;:,.<>?/";  // 包含空格與更多符號

    static std::mt19937 rng(std::random_device{}());
    // 確保 charset 大小為正數再建立分布
    static std::uniform_int_distribution<size_t> dist(
        0, (sizeof(charset) > 1 ? sizeof(charset) - 2 : 0));

    std::string result(length, '\0');
    if (sizeof(charset) <= 1) {  // 若 charset 為空或只有 null 結束符則無法產生
        return result;
    }

    for (size_t i = 0; i < length; i++) {
        result[i] = charset[dist(rng)];
    }
    return result;
}

TEST(HuffmanCodingTest, EncodeDecodeIndividualChars) {
    const int TEST_ROUNDS = 20;  // 增加測試回合

    for (int round = 0; round < TEST_ROUNDS; ++round) {
        // 測試不同長度的隨機字串，包含 0 和 1
        std::vector<size_t> lengths = {0, 1, 2, 5, 10, 50, 100, 250};

        for (size_t length : lengths) {
            std::string original = generateRandomString(length);

            HuffmanCoding<std::string> coder;

            // 處理空字串的特殊情況
            if (original.empty()) {
                // 注意：目前的 huffman_coding.hpp 在 buildTree 時若輸入為空，
                // _recursiveBuildTree 會因 nodesPq 為空而拋出異常。
                // 為了使此測試通過，huffman_coding.hpp::buildTree
                // 應能優雅處理空輸入， 例如設定 _root = nullptr 且
                // _elementToFrequencyMap 為空。
                coder.buildTree(original);
                // encode() 現在回傳 std::unordered_map
                auto codes_map = coder.encode();
                // 若 buildTree 和 encode 正確處理空輸入，codes_map 應為空。
                ASSERT_TRUE(codes_map.empty())
                    << "Expected empty codes map for empty string. Round: "
                    << round;
                // getCodeMap() 也應在 _root 為 nullptr (空輸入造成) 時回傳空
                // map 或不拋出異常。
                auto codeMapFromGetter = coder.getCodeMap();
                ASSERT_TRUE(codeMapFromGetter.empty())
                    << "Expected empty code map from getCodeMap() for empty "
                       "string. Round: "
                    << round;
                continue;
            }

            coder.buildTree(original);
            // 呼叫 encode() 以產生
            // _codeMap，即使我們在這裡不直接使用其回傳值進行解碼。
            // huffman_coding.hpp::encode() 應確保在開始編碼前清除 _codeMap。
            coder.encode();

            auto codeMap = coder.getCodeMap();

            // 如果原始字串非空，但 codeMap 為空，這通常表示編碼邏輯有問題。
            // 即使是單一字元字串 (如 "AAA")，其字元也應存在於 codeMap 中，
            // 且其編碼可能是一個空的 std::vector<std::byte> (例如 'A' -> {})。
            // 因此，如果 original 非空，codeMap 不應為空。
            if (!original.empty()) {
                ASSERT_FALSE(codeMap.empty())
                    << "Code map is empty for a non-empty original string. "
                       "Round: "
                    << round << ", Length: " << length
                    << ", Original: " << original;
            }

            std::string decodedString = "";
            bool possibleToDecodeAllChars = true;
            for (char original_char : original) {
                auto it = codeMap.find(original_char);

                ASSERT_NE(it, codeMap.end())
                    << "Character '" << original_char << "' ("
                    << static_cast<int>(original_char)
                    << ") not found in code map."
                    << " Round: " << round << ", Length: " << length
                    << ", Original: '" << original << "'";

                if (it == codeMap.end()) {
                    possibleToDecodeAllChars = false;
                    break;
                }
                const std::vector<std::byte>& char_code = it->second;

                // huffman_coding.hpp::decode 應能處理空 vector<byte> 的情況
                // (單一字元編碼)
                char decoded_char = coder.decode(char_code);
                decodedString += decoded_char;
            }

            if (possibleToDecodeAllChars) {
                ASSERT_EQ(original, decodedString)
                    << "Decoded string does not match original."
                    << " Round: " << round << ", Length: " << length
                    << ", Original: '" << original << "'";
            }
        }
    }
}

// 使用特定的、可能具有挑戰性的模式進行測試
TEST(HuffmanCodingTest, SpecificPatterns) {
    std::vector<std::string> patterns = {
        "",                                            // 空字串
        "A",                                           // 單一字元
        "AAAAA",                                       // 重複的單一字元
        "ABABABAB",                                    // 兩個字元交替
        "AAABBC",                                      // 不同頻率
        "ABCDEFG",                                     // 所有字元唯一
        "The quick brown fox jumps over the lazy dog"  // 一個句子
    };

    for (const auto& pattern : patterns) {
        HuffmanCoding<std::string> coder;

        if (pattern.empty()) {
            // 同上，huffman_coding.hpp 需優雅處理空輸入
            coder.buildTree(pattern);
            auto codes_map =
                coder.encode();  // encode() 回傳 std::unordered_map
            ASSERT_TRUE(codes_map.empty())
                << "Expected empty codes map for empty pattern.";
            auto codeMapFromGetter = coder.getCodeMap();
            ASSERT_TRUE(codeMapFromGetter.empty())
                << "Expected empty code map from getCodeMap() for empty "
                   "pattern.";
            continue;
        }

        coder.buildTree(pattern);
        coder.encode();  // 產生內部 _codeMap

        auto codeMap = coder.getCodeMap();
        std::string decodedString = "";
        bool possible = true;

        // 同上，如果 pattern 非空，codeMap 不應為空
        if (!pattern.empty()) {
            ASSERT_FALSE(codeMap.empty())
                << "Code map is empty for a non-empty pattern: '" << pattern
                << "'";
        }

        for (char original_char : pattern) {
            auto it = codeMap.find(original_char);
            ASSERT_NE(it, codeMap.end())
                << "Character '" << original_char
                << "' not found in code map for pattern: '" << pattern << "'";
            if (it == codeMap.end()) {
                possible = false;
                break;
            }
            const std::vector<std::byte>& char_code = it->second;
            char decoded_char = coder.decode(char_code);
            decodedString += decoded_char;
        }

        if (possible) {
            ASSERT_EQ(pattern, decodedString)
                << "Decoded string does not match pattern: '" << pattern << "'";
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}