#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <iostream>
#include <sstream>

const int BOARD_SIZE = 10;
const int PORT = 12345;
const std::string EMPTY = ".";
const std::string SHIP = "O";
const std::string HIT = "X";
const std::string MISS = "~";

struct Coordinate {
    int row;
    int col;

    Coordinate(int r, int c) : row(r), col(c) {}
};

struct Ship {
    std::vector<Coordinate> coordinates;
    bool isSunk = false;
};

class Board {
public:
    std::vector<std::vector<std::string>> grid;
    std::map<std::string, Ship> ships;

    Board();
    bool placeShip(Coordinate start, Coordinate end, std::string shipId);
    bool isValidPlacement(Coordinate start, Coordinate end);
    bool attack(Coordinate coord);
    bool allShipsSunk();
    void print(bool hideShips = false);
};

std::vector<std::string> splitMessage(const std::string& message, char delimiter);

#endif
