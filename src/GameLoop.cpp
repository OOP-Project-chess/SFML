#include "GameLoop.hpp"
#include "GameLogic.hpp" // isKingInCheck, findKing, isCheckmate, getPossibleMoves 사용

#include <iostream> // for std::cerr, std::cout
#include <iomanip>  // For std::setfill, std::setw
#include <sstream>  // For std::ostringstream

// formatTime 함수 정의
std::string formatTime(sf::Time time) {
    int totalSeconds = static_cast<int>(time.asSeconds());
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

// 게임 루프 함수 정의
void gameLoop(
    sf::RenderWindow& window,
    sf::Font& font,
    sf::RectangleShape& tile,
    sf::Color& lightColor,
    sf::Color& darkColor,
    sf::Color& checkedKingTileColor,
    sf::Text& chooseSidePromptText,
    sf::Text& messageText,
    sf::Text& whiteTimerText,
    sf::Text& blackTimerText,
    sf::RectangleShape& whiteStartButton,
    sf::Text& whiteStartText,
    sf::RectangleShape& blackStartButton,
    sf::Text& blackStartText,
    sf::RectangleShape& popupBackground,
    sf::Text& popupMessageText,
    sf::RectangleShape& homeButtonShape,
    sf::Text& homeButtonText,
    GameState& currentGameState,
    std::optional<sf::Vector2i>& selectedPiecePos,
    std::vector<sf::Vector2i>& possibleMoves,
    PieceColor& currentTurn,
    std::string& gameMessageStr,
    std::map<std::string, sf::Texture>& textures, // place_piece 람다 때문에 main에서 textures를 캡처하고,
                                                 // actualResetGame이 textures를 사용하므로 gameLoop에는 직접 필요 없을 수 있으나,
                                                 // 혹시 모를 확장성을 위해 전달 (현재는 actualResetGame이 main의 textures를 캡처)
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Time& whiteTimeLeft,
    sf::Time& blackTimeLeft,
    sf::Clock& frameClock,
    std::function<void()> actualResetGame,
    float timerPadding
) {
    // --- 메인 게임 루프 ---
    while (window.isOpen()) {
        sf::Time deltaTime = frameClock.restart();
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        // 게임 상태 업데이트 (시간, 체크/메이트, 메시지)
        if (currentGameState == GameState::Playing && currentTurn != PieceColor::None) {
            if (currentTurn == PieceColor::White) {
                if (whiteTimeLeft > sf::Time::Zero) whiteTimeLeft -= deltaTime;
                if (whiteTimeLeft <= sf::Time::Zero) { whiteTimeLeft = sf::Time::Zero; currentGameState = GameState::GameOver; gameMessageStr = "Black wins on time!"; }
            } else if (currentTurn == PieceColor::Black) {
                if (blackTimeLeft > sf::Time::Zero) blackTimeLeft -= deltaTime;
                if (blackTimeLeft <= sf::Time::Zero) { blackTimeLeft = sf::Time::Zero; currentGameState = GameState::GameOver; gameMessageStr = "White wins on time!"; }
            }
            if (currentGameState != GameState::GameOver) {
                 kingIsCurrentlyChecked = isKingInCheck(board_state, currentTurn);
                if (kingIsCurrentlyChecked) {
                    checkedKingCurrentPos = findKing(board_state, currentTurn);
                    if (isCheckmate(board_state, currentTurn)) {
                        currentGameState = GameState::GameOver; gameMessageStr = std::string((currentTurn == PieceColor::White) ? "Black" : "White") + " wins by Checkmate!";
                    } else { gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " King is in Check!"; }
                } else { gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " to move"; }
            }
        } else if (currentGameState == GameState::ChoosingPlayer) {
            gameMessageStr = "";
        }

        // 타이머 텍스트 업데이트
        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));

        sf::FloatRect wt_bounds_loop = whiteTimerText.getLocalBounds();
        whiteTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds_loop.size.x) / 2.f - wt_bounds_loop.position.x, timerPadding - wt_bounds_loop.position.y});
        sf::FloatRect bt_bounds_loop = blackTimerText.getLocalBounds();
        blackTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - bt_bounds_loop.size.x) / 2.f - bt_bounds_loop.position.x, whiteTimerText.getPosition().y + wt_bounds_loop.size.y + wt_bounds_loop.position.y + 5.f - bt_bounds_loop.position.y });

        // 이벤트 처리
        while (const auto event_opt = window.pollEvent()) {
            const sf::Event& event = *event_opt;
            if (event.is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            } else if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    sf::Vector2i mousePos = mouseButtonPressed->position;

                    if (currentGameState == GameState::ChoosingPlayer) {
                        if (whiteStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::White; currentGameState = GameState::Playing;
                            gameMessageStr = "White to move"; frameClock.restart();
                        } else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::Black; currentGameState = GameState::Playing;
                            gameMessageStr = "Black to move"; frameClock.restart();
                        }
                    } else if (currentGameState == GameState::Playing) {
                        int clickedCol = mousePos.x / TILE_SIZE; int clickedRow = mousePos.y / TILE_SIZE;
                        if (clickedCol >=0 && clickedCol < 8 && clickedRow >=0 && clickedRow < 8) {
                            bool moved = false;
                            int fromR_local = -1, fromC_local = -1;

                            if (selectedPiecePos.has_value()) {
                                fromR_local = selectedPiecePos->y;
                                fromC_local = selectedPiecePos->x;
                                for (const auto& move_coord : possibleMoves) {
                                    if (move_coord.x == clickedCol && move_coord.y == clickedRow) {
                                        std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board_state;
                                        std::optional<Piece> pieceToMoveOpt = tempBoard[fromR_local][fromC_local];
                                        if (pieceToMoveOpt.has_value()){
                                            tempBoard[clickedRow][clickedCol] = Piece(pieceToMoveOpt->type, pieceToMoveOpt->color, pieceToMoveOpt->sprite);
                                            tempBoard[fromR_local][fromC_local].reset();
                                        }
                                        if (!isKingInCheck(tempBoard, currentTurn)) {
                                            std::optional<Piece> actualPieceToMoveOpt = board_state[fromR_local][fromC_local];
                                            if(actualPieceToMoveOpt.has_value()){
                                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                                board_state[fromR_local][fromC_local].reset();
                                            }
                                            if (board_state[clickedRow][clickedCol].has_value()) {
                                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds();
                                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                                movedSprite.setPosition({clickedCol*static_cast<float>(TILE_SIZE)+x_offset, clickedRow*static_cast<float>(TILE_SIZE)+y_offset});
                                            }
                                            moved = true;
                                            currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                                            frameClock.restart();
                                            break;
                                        } else gameMessageStr = "Invalid move: King would be in check!";
                                    }
                                }
                            }
                            if (moved) {
                                selectedPiecePos.reset(); possibleMoves.clear();
                            } else {
                                if (board_state[clickedRow][clickedCol].has_value() && board_state[clickedRow][clickedCol]->color == currentTurn) {
                                    if (selectedPiecePos.has_value() && selectedPiecePos->x == clickedCol && selectedPiecePos->y == clickedRow) {
                                        selectedPiecePos.reset(); possibleMoves.clear();
                                    } else {
                                        selectedPiecePos = sf::Vector2i(clickedCol, clickedRow);
                                        possibleMoves = getPossibleMoves(board_state, clickedRow, clickedCol);

                                        std::vector<sf::Vector2i> valid_moves;
                                        for(const auto& p_move : possibleMoves){
                                            std::array<std::array<std::optional<Piece>, 8>, 8> temp_board_check = board_state;
                                            std::optional<Piece> piece_to_sim = temp_board_check[clickedRow][clickedCol];
                                            if(piece_to_sim.has_value()){
                                                temp_board_check[p_move.y][p_move.x] = Piece(piece_to_sim->type, piece_to_sim->color, piece_to_sim->sprite);
                                                temp_board_check[clickedRow][clickedCol].reset();
                                                if(!isKingInCheck(temp_board_check, currentTurn)){
                                                    valid_moves.push_back(p_move);
                                                }
                                            }
                                        }
                                        possibleMoves = valid_moves;
                                    }
                                } else {
                                    selectedPiecePos.reset(); possibleMoves.clear();
                                }
                            }
                        } else { selectedPiecePos.reset(); possibleMoves.clear(); }
                    } else if (currentGameState == GameState::GameOver) {
                         if (homeButtonShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            actualResetGame();
                         }
                    }
                }
            }
        }

        // 그리기
        window.clear(sf::Color(240,240,240));

        if (currentGameState == GameState::ChoosingPlayer) {
            window.draw(chooseSidePromptText);
            window.draw(whiteStartButton); window.draw(whiteStartText);
            window.draw(blackStartButton); window.draw(blackStartText);
        } else { // Playing or GameOver
            for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
                bool isLight = (r + c) % 2 == 0;
                tile.setFillColor(isLight ? lightColor : darkColor);
                if (kingIsCurrentlyChecked && checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r) tile.setFillColor(checkedKingTileColor);
                else if (selectedPiecePos.has_value() && selectedPiecePos->x == c && selectedPiecePos->y == r) tile.setFillColor(sf::Color(255, 255, 0, 150));
                else {
                    for (const auto& move_coord : possibleMoves) {
                        if (move_coord.x == c && move_coord.y == r) {
                            if (board_state[r][c].has_value() && board_state[r][c]->color != currentTurn) tile.setFillColor(sf::Color(50, 150, 250, 150));
                            else tile.setFillColor(sf::Color(100, 250, 50, 150));
                            break;
                        }
                    }
                }
                tile.setPosition({c * static_cast<float>(TILE_SIZE), r * static_cast<float>(TILE_SIZE)});
                window.draw(tile);
            }
            for (int r_idx = 0; r_idx < 8; ++r_idx) for (int c_idx = 0; c_idx < 8; ++c_idx) {
                if (board_state[r_idx][c_idx].has_value()) {
                    // SFML 렌더링 시 const가 아닌 스프라이트가 필요할 수 있으므로,
                    // board_state에서 Piece를 가져올 때 value()로 복사본을 사용하거나,
                    // GameLogic에 const가 아닌 접근자를 만들거나,
                    // UIManager가 board_state의 복사본을 가지도록 설계 변경 필요.
                    // 여기서는 가장 간단하게 복사본을 사용합니다.
                    Piece piece_to_draw = board_state[r_idx][c_idx].value();
                    bool isLosingKing = (currentGameState == GameState::GameOver && piece_to_draw.type == PieceType::King && piece_to_draw.color == currentTurn);
                    if (isLosingKing) piece_to_draw.sprite.setColor(sf::Color::Red);
                    else piece_to_draw.sprite.setColor(sf::Color::White); // 항상 기본 색상으로 리셋
                    window.draw(piece_to_draw.sprite);
                }
            }
            window.draw(whiteTimerText); window.draw(blackTimerText);
            if (!gameMessageStr.empty()) {
                messageText.setString(gameMessageStr);
                sf::FloatRect gameMsgLocalBounds = messageText.getLocalBounds();
                messageText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - gameMsgLocalBounds.size.x) / 2.f - gameMsgLocalBounds.position.x, (WINDOW_HEIGHT - gameMsgLocalBounds.size.y) / 2.f - gameMsgLocalBounds.position.y });
                messageText.setFillColor(sf::Color::Black); messageText.setStyle(sf::Text::Regular);
                if(currentGameState == GameState::GameOver || gameMessageStr.find("wins by") != std::string::npos ) { messageText.setFillColor(sf::Color::Red); messageText.setStyle(sf::Text::Bold); }
                else if (gameMessageStr.find("Check!") != std::string::npos ) { messageText.setFillColor(sf::Color(200,0,0)); messageText.setStyle(sf::Text::Bold); }
                window.draw(messageText);
            }

            if (currentGameState == GameState::GameOver) {
                window.draw(popupBackground);
                popupMessageText.setString(gameMessageStr);
                sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
                popupMessageText.setPosition({ popupBackground.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x, popupBackground.getPosition().y - popupBackground.getSize().y / 2.f + 30.f - popupMsgBounds.position.y });
                window.draw(popupMessageText);
                homeButtonShape.setPosition({ popupBackground.getPosition().x, popupBackground.getPosition().y + popupBackground.getSize().y / 2.f - 40.f - homeButtonShape.getSize().y / 2.f });
                window.draw(homeButtonShape);
                sf::FloatRect homeButtonTextBounds = homeButtonText.getLocalBounds(); // popupMessageText의 바운드를 사용하던 것을 수정
                homeButtonText.setPosition({ homeButtonShape.getPosition().x - homeButtonTextBounds.size.x / 2.f - homeButtonTextBounds.position.x, homeButtonShape.getPosition().y - homeButtonTextBounds.size.y / 2.f - homeButtonTextBounds.position.y });
                window.draw(homeButtonText);
            }
        }
        window.display();
    }
}
