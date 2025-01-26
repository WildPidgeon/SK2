#include "common.h"
#include <iostream>
#include <algorithm>

Board::Board() {
    grid = std::vector<std::vector<std::string>>(BOARD_SIZE, std::vector<std::string>(BOARD_SIZE, EMPTY));
}

bool Board::isValidPlacement(Coordinate start, Coordinate end) {
    if (start.row < 0 || start.row >= BOARD_SIZE || start.col < 0 || start.col >= BOARD_SIZE ||
        end.row < 0 || end.row >= BOARD_SIZE || end.col < 0 || end.col >= BOARD_SIZE)
        return false;

    int rowDelta = abs(end.row - start.row);
    int colDelta = abs(end.col - start.col);
    if (rowDelta != 0 && colDelta != 0) return false; 

    for (int i = std::min(start.row, end.row); i <= std::max(start.row, end.row); ++i) {
        for (int j = std::min(start.col, end.col); j <= std::max(start.col, end.col); ++j) {
            for (int x = i - 1; x <= i + 1; ++x) {
                for (int y = j - 1; y <= j + 1; ++y) {
                    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                        if (grid[x][y] != EMPTY) return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Board::placeShip(Coordinate start, Coordinate end, std::string shipId) {
    if (!isValidPlacement(start, end)) return false;

    Ship ship;
    for (int i = std::min(start.row, end.row); i <= std::max(start.row, end.row); ++i) {
        for (int j = std::min(start.col, end.col); j <= std::max(start.col, end.col); ++j) {
            grid[i][j] = SHIP;
            ship.coordinates.emplace_back(i, j);
        }
    }
    ships[shipId] = ship;
    return true;
}

bool Board::attack(Coordinate coord) {
    if (coord.row < 0 || coord.row >= BOARD_SIZE || coord.col < 0 || coord.col >= BOARD_SIZE)
        return false;

    if (grid[coord.row][coord.col] == SHIP) {
        grid[coord.row][coord.col] = HIT;

        for (auto& pair : ships) {
            for (auto& c : pair.second.coordinates) {
                if (c.row == coord.row && c.col == coord.col) {
                    pair.second.isSunk = std::all_of(pair.second.coordinates.begin(),
                                                     pair.second.coordinates.end(),
                                                     [this](const Coordinate& c) {
                                                         return grid[c.row][c.col] == HIT;
                                                     });
                    break;
                }
            }
        }

        return true;
    } else if (grid[coord.row][coord.col] == EMPTY) {
        grid[coord.row][coord.col] = MISS;
        return false;
    }

    return false;
}

bool Board::allShipsSunk() {
    for (const auto& pair : ships) {
        if (!pair.second.isSunk) return false;
    }
    return true;
}

void Board::print(bool hideShips) {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (hideShips && grid[i][j] == SHIP)
                std::cout << EMPTY << " ";
            else
                std::cout << grid[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

std::vector<std::string> splitMessage(const std::string& message, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(message);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        tokens.push_back(item);
    }
    return tokens;
}
