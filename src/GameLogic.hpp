#ifndef GAMELOGIC_HPP
#define GAMELOGIC_HPP

#include "GameData.hpp" // Piece, PieceType, PieceColor 등 사용

// 함수 선언 (board_state를 const 참조로 받음)
std::vector<sf::Vector2i> getPossibleMoves(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, int row, int col);
sf::Vector2i findKing(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor);
bool isKingInCheck(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor);
bool isCheckmate(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor currentColor);

#endif // GAMELOGIC_HPP
