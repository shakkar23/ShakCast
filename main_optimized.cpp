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
#include <utility>
#include <vector>

enum class TileType {
    DoubleLetter,
    TripleLetter,
    DoubleWord,
    Normal
};

constexpr static size_t char_to_index(char c) {
    return c - 'a';
}
constexpr int N = 26;
class TrieNode {
   public:
    uint32_t bitfield = 0;
    bool isEndOfWord = false;
    std::array<std::unique_ptr<TrieNode>, N> children{};

    static void TrieInsert(TrieNode* x, std::string_view key) {
        for (char c : key) {
            size_t index = char_to_index(c);
            if (!(x->bitfield & (1 << index))) {
                x->bitfield |= (1 << index);
                x->children[index] = std::make_unique<TrieNode>();
            }

            x = x->children[index].get();
        }
        x->isEndOfWord = true;
    }
};

using Board = std::array<std::array<std::pair<char, TileType>, 5>, 5>;

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

constexpr static int score(const Board& board, const Path& path) {
    int score = 0;
    int word_multiplier = 1;

    for (auto& [x, y, c] : path) {
        auto& [letter, tile_type] = board[x][y];
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

static void print_path(Path& path, bool isEndOfWord) {
    for (auto [x, y, c] : path) {
        std::cout << std::format("({}, {}, {}), ", x, y, c);
    }
    if (isEndOfWord)
        std::cout << "END OF WORD";
    std::cout << std::endl;
}

static void recurse(Board& board, Path& path, const BitBoard bboard, const int word_points, const bool has_word_mul, const int word_len, int& max_score, TrieNode* node, const int swaps, std::vector<Path>& largest_word) {
    // print_path(path, node->isEndOfWord);
    const auto [x, y, c] = path.back();

    const auto [neighbors, n_neighbors] = get_neighbors(path, bboard, x, y);

    for (const auto& [x1, y1] : neighbors | std::views::take(n_neighbors)) {
        const size_t reserved_index = char_to_index(board[x1][y1].first);
        TrieNode* next_node{};

        if (swaps > 0) {
            for (size_t i = 0; i < N; ++i) {
                if (!(node->bitfield & (1 << i)) || i == reserved_index)
                    continue;

                BitBoard bb_copy = bboard;
                int next_letter_points = word_points + char_to_points(i + 'a') * letter_type_to_mul(board[x1][y1].second);
                bool next_has_word_mul = has_word_mul || board[x1][y1].second == TileType::DoubleWord;

                next_node = node->children.at(i).get();
                set(bb_copy, x1, y1);

                path.emplace_back(x1, y1, i + 'a');
                recurse(board, path, bb_copy, next_letter_points, next_has_word_mul, word_len + 1, max_score, next_node, swaps - 1, largest_word);
                path.pop_back();
            }
        }

        if (node->bitfield & (1 << reserved_index)) {
            BitBoard bb_copy = bboard;
            int next_letter_points = word_points + char_to_points(reserved_index + 'a') * letter_type_to_mul(board[x1][y1].second);
            bool next_has_word_mul = has_word_mul || board[x1][y1].second == TileType::DoubleWord;

            set(bb_copy, x1, y1);
            next_node = node->children.at(reserved_index).get();

            path.emplace_back(x1, y1, board[x1][y1].first);
            recurse(board, path, bb_copy, next_letter_points, next_has_word_mul, word_len + 1, max_score, next_node, swaps, largest_word);
            path.pop_back();
        }
    }

    if (node->isEndOfWord) {
        const int our_score = word_points * (has_word_mul ? 2 : 1) + (word_len >= 6 ? 10 : 0);

        if (our_score > max_score) {
            largest_word.clear();
            largest_word.emplace_back(path);
            max_score = our_score;
        } else if (our_score == max_score) {
            largest_word.emplace_back(path);
        }
    }
}

// default word, should be overridden by recurse
std::vector<Path> biggest_words = {{{{0, 0, 'e'}}}};

static void recurse_swapless(const Board& board, Path& path, BitBoard bboard, TrieNode* node) {
    // print_path(path, node->isEndOfWord);
    auto [x, y, c] = path.back();

    const auto [neighbors, n_neighbors] = get_neighbors(path, bboard, x, y);

    for (auto& next_cell : neighbors | std::views::take(n_neighbors)) {
        auto [x1, y1] = next_cell;

        size_t index = char_to_index(board[x1][y1].first);

        if (node->bitfield & (1 << index)) {
            path.emplace_back(x1, y1, board[x1][y1].first);
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

Board parse_board() {
    std::ifstream board_file("board.txt");

    Board board{};
    int i = 0;
    std::string line;
    while (std::getline(board_file, line)) {
        auto letters = std::views::split(line, ' ');
        auto v = std::views::zip(letters, std::views::iota(0)) | std::views::take(5) | std::views::transform([](auto pair) {
                     auto [letter, i] = pair;
                     return std::pair{letter, i};
                 });

        for (auto [letter, e] : v) {
            TileType tile_type = TileType::Normal;
            if (letter.size() > 1)
                switch (letter[1]) {
                    case 'l':
                        tile_type = TileType::DoubleLetter;
                        break;
                    case 't':
                        tile_type = TileType::TripleLetter;
                        break;
                    case 'w':
                        tile_type = TileType::DoubleWord;
                        break;
                    default:
                        std::unreachable();
                        break;
                }
            board[i][e] = {letter[0], tile_type};
        }
        i++;
    }

    return board;
}

void print_board(const Board& board) {
    std::cout << "board: " << std::endl;
    for (auto& row : board) {
        for (auto [letter, tile_type] : row) {
            std::cout << letter << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_biggest_word(const Board& board) {
    for (auto& words : std::views::reverse(biggest_words)) {
        std::cout << "board: " << std::endl;
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; j++) {
                auto it = std::ranges::find_if(words, [i, j](const auto& path) {
                    auto [x, y, c] = path;
                    return x == i && y == j;
                });
                char c = board[i][j].first;

                if (it != words.end()) {
                    {
                        char it_c = std::get<2>(*it);
                        bool first = it == words.begin();
                        if (it_c != board[i][j].first)
                        // make the brackets red
                        {
                            std::cout << std::format("\033[1;31m[\033[0m", c);

                            if (first)
                                // color this green
                                std::cout << std::format("\033[1;32m{}\033[0m", c);
                            else
                                std::cout << std::format("\033[1;31m{}\033[0m", c);

                            std::cout << std::format("\033[1;31m]\033[0m", c);
                        } else
                            // make the text white
                            if (first)
                                // color this green
                                std::cout << std::format("\033[1;32m[{}]\033[0m", c);
                            else
                                std::cout << std::format("\033[1;37m[{}]\033[0m", c);
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

int main(int argc, char* argv[]) {
    std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();
    Board board = parse_board();
    // print_board(board);

    std::ifstream file("wordlist.txt");
    std::ifstream file2("swaps.txt");

    std::string word;
    while (file >> word)
        TrieNode::TrieInsert(root.get(), word);

    const int swaps = std::stoi(std::string{std::istreambuf_iterator<char>(file2), std::istreambuf_iterator<char>()});

    auto start = std::chrono::high_resolution_clock::now();
    int max_score = 0;
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            if (swaps > 0)
                for (int a = 'a'; a <= 'z'; ++a) {
                    Path path = {{i, j, a}};
                    BitBoard bboard = 0;
                    set(bboard, i, j);
                    path.reserve(25);
                    int swaps_left = swaps;
                    if (a != board[i][j].first)
                        swaps_left--;

                    int next_letter_points = char_to_points(a) * letter_type_to_mul(board[i][j].second);
                    bool next_has_word_mul = board[i][j].second == TileType::DoubleWord;

                    recurse(board, path, bboard, next_letter_points, next_has_word_mul, 1, max_score, root.get()->children.at(char_to_index(a)).get(), swaps_left, biggest_words);
                }
            else {
                Path path = {{i, j, board[i][j].first}};
                BitBoard bboard = 0;
                set(bboard, i, j);
                recurse_swapless(board, path, bboard, root.get()->children.at(char_to_index(board[i][j].first)).get());
            }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::format("elapsed time: {}s", elapsed.count() * 1e-6) << std::endl;

    // print_biggest_word(board);
    std::cout << biggest_words.size() << std::endl;
    return 0;
}