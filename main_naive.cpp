#include <stdint.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <execution>
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
    std::array<std::unique_ptr<TrieNode>, N> children{};
    bool isEndOfWord = false;

    static void TrieInsert(TrieNode *x, std::string_view key) {
        for (char c : key) {
            size_t index = char_to_index(c);
            if (!x->children[index]) {
                x->children[index] = std::make_unique<TrieNode>();
            }

            x = x->children[index].get();
        }
        x->isEndOfWord = true;
    }
};

using Board = std::array<std::array<std::pair<char, TileType>, 5>, 5>;

using Path = std::vector<std::tuple<int, int, char>>;

constexpr static int char_to_points(char c) {
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

constexpr static int score(const Board &board, const Path &path) {
    int score = 0;
    int word_multiplier = 1;

    for (auto [x, y, c] : path) {
        auto [letter, tile_type] = board[x][y];
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

constexpr static std::vector<std::pair<int, int>> get_neighbors(const Path &path, int cur_x, int cur_y) {
    std::vector<std::pair<int, int>> neighbors;
    neighbors.reserve(8);

    std::array<std::pair<int, int>, 8> offsets = {{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}}};
    for (auto [x, y] : offsets) {
        x += cur_x;
        y += cur_y;
        if (x < 0 || x >= 5 || y < 0 || y >= 5) {
            continue;
        }
        auto comp = [x, y](const auto &param) {
            auto [lx, ly, lc] = param;
            return lx == x && ly == y;
        };

        if (std::ranges::find_if(path, comp) == path.end()) {
            neighbors.emplace_back(x, y);
        }
    }

    return neighbors;
}

// default word, should be overridden by recurse

void print_path(Path &path, bool isEndOfWord) {
    for (auto [x, y, c] : path) {
        std::cout << std::format("({}, {}, {}), ", x, y, c);
    }
    if (isEndOfWord)
        std::cout << "END OF WORD";
    std::cout << std::endl;
}

static void recurse(const Board &board, Path &path, TrieNode *node, const int swaps, std::vector<Path> &largest_word) {
    // print_path(path, node->isEndOfWord);

    const auto [x, y, c] = *(path.end() - 1);

    for (const auto &next_cell : get_neighbors(path, x, y)) {
        const auto [x1, y1] = next_cell;

        const size_t reserved_index = char_to_index(board[x1][y1].first);
        TrieNode *next_node{};

        if (swaps > 0)
            for (size_t i = 0; i < N; ++i) {
                if (i == reserved_index)
                    continue;

                next_node = node->children.at(i).get();

                if (next_node) {
                    path.emplace_back(x1, y1, i + 'a');
                    recurse(board, path, next_node, swaps - 1, largest_word);
                    path.pop_back();
                }
            }

        next_node = node->children.at(reserved_index).get();
        if (next_node) {
            path.emplace_back(x1, y1, board[x1][y1].first);
            recurse(board, path, next_node, swaps, largest_word);
            path.pop_back();
        }
    }
    // see if we are at the end of a word
    if (node->isEndOfWord) {
        const int max_score = score(board, largest_word[0]);
        const int our_score = score(board, path);

        if (our_score > max_score) {
            largest_word.clear();
            largest_word.emplace_back(path);
        } else if (our_score == max_score) {
            largest_word.emplace_back(path);
        }
    }
}

static void recurse_swapless(const Board &board, Path &path, TrieNode *node, std::vector<Path> &biggest_words) {
    // print_path(path, node->isEndOfWord);

    auto [x, y, c] = *(path.end() - 1);

    for (auto &next_cell : get_neighbors(path, x, y)) {
        auto [x1, y1] = next_cell;

        path.emplace_back(x1, y1, board[x1][y1].first);

        TrieNode *next_node = node->children.at(char_to_index(board[x1][y1].first)).get();

        if (next_node)
            recurse_swapless(board, path, next_node, biggest_words);

        path.pop_back();
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

void print_board(const Board &board) {
    std::cout << "board: " << std::endl;
    for (auto &row : board) {
        for (auto [letter, tile_type] : row) {
            std::cout << letter << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void print_biggest_word(const Board &board, std::vector<Path> &biggest_words) {
    for (auto &words : std::views::reverse(biggest_words)) {
        std::cout << "board: " << std::endl;
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; j++) {
                auto it = std::ranges::find_if(words, [i, j](const auto &path) {
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

int main(int argc, char *argv[]) {
    auto start = std::chrono::high_resolution_clock::now();
    std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();
    Board board = parse_board();
    // print_board(board);

    std::ifstream file("wordlist.txt");
    std::ifstream file2("swaps.txt");

    std::string word;
    while (file >> word)
        TrieNode::TrieInsert(root.get(), word);

    const int swaps = std::stoi(std::string{std::istreambuf_iterator<char>(file2), std::istreambuf_iterator<char>()});

    std::vector<int> Xs(5);
    std::iota(Xs.begin(), Xs.end(), 0);
    std::vector<std::vector<Path>> biggest_words_local = {{{{{{0, 0, 'e'}}}}, {{{{0, 0, 'e'}}}}, {{{{0, 0, 'e'}}}}, {{{{0, 0, 'e'}}}}, {{{{0, 0, 'e'}}}}}};

    std::for_each(std::execution::par_unseq, Xs.begin(), Xs.end(), [swaps, &board, &root, &biggest_words_local](int &i) {
        std::vector<int> Ys(5);
        std::iota(Ys.begin(), Ys.end(), 0);
        std::for_each(std::execution::par_unseq, Ys.begin(), Ys.end(), [i, swaps, board, &root, &biggest_words_local](int &j) {
            if (swaps > 0)
                for (int a = 'a'; a <= 'z'; ++a) {
                    Path path = {{i, j, a}};
                    path.reserve(25);
                    
                    int swaps_left = swaps;
                    if (a != board[i][j].first)
                        swaps_left--;

                    recurse(board, path, root.get()->children.at(char_to_index(a)).get(), swaps_left, biggest_words_local[i]);
                }
            else {
                Path path = {{i, j, board[i][j].first}};
                recurse_swapless(board, path, root.get()->children.at(char_to_index(board[i][j].first)).get(), biggest_words_local[i]);
            }
        });
    });

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << std::format("elapsed time: {}s", elapsed.count()) << std::endl;

    std::vector<Path> biggest_words;

    int biggest_score = 0;
    for (auto &i : biggest_words_local) {
        for (auto &j : i) {
            int score = 0;
            for (auto [x, y, c] : j) {
                score += char_to_points(c);
            }
            if (score > biggest_score) {
                biggest_score = score;
                biggest_words.clear();
                biggest_words.emplace_back(j);
            } else if (score == biggest_score) {
                biggest_words.emplace_back(j);
            }
        }
    }

    print_biggest_word(board, biggest_words);
    return 0;
}