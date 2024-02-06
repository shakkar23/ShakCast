#include <stdint.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ranges>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

enum class TileType {
    DoubleLetter,
    TripleLetter,
    DoubleWord,
    Ice,
    Normal
};

constexpr static size_t char_to_index(char c) {
    return c - 'a';
}

using Board = std::array<std::array<std::tuple<char, TileType, bool>, 5>, 5>;

using BitBoard = uint32_t;

constexpr inline static bool get(const BitBoard board, int x, int y) {
    return board & (1 << (x * 5 + y));
}

constexpr inline static void set(BitBoard& board, int x, int y) {
    board |= (1 << (x * 5 + y));
}

constexpr inline static void unset(BitBoard& board, int x, int y) {
    board &= ~(1 << (x * 5 + y));
}

using Path = std::vector<std::tuple<int, int, char>>;

constexpr static inline int char_to_points(char c) {
    int value{};
    switch (c) {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
            value = 1;
            break;
        case 'n':
        case 'r':
        case 's':
        case 't':
            value = 2;
            break;
        case 'd':
        case 'g':
        case 'l':
            value = 3;
            break;
        case 'b':
        case 'h':
        case 'm':
        case 'p':
        case 'u':
        case 'y':
            value = 4;
            break;
        case 'c':
        case 'f':
        case 'v':
        case 'w':
            value = 5;
            break;
        case 'k':
            value = 6;
            break;
        case 'j':
        case 'x':
            value = 7;
            break;
        case 'q':
        case 'z':
            value = 8;
            break;
        default:
            std::unreachable();
            break;
    }
    return value;
}

constexpr static inline int letter_type_to_mul(TileType tile_type) {
    switch (tile_type) {
        case TileType::DoubleLetter:
            return 2;
            break;
        case TileType::TripleLetter:
            return 3;
            break;
        default:
            return 1;
    }
    return 1;
}

constexpr static int get_max_score(const std::string_view key, bool has_double_word, TileType tile_type) {
    int upper_bound = 0;
    int max_letter_score = 0;

    for (char c : key) {
        upper_bound += char_to_points(c);
        max_letter_score = std::max(max_letter_score, char_to_points(c));
    }

    upper_bound += max_letter_score * (letter_type_to_mul(tile_type) - 1);
    upper_bound *= (has_double_word ? 2 : 1);
    upper_bound += (key.size() >= 6 ? 10 : 0);

    return upper_bound;
}

constexpr int N = 26;
class TrieNode {
   public:
    uint32_t bitfield = 0;
    uint8_t max_score = 0;
    bool isEndOfWord = false;
    std::array<std::unique_ptr<TrieNode>, N> children{};

    static void TrieInsert(TrieNode* x, const std::string_view key, const uint8_t max_score) {
        for (const char c : key) {
            x->max_score = std::max(x->max_score, max_score);

            size_t index = char_to_index(c);
            if (!(x->bitfield & (1 << index))) {
                x->bitfield |= (1 << index);
                x->children[index] = std::make_unique<TrieNode>();
            }

            x = x->children[index].get();
        }
        x->isEndOfWord = true;
        x->max_score = std::max(x->max_score, max_score);
    }
};

// old way of scoring
constexpr static int score(const Board& board, const Path& path) {
    int score = 0;
    int word_multiplier = 1;

    for (auto& [x, y, c] : path) {
        auto& [letter, tile_type, has_gem] = board[x][y];
        int letter_multiplier = 1;
        switch (tile_type) {
            case TileType::DoubleLetter:
                letter_multiplier = 2;
                break;
            case TileType::TripleLetter:
                letter_multiplier = 3;
                break;
            case TileType::DoubleWord:
                word_multiplier = 2;
                break;
            case TileType::Ice:
                break;
            case TileType::Normal:
                break;
        }
        score += char_to_points(c) * letter_multiplier;
    }
    score *= word_multiplier;

    if (path.size() >= 6) score += 10;

    return score;
}

