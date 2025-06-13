#include "InputHandler.hpp"
#include "GameLogic.hpp"
#include "ChessUtils.hpp"
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void handleMouseClick(
    const sf::Vector2i& mousePos,
    GameState& currentGameState,
    sf::Sprite& startButtonSprite,
    sf::RectangleShape& blackStartButton,
    sf::Text& blackStartText,
    sf::Clock& frameClock,
    PieceColor& currentTurn,
    std::string& gameMessageStr,
    std::optional<sf::Vector2i>& selectedPiecePos,
    std::vector<sf::Vector2i>& possibleMoves,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Sprite& homeButtonSprite,
    std::function<void()> actualResetGame,
    boost::asio::ip::tcp::socket& socket,
    PieceColor myColor
) {
    if (currentGameState == GameState::ChoosingPlayer) {
        if (startButtonSprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            // --- 핫시트 모드 코드 (주석 처리) ---

            currentGameState = GameState::Playing;
            currentTurn = PieceColor::White;
            myColor = currentTurn;
            gameMessageStr = "White to move";
            frameClock.restart();


            // --- 원래 네트워크 모드 코드 (주석 해제) ---
            // json readyMsg;
            // readyMsg["type"] = "ready";
            // std::string msgStr = readyMsg.dump() + "\n";
            // boost::asio::async_write(socket, boost::asio::buffer(msgStr),
            //     [](boost::system::error_code /*ec*/, std::size_t /*length*/) {
            //         // 쓰기 완료 후 콜백 (필요시 에러 처리)
            //     });
            // gameMessageStr = "Ready signal sent. Waiting for server...";
        }
    } else if (currentGameState == GameState::Playing) {
        // --- 원래 네트워크 모드 입력 가드 (주석 해제) ---
        if (currentTurn != myColor || myColor == PieceColor::None) {
             if (myColor == PieceColor::None) {
                gameMessageStr = "Waiting for color assignment...";
            }
            // else if (currentTurn != myColor) { // 상대 턴 메시지는 GameStateUpdater에서 처리하므로 주석 유지 가능
            //     gameMessageStr = "Opponent's turn.";
            // }
            return;
        }

        // --- 핫시트 모드 입력 가드 (주석 처리) ---
        /*
        if (myColor == PieceColor::None) {
            gameMessageStr = "Color not assigned for hotseat test.";
            return;
        }
        */

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

                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds();
                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                movedSprite.setPosition({clickedCol * static_cast<float>(TILE_SIZE) + x_offset, clickedRow * static_cast<float>(TILE_SIZE) + y_offset});

                                // --- 원래 네트워크 모드: 서버로 이동 메시지 전송 (주석 해제) ---
                                json moveMsg;
                                moveMsg["type"] = "move";
                                moveMsg["from"] = toChessNotation(fromC_local, fromR_local);
                                moveMsg["to"] = toChessNotation(clickedCol, clickedRow);
                                std::string msgStr = moveMsg.dump() + "\n";
                                boost::asio::async_write(socket, boost::asio::buffer(msgStr),
                                    [](boost::system::error_code, std::size_t){
                                    });
                            }
                            moved = true;

                            // --- 핫시트 모드 턴 넘기기 (주석 처리) ---
                            /*
                            currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                            myColor = currentTurn;
                            gameMessageStr = (currentTurn == PieceColor::White ? "White" : "Black") + std::string(" to move (Hotseat)");
                            frameClock.restart();
                            */

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
                        auto raw_moves = getPossibleMoves(board_state, clickedRow, clickedCol);
                        possibleMoves.clear();
                        for (const auto& p_move : raw_moves) {
                            auto temp_board_check = board_state;
                            auto piece_to_sim_opt = temp_board_check[clickedRow][clickedCol];
                            if (piece_to_sim_opt.has_value()) {
                                temp_board_check[p_move.y][p_move.x] = Piece(piece_to_sim_opt->type, piece_to_sim_opt->color, piece_to_sim_opt->sprite);
                                temp_board_check[clickedRow][clickedCol].reset();
                                if (!isKingInCheck(temp_board_check, currentTurn)) {
                                    possibleMoves.push_back(p_move);
                                }
                            }
                        }
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
        if (homeButtonSprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            actualResetGame();
        }
    }
}
