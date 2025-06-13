#include "BoardRenderer.hpp"
#include "GameData.hpp"
#include <SFML/Graphics.hpp>

void drawBoardAndUI(
    sf::RenderWindow& window,
    sf::RectangleShape& tile,
    sf::Color& lightColor,
    sf::Color& darkColor,
    sf::Color& checkedKingTileColor,
    std::optional<sf::Vector2i>& selectedPiecePos,
    std::vector<sf::Vector2i>& possibleMoves,
    sf::Text& whiteTimerText,
    sf::Text& blackTimerText,
    sf::Text& messageText,
    std::string& gameMessageStr,
    sf::Text& popupMessageText,
    sf::Sprite& popupImageSprite,
    sf::Sprite& homeButtonSprite,
    GameState& currentGameState,
    PieceColor& currentTurn,
    sf::Vector2i checkedKingCurrentPos,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Sprite& startButtonSprite,
    [[maybe_unused]] sf::RectangleShape& blackStartButton,
    [[maybe_unused]] sf::Text& blackStartText,
    sf::Sprite& backgroundSprite,
    sf::Sprite& logoSprite,
    sf::Sprite& uiPanelBgSprite,
    sf::Sprite& player1Sprite,
    sf::Sprite& player2Sprite,
    sf::Text& player1NameText,
    sf::Text& player2NameText,
    sf::Texture& player1Texture,
    sf::Texture& player2Texture,
    sf::Texture& waitingTexture
) {
    window.clear(sf::Color::Black);

    if (currentGameState == GameState::ChoosingPlayer) {
        window.draw(backgroundSprite);
        window.draw(logoSprite);
        window.draw(startButtonSprite);
    }
    else {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                bool isLight = (r + c) % 2 == 0;
                tile.setFillColor(isLight ? lightColor : darkColor);
                if (checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r) {
                    tile.setFillColor(checkedKingTileColor);
                } else if (selectedPiecePos && selectedPiecePos->x == c && selectedPiecePos->y == r) {
                    tile.setFillColor(sf::Color(215, 244, 178));
                } else {
                    for (const auto& move : possibleMoves) {
                        if (move.x == c && move.y == r) {
                            if (board_state[r][c] && board_state[r][c]->color != currentTurn) {
                                tile.setFillColor(sf::Color(250, 101, 67));
                            } else {
                                tile.setFillColor(sf::Color(172, 224, 240));
                            }
                            break;
                        }
                    }
                }
                float lineThickness = 2.5f;
                tile.setOutlineThickness(-lineThickness);
                tile.setOutlineColor(sf::Color(30, 30, 30, 200));
                tile.setPosition({static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE)});
                window.draw(tile);
            }
        }

        window.draw(uiPanelBgSprite);

        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board_state[r][c].has_value()) {
                    Piece pieceToDraw = board_state[r][c].value();
                    bool isLosingKing = (currentGameState == GameState::GameOver &&
                                         pieceToDraw.type == PieceType::King &&
                                         pieceToDraw.color == currentTurn);
                    pieceToDraw.sprite.setColor(isLosingKing ? sf::Color(255, 0, 0, 200) : sf::Color::White);
                    window.draw(pieceToDraw.sprite);
                }
            }
        }

        // --- 턴에 따라 플레이어 이미지 텍스처 변경 ---
        if (currentTurn == PieceColor::White) {
            player1Sprite.setTexture(player1Texture, true);
            player2Sprite.setTexture(waitingTexture, true);
        } else if (currentTurn == PieceColor::Black) {
            player1Sprite.setTexture(waitingTexture, true);
            player2Sprite.setTexture(player2Texture, true);
        } else { // 게임 오버 등 다른 상태일 경우
            player1Sprite.setTexture(player1Texture, true);
            player2Sprite.setTexture(player2Texture, true);
        }

        float panelCenterX = BOARD_WIDTH + BUTTON_PANEL_WIDTH / 2.f;

        // Player 2 이미지 및 텍스트 위치 설정
        player2Sprite.setPosition({panelCenterX, WINDOW_HEIGHT / 3.f});
        window.draw(player2Sprite);
        player2NameText.setPosition({player2Sprite.getPosition().x, player2Sprite.getPosition().y + 30.f});
        window.draw(player2NameText);

        // Player 1 이미지 및 텍스트 위치 설정
        player1Sprite.setPosition({panelCenterX, WINDOW_HEIGHT * 2.f / 3.f});
        window.draw(player1Sprite);
        player1NameText.setPosition({player1Sprite.getPosition().x, player1Sprite.getPosition().y + 30.f});
        window.draw(player1NameText);

        // 타이머 위치 설정
        float verticalCenter = WINDOW_HEIGHT / 2.f;
        float timerSpacing = 15.f;
        sf::FloatRect whiteTimerBounds = whiteTimerText.getLocalBounds();
        // White Timer 위치를 아래로 변경
        whiteTimerText.setPosition({
            BOARD_WIDTH + (BUTTON_PANEL_WIDTH - whiteTimerBounds.size.x) / 2.f - whiteTimerBounds.position.x,
            verticalCenter + timerSpacing
        });

        sf::FloatRect blackTimerBounds = blackTimerText.getLocalBounds();
        // Black Timer 위치를 위로 변경
        blackTimerText.setPosition({
            BOARD_WIDTH + (BUTTON_PANEL_WIDTH - blackTimerBounds.size.x) / 2.f - blackTimerBounds.position.x,
            verticalCenter - blackTimerText.getGlobalBounds().size.y - timerSpacing
        });

        window.draw(whiteTimerText);
        window.draw(blackTimerText);

        // 중요 메시지 (Check!) 표시
        if (!gameMessageStr.empty()) {
            bool isTurnMessage = (gameMessageStr.find("to move") != std::string::npos);
            bool isGameOverMessage = (currentGameState == GameState::GameOver);

            if (!isTurnMessage && !isGameOverMessage) {
                messageText.setString(gameMessageStr);
                sf::FloatRect msgBounds = messageText.getLocalBounds();

                // ▼▼▼ "Check!" 메시지 위치 조건부 설정 ▼▼▼
                float messageY;
                float originalY = (WINDOW_HEIGHT / 2.f) - 100.f - (msgBounds.size.y / 2.f) - msgBounds.position.y;

                // 현재 턴이 White일 때 (즉, White King이 체크일 때) Y좌표를 300px 내립니다.
                if (currentTurn == PieceColor::White && gameMessageStr.find("Check!") != std::string::npos) {
                    messageY = originalY + 210.f;
                } else { // Black King이 체크이거나 다른 메시지일 때는 원래 위치
                    messageY = originalY;
                }

                messageText.setPosition({
                    BOARD_WIDTH + (BUTTON_PANEL_WIDTH - msgBounds.size.x) / 2.f - msgBounds.position.x,
                    messageY
                });
                // ▲▲▲ "Check!" 메시지 위치 조건부 설정 ▲▲▲

                if (gameMessageStr.find("Check!") != std::string::npos) {
                    messageText.setFillColor(sf::Color(200, 0, 0));
                    messageText.setStyle(sf::Text::Bold);
                }

                window.draw(messageText);
            }
        }

        // 게임 오버 팝업 표시
        if (currentGameState == GameState::GameOver) {
            window.draw(popupImageSprite);

            popupMessageText.setString(gameMessageStr);
            popupMessageText.setCharacterSize(41);
            popupMessageText.setFillColor(sf::Color(255, 245, 207));
            popupMessageText.setOutlineColor(sf::Color(88, 43, 10));
            popupMessageText.setOutlineThickness(6.f);
            popupMessageText.setStyle(sf::Text::Bold);
            sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
            popupMessageText.setPosition({
                popupImageSprite.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x,
                popupImageSprite.getPosition().y - 50.f
            });
            window.draw(popupMessageText);

            homeButtonSprite.setPosition({
                popupImageSprite.getPosition().x,
                popupImageSprite.getPosition().y + 80.f
            });
            window.draw(homeButtonSprite);
        }
    }

    window.display();
}