constexpr static std::pair<std::array<std::pair<int, int>, 8>, int> get_neighbors(const Path& path, const BitBoard& bboard, int cur_x, int cur_y) {
    std::pair<std::array<std::pair<int, int>, 8>, int> neighbors;

    const std::array<std::pair<const int, const int>, 8> offsets = {{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}}};
    for (auto& [ox, oy] : offsets) {
        int x = ox + cur_x;
        int y = oy + cur_y;

        if (x < 0 || x >= 5 || y < 0 || y >= 5) {
            continue;
        }

        if (!get(bboard, x, y)) {
            neighbors.first[neighbors.second] = {x, y};
            neighbors.second++;
        }
    }

    return neighbors;
}

// made for debugging purposes
/*
static void print_path(Path& path, bool isEndOfWord) {
    for (auto [x, y, c] : path) {
        std::cout << std::format("({}, {}, {}), ", x, y, c);
    }
    if (isEndOfWord)
        std::cout << "END OF WORD";
    std::cout << std::endl;
}
*/

struct RecurseParams {
    BitBoard bboard;
    int current_word_points;
    int current_eco_points;
    int word_len;
    int swaps;
    bool has_word_mul;
    bool eco_mode;

    void update(const int x, const int y, const char c, const TileType tile_type, bool has_gem) {
        current_eco_points += has_gem ? 1 : 0;
        current_word_points += char_to_points(c) * letter_type_to_mul(tile_type);
        has_word_mul |= tile_type == TileType::DoubleWord;
        word_len++;
        set(bboard, x, y);
        swaps--;
    }
};

// optimized implementation, hard to read, will refactor later
static void recurse(const Board& board, Path& path, const RecurseParams params, int& max_eco_score, int& max_score, const TrieNode* node, std::vector<Path>& largest_word) {
    if (node->max_score <= max_score)
        return;

    // print_path(path, node->isEndOfWord);
    const auto [x, y, c] = path.back();

    const auto [neighbors, n_neighbors] = get_neighbors(path, params.bboard, x, y);

    for (const auto& [x1, y1] : neighbors | std::views::take(n_neighbors)) {
        const size_t reserved_index = char_to_index(std::get<0>(board[x1][y1]));
        TrieNode* next_node{};

        if (params.swaps > 0) {
            for (size_t i = 0; i < N; ++i) {
                if (!(node->bitfield & (1 << i)) || i == reserved_index)
                    continue;

                RecurseParams params_copy = params;
                params_copy.update(x1, y1, i + 'a', std::get<1>(board[x1][y1]), std::get<2>(board[x1][y1]));
                next_node = node->children.at(i).get();

                path.emplace_back(x1, y1, i + 'a');
                recurse(board, path, params_copy, max_eco_score, max_score, next_node, largest_word);
                path.pop_back();
            }
        }

        if (node->bitfield & (1 << reserved_index)) {
            RecurseParams params_copy = params;
            params_copy.update(x1, y1, std::get<0>(board[x1][y1]), std::get<1>(board[x1][y1]), std::get<2>(board[x1][y1]));
            params_copy.swaps++;
            next_node = node->children.at(reserved_index).get();

            path.emplace_back(x1, y1, std::get<0>(board[x1][y1]));
            recurse(board, path, params_copy, max_eco_score, max_score, next_node, largest_word);
            path.pop_back();
        }
    }

    if (node->isEndOfWord) {
        const int eco_score = params.current_eco_points;
        const int our_score = params.current_word_points * (params.has_word_mul ? 2 : 1) + (params.word_len >= 6 ? 10 : 0);
        if (params.eco_mode) {
            if (eco_score > max_eco_score) {
                largest_word.clear();
                largest_word.emplace_back(path);
                max_score = our_score;
                max_eco_score = eco_score;
            } else if (eco_score == max_eco_score) {
                if (our_score > max_score) {
                    largest_word.clear();
                    largest_word.emplace_back(path);
                    max_score = our_score;
                    max_eco_score = eco_score;
                } else if (our_score == max_score) {
                    largest_word.emplace_back(path);
                }
            }
        }

        else if (our_score > max_score) {
            largest_word.clear();
            largest_word.emplace_back(path);
            max_score = our_score;
            max_eco_score = eco_score;
        } else if (our_score == max_score) {
            largest_word.emplace_back(path);
        }
    }
}

// default word, should be overridden by recurse, also should be refactored out
std::vector<Path> biggest_words = {{{{0, 0, 'e'}}}};

