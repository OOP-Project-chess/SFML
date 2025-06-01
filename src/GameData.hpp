#ifndef GAMEDATA_HPP
#define GAMEDATA_HPP

#include <SFML/Graphics.hpp>
#include <map>

// 전역 상수 정의
const int TILE_SIZE = 100;
const int BOARD_WIDTH = 8 * TILE_SIZE;
const int BOARD_HEIGHT = 8 * TILE_SIZE;
const int BUTTON_PANEL_WIDTH = 300;
const int WINDOW_WIDTH = BOARD_WIDTH + BUTTON_PANEL_WIDTH;
const int WINDOW_HEIGHT = BOARD_HEIGHT;
const float INITIAL_TIME_SECONDS = 600.f;

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
