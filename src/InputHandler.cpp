#include "InputHandler.hpp"
#include "GameLogic.hpp"
#include "ChessUtils.hpp" // Added for toChessNotation, pieceTypeToString
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp> // For sending JSON message
using json = nlohmann::json;

// toChessNotation and pieceTypeToString are now in ChessUtils.cpp, so remove them from here.

void handleMouseClick(
    const sf::Vector2i& mousePos,
    GameState& currentGameState,
    // sf::RectangleShape& whiteStartButton, // Removed
    // sf::Text& whiteStartText,          // Removed
    sf::Sprite& startButtonSprite,       // Added
    sf::RectangleShape& blackStartButton,
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
    /*if (currentGameState == GameState::ChoosingPlayer) {
        if (startButtonSprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            // Send a ready message to the server instead of directly changing state
            json readyMsg;
            readyMsg["type"] = "ready";
            // Optionally, client can express a preference, or server decides
            // readyMsg["preferred_color"] = "any";
            std::string msgStr = readyMsg.dump() + "\n";
            boost::asio::async_write(socket, boost::asio::buffer(msgStr),
                [](boost::system::error_code /*ec#1#, std::size_t /*length#1#) {
                    // Handle write completion or error if necessary
                });
            gameMessageStr = "Ready signal sent. Waiting for server...";
            // currentTurn = PieceColor::White; // Server will assign color and turn
            // currentGameState = GameState::Playing; // Server will change game state
            // frameClock.restart(); // Server message will trigger game start
        }
        // else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
        //     // This button is now invisible and zero-size, effectively disabled.
        //     // If it were to be re-enabled, it should also send a server message.
        //     // currentTurn = PieceColor::Black;
        //     // currentGameState = GameState::Playing;
        //     // gameMessageStr = "Black to move";
        //     // frameClock.restart();
        // }*/
    if (currentGameState == GameState::ChoosingPlayer) {
        if (startButtonSprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            // 로컬에서 즉시 게임 상태 변경
            currentGameState = GameState::Playing;
            currentTurn = PieceColor::White; // 로컬 시작 시 기본적으로 백이 먼저 시작하도록 설정
            gameMessageStr = "White to move";   // 또는 myColor에 따라 메시지 설정 가능
            // 예: if (myColor == PieceColor::White || myColor == PieceColor::None) { gameMessageStr = "White to move"; }
            //     else { gameMessageStr = "Black to move"; currentTurn = PieceColor::Black; }
            // 하지만 간단하게 백 시작으로 고정합니다.
            frameClock.restart(); // 타이머 및 다음 프레임 처리를 위해 시계 재시작
        }
    } else if (currentGameState == GameState::Playing) {
        if (currentTurn != myColor || myColor == PieceColor::None) { // Also check if myColor is assigned
             if (myColor == PieceColor::None) {
                gameMessageStr = "Waiting for color assignment...";
            } else if (currentTurn != myColor) {
                 // gameMessageStr = "Opponent's turn."; // This message is better handled by GameStateUpdater
            }
            return;
        }

        int clickedCol = mousePos.x / TILE_SIZE;
        int clickedRow = mousePos.y / TILE_SIZE;

        if (clickedCol >= 0 && clickedCol < 8 && clickedRow >= 0 && clickedRow < 8) { // Click is on board
            bool moved = false;
            int fromR_local = -1, fromC_local = -1;

            if (selectedPiecePos.has_value()) {
                fromR_local = selectedPiecePos->y;
                fromC_local = selectedPiecePos->x;
                for (const auto& move_coord : possibleMoves) {
                    if (move_coord.x == clickedCol && move_coord.y == clickedRow) {
                        // Temporarily apply move to check for self-check
                        auto tempBoard = board_state;
                        auto pieceToMoveOpt = tempBoard[fromR_local][fromC_local];
                        
                        if (pieceToMoveOpt.has_value()) { // Should always be true if selectedPiecePos is valid
                            // Create a new piece for the temporary board
                            tempBoard[clickedRow][clickedCol] = Piece(pieceToMoveOpt->type, pieceToMoveOpt->color, pieceToMoveOpt->sprite);
                            tempBoard[fromR_local][fromC_local].reset();
                        }

                        if (!isKingInCheck(tempBoard, currentTurn)) { // currentTurn is myColor here
                            // Move is legal, apply to actual board and send to server
                            auto actualPieceToMoveOpt = board_state[fromR_local][fromC_local];
                            if (actualPieceToMoveOpt.has_value()) {
                                // For the actual board, also create a new piece or ensure sprite is handled correctly
                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                board_state[fromR_local][fromC_local].reset();

                                // Update sprite position for the moved piece on the actual board
                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                // Scale should be preserved from original piece
                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds(); // Get bounds after potential implicit copy
                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                movedSprite.setPosition({clickedCol * static_cast<float>(TILE_SIZE) + x_offset, clickedRow * static_cast<float>(TILE_SIZE) + y_offset});
                                
                                // Send move to server
                                json moveMsg;
                                moveMsg["type"] = "move";
                                moveMsg["from"] = toChessNotation(fromC_local, fromR_local);
                                moveMsg["to"] = toChessNotation(clickedCol, clickedRow);
                                moveMsg["piece"] = pieceTypeToString(actualPieceToMoveOpt->type); // For server-side validation/logging
                                moveMsg["color"] = (actualPieceToMoveOpt->color == PieceColor::White) ? "white" : "black";
                                std::string msgStr = moveMsg.dump() + "\n";
                                
                                boost::asio::async_write(socket, boost::asio::buffer(msgStr),
                                    [](boost::system::error_code /*ec*/, std::size_t /*length*/) {
                                    // Handle write completion or error
                                });
                            }
                            
                            moved = true;
                            // currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White; // Server will dictate turn
                            // frameClock.restart(); // Server's turn message will restart clock
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
            } else { // No move was made, try to select/deselect a piece
                if (board_state[clickedRow][clickedCol].has_value() &&
                    board_state[clickedRow][clickedCol]->color == currentTurn) { // It's current player's piece
                    if (selectedPiecePos.has_value() &&
                        selectedPiecePos->x == clickedCol &&
                        selectedPiecePos->y == clickedRow) { // Clicked selected piece again
                        selectedPiecePos.reset(); // Deselect
                        possibleMoves.clear();
                    } else { // Selected a new piece (or different piece)
                        selectedPiecePos = sf::Vector2i(clickedCol, clickedRow);
                        auto raw_moves = getPossibleMoves(board_state, clickedRow, clickedCol);
                        
                        // Filter moves to exclude those that leave king in check
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
                } else { // Clicked on empty square or opponent's piece (and no move was made)
                    selectedPiecePos.reset(); // Deselect if any
                    possibleMoves.clear();
                }
            }
        } else { // Clicked outside board
            selectedPiecePos.reset();
            possibleMoves.clear();
        }

    } else if (currentGameState == GameState::GameOver) {
        if (homeButtonShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            actualResetGame(); // This resets to ChoosingPlayer locally
            // Optionally, send a message to server if it needs to know about "play again" intention
            // json playAgainMsg;
            // playAgainMsg["type"] = "play_again_request";
            // std::string msgStr = playAgainMsg.dump() + "\n";
            // boost::asio::async_write(socket, boost::asio::buffer(msgStr), [](boost::system::error_code, std::size_t){});
        }
    }
}