static void recurse_swapless(const Board& board, Path& path, BitBoard bboard, TrieNode* node) {
    // print_path(path, node->isEndOfWord);
    auto [x, y, c] = path.back();

    const auto [neighbors, n_neighbors] = get_neighbors(path, bboard, x, y);

    for (auto& next_cell : neighbors | std::views::take(n_neighbors)) {
        auto [x1, y1] = next_cell;

        size_t index = char_to_index(std::get<0>(board[x1][y1]));

        if (node->bitfield & (1 << index)) {
            path.emplace_back(x1, y1, std::get<0>(board[x1][y1]));
            BitBoard bb_copy = bboard;
            set(bboard, x1, y1);
            recurse_swapless(board, path, bboard, node->children.at(index).get());
            bboard = bb_copy;
            path.pop_back();
        }
    }
    // see if we are at the end of a word
    if (node->isEndOfWord) {
        int max_score = score(board, biggest_words[0]);
        int our_score = score(board, path);

        if (our_score > max_score) {
            biggest_words.clear();
            biggest_words.emplace_back(path);
        } else if (our_score == max_score) {
            biggest_words.emplace_back(path);
        }
    }
}

Board parse_board_from_file() {
    std::ifstream board_file("board.txt");

    Board board{};
    int i = 0;
    std::string line;
    while (std::getline(board_file, line)) {
        auto letters = std::views::split(line, ' ');
        auto v = std::views::zip(letters, std::views::iota(0)) | std::views::take(5);

        for (auto [letter, e] : v) {
            TileType tile_type = TileType::Normal;
            bool has_gem = false;
            for (int c = 1; c < letter.size(); ++c) {
                switch (letter[c]) {
                    case 'l':
                        tile_type = TileType::DoubleLetter;
                        break;
                    case 't':
                        tile_type = TileType::TripleLetter;
                        break;
                    case 'w':
                        tile_type = TileType::DoubleWord;
                        break;
                    case 'i':
                        tile_type = TileType::Ice;
                        break;
                    case 'g':
                        has_gem = true;
                        break;
                    default:
                        std::unreachable();
                }
            }
            board[i][e] = {letter[0], tile_type, has_gem};
        }
        i++;
    }

    return board;
}

constexpr static Board parse_board_from_string(const std::string& s_board) {
    Board board{};
    auto lines = std::views::split(s_board, '\n');
    auto v = std::views::zip(lines, std::views::iota(0)) | std::views::take(5) | std::views::transform([](auto pair) {
                 auto [line, i] = pair;
                 return std::pair{line, i};
             });

    for (auto [line, i] : v) {
        auto letters = std::views::split(line, ' ');
        auto v = std::views::zip(letters, std::views::iota(0)) | std::views::take(5) | std::views::transform([](auto pair) {
                     auto [letter, i] = pair;
                     return std::pair{letter, i};
                 });

        for (auto [letter, e] : v) {
            TileType tile_type = TileType::Normal;
            bool has_gem = false;
            for (int i = 1; i < letter.size(); ++i)
                switch (letter[i]) {
                    case 'l':
                        tile_type = TileType::DoubleLetter;
                        break;
                    case 't':
                        tile_type = TileType::TripleLetter;
                        break;
                    case 'w':
                        tile_type = TileType::DoubleWord;
                        break;
                    case 'i':
                        tile_type = TileType::Ice;
                        break;
                    case 'g':
                        has_gem = true;
                    default:
                        std::unreachable();
                        break;
                }
            board[i][e] = {letter[0], tile_type, has_gem};
        }
    }

    return board;
}

BitBoard board_to_bitboard(const Board& board) {
    BitBoard bboard = 0;

    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; j++)
            if (std::get<1>(board[i][j]) == TileType::Ice)
                set(bboard, i, j);
    return bboard;
}

