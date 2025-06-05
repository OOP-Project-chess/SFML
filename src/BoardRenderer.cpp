#include "BoardRenderer.hpp"
#include "GameData.hpp" // TILE_SIZE, BOARD_WIDTH, BUTTON_PANEL_WIDTH, WINDOW_HEIGHT 등 사용
#include <SFML/Graphics.hpp> // 여기서도 SFML 타입들을 사용하므로 필요

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
    sf::RectangleShape& popupBackground,
    sf::RectangleShape& homeButtonShape,
    sf::Text& homeButtonText,
    GameState& currentGameState,
    PieceColor& currentTurn,
    sf::Vector2i checkedKingCurrentPos,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Sprite& startButtonSprite,
    [[maybe_unused]] sf::RectangleShape& blackStartButton, // 명시적으로 사용 안 함을 표시 (주석 처리된 부분)
    [[maybe_unused]] sf::Text& blackStartText,          // 명시적으로 사용 안 함을 표시 (주석 처리된 부분)
    sf::Sprite& backgroundSprite,
    sf::Sprite& logoSprite,
    sf::Sprite& uiPanelBgSprite
) {
    window.clear(sf::Color::Black); // 매 프레임 시작 시 화면을 특정 색으로 지웁니다 (예: 검은색)

    if (currentGameState == GameState::ChoosingPlayer) {
        window.draw(backgroundSprite); // 홈 화면 배경 이미지 그리기
        window.draw(logoSprite);       // 로고 이미지 그리기
        window.draw(startButtonSprite);// 시작 버튼 이미지 그리기
        // blackStartButton 과 blackStartText 는 main.cpp 에서 투명하게 설정했으므로 여기서는 그리지 않아도 됩니다.
    }
    else { // GameState::Playing or GameState::GameOver
        // 1. 체스 보드 타일 그리기
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                bool isLight = (r + c) % 2 == 0;
                tile.setFillColor(isLight ? lightColor : darkColor); // 기본 타일 색상

                if (checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r) {
                    tile.setFillColor(checkedKingTileColor);
                } else if (selectedPiecePos && selectedPiecePos->x == c && selectedPiecePos->y == r) {
                    tile.setFillColor(sf::Color(255, 255, 0, 150)); // 선택된 말 노란색
                } else {
                    for (const auto& move : possibleMoves) {
                        if (move.x == c && move.y == r) {
                            if (board_state[r][c] && board_state[r][c]->color != currentTurn) { // 공격
                                tile.setFillColor(sf::Color(250, 50, 50, 150)); // 공격 가능 경로
                            } else { // 빈 곳으로 이동
                                tile.setFillColor(sf::Color(100, 200, 50, 150)); // 이동 가능 경로
                            }
                            break;
                        }
                    }
                }

                // 타일 외곽선 추가
                float lineThickness = 2.5f;
                tile.setOutlineThickness(-lineThickness);
                tile.setOutlineColor(sf::Color(30, 30, 30, 200)); // 선 색상 (어두운 회색 계열)

                tile.setPosition({static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE)});
                window.draw(tile);
            }
        }

        // 2. UI 패널 배경 그리기 (보드 오른쪽)
        window.draw(uiPanelBgSprite);

        // 3. 체스 말 그리기
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board_state[r][c].has_value()) {
                    Piece pieceToDraw = board_state[r][c].value(); // 값 복사로 안전하게 색상 변경

                    bool isLosingKing = (currentGameState == GameState::GameOver &&
                                         pieceToDraw.type == PieceType::King &&
                                         pieceToDraw.color == currentTurn); // currentTurn이 패배한 플레이어

                    pieceToDraw.sprite.setColor(isLosingKing ? sf::Color(255, 0, 0, 200) : sf::Color::White); // 패배한 킹은 빨갛게
                    window.draw(pieceToDraw.sprite);
                }
            }
        }

        // 4. UI 패널 위의 요소들(타이머, 메시지 등) 그리기
        window.draw(whiteTimerText);
        window.draw(blackTimerText);

        if (!gameMessageStr.empty()) {
            messageText.setString(gameMessageStr);
            sf::FloatRect msgBounds = messageText.getLocalBounds();
            messageText.setPosition({
                BOARD_WIDTH + (BUTTON_PANEL_WIDTH - msgBounds.size.x) / 2.f - msgBounds.position.x,
                (WINDOW_HEIGHT - msgBounds.size.y) / 2.f - msgBounds.position.y // UI 패널 중앙에 메시지
            });

            // 메시지 종류에 따른 스타일링
            messageText.setFillColor(sf::Color::Black); // 기본값
            messageText.setStyle(sf::Text::Regular);   // 기본값

            if (currentGameState == GameState::GameOver || gameMessageStr.find("wins by") != std::string::npos || gameMessageStr.find("wins on time") != std::string::npos) {
                messageText.setFillColor(sf::Color(180, 0, 0)); // 어두운 빨강
                messageText.setStyle(sf::Text::Bold);
            } else if (gameMessageStr.find("Check!") != std::string::npos) {
                messageText.setFillColor(sf::Color(200, 0, 0)); // 밝은 빨강
                messageText.setStyle(sf::Text::Bold);
            }
             else if (gameMessageStr.find("Your turn") != std::string::npos) {
                messageText.setFillColor(sf::Color(0, 100, 0)); // 어두운 초록
                messageText.setStyle(sf::Text::Bold);
            } else if (gameMessageStr.find("to move") != std::string::npos) {
                 messageText.setFillColor(sf::Color::Black);
            }


            window.draw(messageText);
        }

        // 5. 게임 오버 팝업 그리기 (필요시)
        if (currentGameState == GameState::GameOver) {
            window.draw(popupBackground);

            popupMessageText.setString(gameMessageStr); // 게임 오버 메시지는 gameMessageStr 사용
            sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
            popupMessageText.setPosition({
                popupBackground.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x,
                popupBackground.getPosition().y - popupBackground.getSize().y / 2.f + 40.f - popupMsgBounds.position.y // 팝업 상단 근처
            });
            window.draw(popupMessageText);

            homeButtonShape.setPosition({
                popupBackground.getPosition().x,
                popupBackground.getPosition().y + popupBackground.getSize().y / 2.f - homeButtonShape.getSize().y / 2.f - 30.f // 팝업 하단 근처
            });
            window.draw(homeButtonShape);

            // homeButtonText의 원점과 위치를 homeButtonShape 기준으로 다시 계산
            sf::FloatRect homeTextBounds = homeButtonText.getLocalBounds();
            homeButtonText.setOrigin({homeTextBounds.position.x + homeTextBounds.size.x / 2.f, homeTextBounds.position.y + homeTextBounds.size.y / 2.f});
            homeButtonText.setPosition({homeButtonShape.getPosition().x, homeButtonShape.getPosition().y});
            window.draw(homeButtonText);
        }
    }

    window.display();
}