#include "InputHandler.hpp"
#include "GameLogic.hpp"
#include <boost/asio/write.hpp>

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

void handleMouseClick(
    const sf::Vector2i& mousePos,
    GameState& currentGameState,
    sf::RectangleShape& whiteStartButton,
    sf::RectangleShape& blackStartButton,
    sf::Text& whiteStartText,
    sf::Text& blackStartText,
    sf::Clock& frameClock,
    PieceColor& currentTurn,
    std::string& gameMessageStr,
    std::optional<sf::Vector2i>& selectedPiecePos,
    std::vector<sf::Vector2i>& possibleMoves,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::RectangleShape& homeButtonShape,
    std::function<void()> actualResetGame,
    boost::asio::ip::tcp::socket& socket,
    PieceColor myColor

) {
    if (currentGameState == GameState::ChoosingPlayer) {
        if (whiteStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            currentTurn = PieceColor::White;
            currentGameState = GameState::Playing;
            gameMessageStr = "White to move";
            frameClock.restart();
        } else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            currentTurn = PieceColor::Black;
            currentGameState = GameState::Playing;
            gameMessageStr = "Black to move";
            frameClock.restart();
        }
    } else if (currentGameState == GameState::Playing) {
        if (currentTurn != myColor) return;

        int clickedCol = mousePos.x / TILE_SIZE;
        int clickedRow = mousePos.y / TILE_SIZE;

        if (clickedCol >= 0 && clickedCol < 8 && clickedRow >= 0 && clickedRow < 8) {
            bool moved = false;
            int fromR_local = -1, fromC_local = -1;

            if (selectedPiecePos.has_value()) {
                fromR_local = selectedPiecePos->y;
                fromC_local = selectedPiecePos->x;
                for (const auto& move_coord : possibleMoves) {
                    if (move_coord.x == clickedCol && move_coord.y == clickedRow) {
                        auto tempBoard = board_state;
                        auto pieceToMoveOpt = tempBoard[fromR_local][fromC_local];
                        if (pieceToMoveOpt.has_value()) {
                            tempBoard[clickedRow][clickedCol] = Piece(pieceToMoveOpt->type, pieceToMoveOpt->color, pieceToMoveOpt->sprite);
                            tempBoard[fromR_local][fromC_local].reset();
                        }
                        if (!isKingInCheck(tempBoard, currentTurn)) {
                            auto actualPieceToMoveOpt = board_state[fromR_local][fromC_local];
                            if (actualPieceToMoveOpt.has_value()) {
                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                board_state[fromR_local][fromC_local].reset();

                                std::string from = toChessNotation(fromC_local, fromR_local);
                                std::string to = toChessNotation(clickedCol, clickedRow);
                                std::string piece = pieceTypeToString(actualPieceToMoveOpt->type);
                                std::string color = (actualPieceToMoveOpt->color == PieceColor::White) ? "white" : "black";

                                std::string message = R"({"type":"move","from":")" + from +
                                                      R"(","to":")" + to +
                                                      R"(","piece":")" + piece +
                                                      R"(","color":")" + color + R"("})";

                                boost::asio::write(socket, boost::asio::buffer(message + "\n"));
                            }

                            if (board_state[clickedRow][clickedCol].has_value()) {
                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds();
                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                movedSprite.setPosition({clickedCol * TILE_SIZE + x_offset, clickedRow * TILE_SIZE + y_offset});
                            }

                            moved = true;
                            currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                            frameClock.restart();
                            break;
                        } else {
                            gameMessageStr = "Invalid move: King would be in check!";
                        }
                    }
                }
            }

            if (moved) {
                selectedPiecePos.reset();
                possibleMoves.clear();
            } else {
                if (board_state[clickedRow][clickedCol].has_value() &&
                    board_state[clickedRow][clickedCol]->color == currentTurn) {
                    if (selectedPiecePos.has_value() &&
                        selectedPiecePos->x == clickedCol &&
                        selectedPiecePos->y == clickedRow) {
                        selectedPiecePos.reset();
                        possibleMoves.clear();
                    } else {
                        selectedPiecePos = sf::Vector2i(clickedCol, clickedRow);
                        possibleMoves = getPossibleMoves(board_state, clickedRow, clickedCol);

                        std::vector<sf::Vector2i> valid_moves;
                        for (const auto& p_move : possibleMoves) {
                            auto temp_board_check = board_state;
                            auto piece_to_sim = temp_board_check[clickedRow][clickedCol];
                            if (piece_to_sim.has_value()) {
                                temp_board_check[p_move.y][p_move.x] = Piece(piece_to_sim->type, piece_to_sim->color, piece_to_sim->sprite);
                                temp_board_check[clickedRow][clickedCol].reset();
                                if (!isKingInCheck(temp_board_check, currentTurn)) {
                                    valid_moves.push_back(p_move);
                                }
                            }
                        }
                        possibleMoves = valid_moves;
                    }
                } else {
                    selectedPiecePos.reset();
                    possibleMoves.clear();
                }
            }
        } else {
            selectedPiecePos.reset();
            possibleMoves.clear();
        }
    } else if (currentGameState == GameState::GameOver) {
        if (homeButtonShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            actualResetGame();
        }
    }
}