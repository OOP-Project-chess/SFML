#include "ChessUtils.hpp"

std::string toChessNotation(int col, int row) {
    char file = 'a' + col;
    char rank = '8' - row;
    return std::string{file} + rank;
}

std::string pieceTypeToString(PieceType type) {
    switch (type) {
        case PieceType::King: return "king";
        case PieceType::Queen: return "queen";
        case PieceType::Rook: return "rook";
        case PieceType::Bishop: return "bishop";
        case PieceType::Knight: return "knight";
        case PieceType::Pawn: return "pawn";
        default: return "unknown";
    }
}