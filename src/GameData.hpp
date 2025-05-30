#ifndef GAMEDATA_HPP
#define GAMEDATA_HPP

#include <SFML/Graphics.hpp> // sf::Sprite, sf::Time 때문
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>

// 전역 상수 정의
const int TILE_SIZE = 100;
const int BOARD_WIDTH = 8 * TILE_SIZE;
const int BOARD_HEIGHT = 8 * TILE_SIZE;
const int BUTTON_PANEL_WIDTH = 300;
const int WINDOW_WIDTH = BOARD_WIDTH + BUTTON_PANEL_WIDTH;
const int WINDOW_HEIGHT = BOARD_HEIGHT;
const float INITIAL_TIME_SECONDS = 60.f;
// 서버 관련 상수는 네트워크 기능 추가 시 다시 포함 예정
// const std::string SERVER_IP = "10.2.3.147";
// const short SERVER_PORT = 1234;

// 열거형 정의
enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn, None };
enum class PieceColor { White, Black, None };
enum class GameState { ChoosingPlayer, Playing, GameOver };

// Piece 구조체 정의
struct Piece {
    PieceType type;
    PieceColor color;
    sf::Sprite sprite;

    Piece(PieceType t, PieceColor c, sf::Sprite s) : type(t), color(c), sprite(std::move(s)) {}
};

#endif // GAMEDATA_HPP