void print_board(const Board& board) {
    std::cout << "board: " << std::endl;
    for (auto& row : board) {
        for (auto [letter, tile_type, has_gem] : row) {
            std::cout << letter << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_biggest_word(const Board& board) {
    for (auto& words : std::views::reverse(biggest_words) | std::views::take(10)) {
        std::cout << "board: " << std::endl;
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; j++) {
                auto it = std::ranges::find_if(words, [i, j](const auto& path) {
                    auto [x, y, c] = path;
                    return x == i && y == j;
                });
                const char c = std::get<0>(board[i][j]);

                if (it != words.end()) {
                    {
                        char it_c = std::get<2>(*it);
                        bool first = it == words.begin();
                        if (it_c != c)
                        // make the brackets red
                        {
                            std::cout << std::format("\033[1;31m[\033[0m");

                            if (first)
                                // color this green
                                std::cout << std::format("\033[1;32m{}\033[0m", it_c);
                            else
                                std::cout << std::format("\033[1;31m{}\033[0m", it_c);

                            std::cout << std::format("\033[1;31m]\033[0m");
                        } else
                            // make the text white
                            if (first)
                                // color this green
                                std::cout << std::format("\033[1;32m[{}]\033[0m", it_c);
                            else
                                std::cout << std::format("\033[1;37m[{}]\033[0m", it_c);
                    }
                } else
                    std::cout << std::format(" {} ", c);
            }
            std::cout << std::endl;
            std::cout << std::endl;
        }
        std::cout << "word found: ";
        for (auto [x, y, c] : words)
            std::cout << c;
        std::cout << std::endl;

        for (auto [x, y, c] : words)
            std::cout << std::format("({}, {}, {}), ", x, y, c);
        std::cout << std::endl;

        std::cout << "score: " << score(board, words) << std::endl;
        std::cout << std::endl;
    }
}

std::pair<bool, TileType> get_mods(const Board& board) {
    TileType max_letter_mod = TileType::Normal;
    bool has_word_mod = false;
    for (auto& row : board)
        for (auto& [letter, tile_type, has_gem] : row)
            if (tile_type == TileType::DoubleLetter)
                max_letter_mod = max_letter_mod == TileType::Normal ? TileType::DoubleLetter : max_letter_mod;
            else if (tile_type == TileType::TripleLetter)
                max_letter_mod = TileType::TripleLetter;
            else if (tile_type == TileType::DoubleWord)
                has_word_mod = true;

    return {has_word_mod, max_letter_mod};
}

int main(int argc, char* argv[]) {
    std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();
    Board board = parse_board_from_file();
    auto [has_word_mod, max_letter_mod] = get_mods(board);

    std::ifstream file("wordlist.txt");
    std::ifstream file2("swaps.txt");
    std::ifstream file3("eco.txt");

    std::string word;
    while (file >> word) {
        int max_score = get_max_score(word, has_word_mod, max_letter_mod);
        // get max score based on word or something
        [[unlikely]] if (max_score > 255) {
            throw std::runtime_error("max score is too big");
            std::unreachable();
        }

        TrieNode::TrieInsert(root.get(), word, max_score);
    }

    const int swaps = std::stoi(std::string{std::istreambuf_iterator<char>(file2), std::istreambuf_iterator<char>()});
    bool eco_mode = std::stoi(std::string{std::istreambuf_iterator<char>(file3), std::istreambuf_iterator<char>()});

    auto start = std::chrono::high_resolution_clock::now();
    int max_eco_score = 0;
    int max_score = 0;
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            if (swaps > 0)
                for (int a = 'a'; a <= 'z'; ++a) {
                    Path path = {{i, j, a}};
                    RecurseParams params{};
                    params.update(i, j, a, std::get<1>(board[i][j]), std::get<2>(board[i][j]));

                    int swaps_left = swaps;
                    if (a != std::get<0>(board[i][j]))
                        swaps_left--;
                    params.swaps = swaps_left;
                    params.eco_mode = eco_mode;

                    path.reserve(25);
                    recurse(board, path, params, max_eco_score, max_score, root.get()->children.at(char_to_index(a)).get(), biggest_words);
                }
            else {
                Path path = {{i, j, std::get<0>(board[i][j])}};
                RecurseParams params{};
                params.update(i, j, std::get<0>(board[i][j]), std::get<1>(board[i][j]), std::get<2>(board[i][j]));
                params.swaps = 0;
                params.eco_mode = eco_mode;

                path.reserve(25);
                recurse(board, path, params, max_eco_score, max_score, root.get()->children.at(char_to_index(std::get<0>(board[i][j]))).get(), biggest_words);
            }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::format("elapsed time: {}s", elapsed.count() * 1e-6) << std::endl;

    print_biggest_word(board);
    std::cout << "max score: " << max_score << std::endl;
    std::cout << "max eco score: " << max_eco_score << std::endl;
    return 0;
}
