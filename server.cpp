#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <cctype>
#include <map>

std::mutex gameMutex;

std::string displayDualBoards(const Board& playerBoard, const Board& attackingBoard) {
    std::string result = "  Player's Board                Attacking Board\n";
    result += "  A B C D E F G H I J         A B C D E F G H I J\n";

    for (int i = 0; i < BOARD_SIZE; ++i) {
        result += std::to_string(i + 1);
        if (i + 1 < 10) result += " ";
        for (int j = 0; j < BOARD_SIZE; ++j) {
            result += playerBoard.grid[i][j] + " ";
        }

        result += "      ";

        result += std::to_string(i + 1);
        if (i + 1 < 10) result += " ";
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (attackingBoard.grid[i][j] == SHIP) {
                result += EMPTY + " ";
            } else {
                result += attackingBoard.grid[i][j] + " ";
            }
        }

        result += "\n";
    }

    return result;
}

int letterToColumn(const std::string& letter) {
    char c = toupper(letter[0]);
    if (c >= 'A' && c <= 'J') {
        return c - 'A';
    }
    return -1;
}

void handleGame(int player1, int player2) {
    Board board1, board2;
    Board attackingBoard1, attackingBoard2;
    bool player1Turn = true;
    bool gameOver = false;
    const int SHIP_LENGTHS[] = {5,4,3,3,2,2,1};

    auto sendMessage = [](int socket, const std::string& message) {
        send(socket, message.c_str(), message.length(), 0);
    };

    sendMessage(player1, "Set up your board. Place your ships.\n");
    sendMessage(player2, "Set up your board. Place your ships.\n");

    for (int i = 0; i < 2; i++) {
        int player = (i == 0) ? player1 : player2;
        Board& currentBoard = (i == 0) ? board1 : board2;

        for (size_t j = 0; j < sizeof(SHIP_LENGTHS) / sizeof(SHIP_LENGTHS[0]); ++j) {
            bool validPlacement = false;

            while (!validPlacement) {
                std::string boardDisplay = displayDualBoards(currentBoard, Board());
                sendMessage(player, boardDisplay);

                std::string prompt = "Place ship of length " + std::to_string(SHIP_LENGTHS[j]) +
                                     ". Enter start position (e.g., A,1,V for vertical): \n";
                sendMessage(player, prompt);

                char buffer[1024] = {0};
                ssize_t bytesRead = read(player, buffer, sizeof(buffer));
                if (bytesRead <= 0) {
                    std::cout << "Player disconnected during setup!" << std::endl;
                    close(player1);
                    close(player2);
                    return;
                }

                buffer[bytesRead] = '\0';
                std::string input(buffer);
                input.erase(input.find_last_not_of("\r\n") + 1);

                std::vector<std::string> tokens = splitMessage(input, ',');
                if (tokens.size() != 3) {
                    sendMessage(player, "Invalid input format. Use letter,row,direction (e.g., A,1,V).\n");
                    continue;
                }

                int col = letterToColumn(tokens[0]);
                int row = std::stoi(tokens[1]) - 1;
                std::string direction = tokens[2];

                if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE) {
                    sendMessage(player, "Invalid coordinates. Ensure row is 1-10 and column is A-J.\n");
                    continue;
                }

                int endRow = row;
                int endCol = col;

                if (direction == "H" || direction == "h") {
                    endCol = col + SHIP_LENGTHS[j] - 1;
                } else if (direction == "V" || direction == "v") {
                    endRow = row + SHIP_LENGTHS[j] - 1;
                } else {
                    sendMessage(player, "Invalid direction. Use H for horizontal or V for vertical.\n");
                    continue;
                }

                if (endRow >= BOARD_SIZE || endCol >= BOARD_SIZE) {
                    sendMessage(player, "Ship placement out of bounds. Try again.\n");
                    continue;
                }

                validPlacement = currentBoard.placeShip(Coordinate(row, col), Coordinate(endRow, endCol),
                                                        "Ship" + std::to_string(j));
                if (!validPlacement) {
                    sendMessage(player, "Invalid placement. Ships may overlap or be out of bounds. Try again.\n");
                }
            }
        }

        std::string finalBoardDisplay = displayDualBoards(currentBoard, Board());
        sendMessage(player, "Final board setup complete. Here's your board:\n" + finalBoardDisplay);
    }

    sendMessage(player1, "Game is starting! It's your turn.\n");
    sendMessage(player2, "Game is starting! Waiting for opponent's move...\n");

    while (!gameOver) {
        int activePlayer = player1Turn ? player1 : player2;
        int passivePlayer = player1Turn ? player2 : player1;
        Board& activeBoard = player1Turn ? board1 : board2;
        Board& passiveBoard = player1Turn ? board2 : board1;
        Board& activeAttackingBoard = player1Turn ? attackingBoard1 : attackingBoard2;
        Board& passiveAttackingBoard = player1Turn ? attackingBoard2 : attackingBoard1;

        sendMessage(activePlayer, "Your move. Enter attack coordinates (e.g., A,5):\n");
        sendMessage(passivePlayer, "Waiting for opponent's move...\n");

        char attackBuffer[1024] = {0};
        ssize_t bytesRead = read(activePlayer, attackBuffer, sizeof(attackBuffer));

        if (bytesRead <= 0) {
            std::cout << "Player disconnected!" << std::endl;
            close(activePlayer);
            close(passivePlayer);
            return;
        }

        attackBuffer[bytesRead] = '\0'; 
        std::string attack(attackBuffer);
        attack.erase(attack.find_last_not_of("\r\n") + 1); 

        std::vector<std::string> attackTokens = splitMessage(attack, ',');
        if (attackTokens.size() != 2) {
            sendMessage(activePlayer, "Invalid attack format. Use letter,row (e.g., A,5).\n");
            continue;
        }

        int col = letterToColumn(attackTokens[0]);
        int row = std::stoi(attackTokens[1]) - 1;

        if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE) {
            sendMessage(activePlayer, "Invalid coordinates. Ensure row is 1-10 and column is A-J.\n");
            continue;
        }

        bool hit = passiveBoard.attack(Coordinate(row, col));
        activeAttackingBoard.grid[row][col] = hit ? HIT : MISS;

        if (hit) {
            sendMessage(activePlayer, "Hit! You can attack again.\n");
            sendMessage(passivePlayer, "Your ship was hit!\n");

            for (const auto& shipPair : passiveBoard.ships) {
                const Ship& ship = shipPair.second;
                if (ship.isSunk) {
                    sendMessage(activePlayer, "The ship has been destroyed.\n");
                    break;
                }
            }
        } else {
            sendMessage(activePlayer, "Miss! Opponent's turn.\n");
            sendMessage(passivePlayer, "Opponent missed. Your turn.\n");
            player1Turn = !player1Turn;
        }

        if (board1.allShipsSunk() || board2.allShipsSunk()) {
            gameOver = true;
            const char* winMsg = "You won! All enemy ships sunk!\n";
            const char* loseMsg = "You lost! All your ships sunk!\n";
            if (board1.allShipsSunk()) {
                sendMessage(player1, loseMsg);
                sendMessage(player2, winMsg);
            } else {
                sendMessage(player1, winMsg);
                sendMessage(player2, loseMsg);
            }
        }

        std::string activeBoardDisplay = displayDualBoards(activeBoard, activeAttackingBoard);
        sendMessage(activePlayer, activeBoardDisplay);

        std::string passiveBoardDisplay = displayDualBoards(passiveBoard, passiveAttackingBoard);
        sendMessage(passivePlayer, passiveBoardDisplay);
    }

    close(player1);
    close(player2);
    std::cout << "Game over. Connections closed." << std::endl;
}

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t addrSize;
    std::vector<int> playerQueue;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(serverSocket, 10) != 0) {
        perror("Listen failed");
        exit(1);
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        addrSize = sizeof(clientAddr);
        newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);

        if (newSocket < 0) {
            perror("Server accept failed");
            exit(1);
        }

        std::cout << "New player connected!" << std::endl;
        playerQueue.push_back(newSocket);

        if (playerQueue.size() == 1) {
            const char* waitingMsg = "Waiting for another player to join...";
            send(newSocket, waitingMsg, strlen(waitingMsg), 0);
        }

        if (playerQueue.size() >= 2) {
            int player1 = playerQueue[0];
            int player2 = playerQueue[1];
            playerQueue.erase(playerQueue.begin(), playerQueue.begin() + 2);

            std::thread gameThread(handleGame, player1, player2);
            gameThread.detach();
        }
    }

    close(serverSocket);
    return 0;
}
