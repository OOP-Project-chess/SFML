#include "GameLogic.hpp"
// 필요한 다른 표준 라이브러리가 있다면 여기에 include (예: <iostream> for debugging)

std::vector<sf::Vector2i> getPossibleMoves(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, int row, int col) {
    std::vector<sf::Vector2i> moves;
    // 함수 시작 시 row, col 유효성 검사 및 piece 존재 여부 확인
    if (row < 0 || row >= 8 || col < 0 || col >= 8 || !board[row][col].has_value()) {
        return moves;
    }

    PieceType type = board[row][col]->type;
    PieceColor color = board[row][col]->color;

    auto addMovesInDirection = [&](int dr, int dc, bool canMoveMultipleSteps) {
        for (int i = 1; i < 8; ++i) {
            int targetRow = row + dr * i;
            int targetCol = col + dc * i;
            if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8) break;
            if (board[targetRow][targetCol].has_value()) {
                if (board[targetRow][targetCol]->color != color) moves.push_back({targetCol, targetRow});
                break;
            } else {
                moves.push_back({targetCol, targetRow});
            }
            if (!canMoveMultipleSteps) break;
        }
    };

    if (type == PieceType::Pawn) {
        int direction = (color == PieceColor::White) ? -1 : 1;
        int nextRow = row + direction;
        if (nextRow >= 0 && nextRow < 8 && !board[nextRow][col].has_value()) {
            moves.push_back({col, nextRow});
            bool isInitialPosition = (color == PieceColor::White && row == 6) || (color == PieceColor::Black && row == 1);
            if (isInitialPosition) {
                int twoStepsRow = row + 2 * direction;
                // 두 칸 전진 시 바로 앞 칸과 두 번째 칸 모두 비어있어야 함
                if (twoStepsRow >= 0 && twoStepsRow < 8 && !board[row + direction][col].has_value() && !board[twoStepsRow][col].has_value()){
                     moves.push_back({col, twoStepsRow});
                }
            }
        }
        int attackColLeft = col - 1; int attackRow = row + direction;
        if (attackColLeft >= 0 && attackRow >= 0 && attackRow < 8) {
            if (board[attackRow][attackColLeft].has_value() && board[attackRow][attackColLeft]->color != color) {
                moves.push_back({attackColLeft, attackRow});
            }
        }
        int attackColRight = col + 1;
        if (attackColRight < 8 && attackRow >= 0 && attackRow < 8) {
            if (board[attackRow][attackColRight].has_value() && board[attackRow][attackColRight]->color != color) {
                moves.push_back({attackColRight, attackRow});
            }
        }
    } else if (type == PieceType::Rook) {
        addMovesInDirection(1, 0, true); addMovesInDirection(-1, 0, true); addMovesInDirection(0, 1, true); addMovesInDirection(0, -1, true);
    } else if (type == PieceType::Bishop) {
        addMovesInDirection(1, 1, true); addMovesInDirection(1, -1, true); addMovesInDirection(-1, 1, true); addMovesInDirection(-1, -1, true);
    } else if (type == PieceType::Queen) {
        addMovesInDirection(1, 0, true); addMovesInDirection(-1, 0, true); addMovesInDirection(0, 1, true); addMovesInDirection(0, -1, true);
        addMovesInDirection(1, 1, true); addMovesInDirection(1, -1, true); addMovesInDirection(-1, 1, true); addMovesInDirection(-1, -1, true);
    } else if (type == PieceType::Knight) {
        std::vector<std::pair<int, int>> knightMoves = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for (const auto& move : knightMoves) {
            int targetRow = row + move.first; int targetCol = col + move.second;
            if (targetRow >= 0 && targetRow < 8 && targetCol >= 0 && targetCol < 8) {
                if (!board[targetRow][targetCol].has_value() || board[targetRow][targetCol]->color != color) {
                    moves.push_back({targetCol, targetRow});
                }
            }
        }
    } else if (type == PieceType::King) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr==0 && dc==0) continue;
                int targetRow = row + dr; int targetCol = col + dc;
                if (targetRow >=0 && targetRow < 8 && targetCol >=0 && targetCol < 8) {
                    if (!board[targetRow][targetCol].has_value() || board[targetRow][targetCol]->color != color) {
                        moves.push_back({targetCol, targetRow});
                    }
                }
            }
        }
    }
    return moves;
}

sf::Vector2i findKing(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor) {
    for (int r=0; r<8; ++r) {
        for (int c=0; c<8; ++c) {
            if (board[r][c].has_value() && board[r][c]->type == PieceType::King && board[r][c]->color == kingColor) {
                return {c, r};
            }
        }
    }
    return {-1,-1}; // 킹을 찾지 못한 경우
}

bool isKingInCheck(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor) {
    sf::Vector2i kingPos = findKing(board, kingColor);
    if (kingPos.x == -1) return false; // 킹이 없으면 체크도 아님

    PieceColor opponentColor = (kingColor == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    for (int r=0; r<8; ++r) {
        for (int c=0; c<8; ++c) {
            if (board[r][c].has_value() && board[r][c]->color == opponentColor) {
                std::vector<sf::Vector2i> opponentMoves = getPossibleMoves(board, r, c);
                for (const auto& move : opponentMoves) {
                    if (move.x == kingPos.x && move.y == kingPos.y) return true;
                }
            }
        }
    }
    return false;
}

bool isCheckmate(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor currentColor) {
    if (!isKingInCheck(board, currentColor)) return false;

    for (int r=0; r<8; ++r) {
        for (int c=0; c<8; ++c) {
            if (board[r][c].has_value() && board[r][c]->color == currentColor) {
                std::vector<sf::Vector2i> possibleMovesForPiece = getPossibleMoves(board, r, c);
                for (const auto& move : possibleMovesForPiece) {
                    std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board; // 임시 보드 복사
                    std::optional<Piece> originalPieceOpt = tempBoard[r][c];
                    if (originalPieceOpt.has_value()) {
                        // 스프라이트 복사 문제 해결을 위해 Piece 생성자 명시적 호출
                        tempBoard[move.y][move.x] = Piece(originalPieceOpt->type, originalPieceOpt->color, originalPieceOpt->sprite);
                        tempBoard[r][c].reset(); // 이전 위치 비우기
                        if (!isKingInCheck(tempBoard, currentColor)) {
                            return false; // 체크를 벗어날 수 있는 수가 있다면 체크메이트 아님
                        }
                    }
                }
            }
        }
    }
    return true; // 모든 수를 시도해도 체크를 벗어날 수 없으면 체크메이트
}
