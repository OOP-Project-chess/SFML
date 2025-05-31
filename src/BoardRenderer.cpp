#include "BoardRenderer.hpp"
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
    sf::RectangleShape& popupBackground,
    sf::RectangleShape& homeButtonShape,
    sf::Text& homeButtonText,
    GameState& currentGameState,
    PieceColor& currentTurn,
    sf::Vector2i checkedKingCurrentPos,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Text& chooseSidePromptText,
    sf::RectangleShape& whiteStartButton,
    sf::Text& whiteStartText,
    sf::RectangleShape& blackStartButton,
    sf::Text& blackStartText
) {
    window.clear(sf::Color(240,240,240));

    window.clear(sf::Color(240,240,240));

    if (currentGameState == GameState::ChoosingPlayer) {
        window.draw(chooseSidePromptText);
        window.draw(whiteStartButton); window.draw(whiteStartText);
        window.draw(blackStartButton); window.draw(blackStartText);
    }
    else {
        for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
            bool isLight = (r + c) % 2 == 0;
            tile.setFillColor(isLight ? lightColor : darkColor);
            if (checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r)
                tile.setFillColor(checkedKingTileColor);
            else if (selectedPiecePos && selectedPiecePos->x == c && selectedPiecePos->y == r)
                tile.setFillColor(sf::Color(255, 255, 0, 150));
            else {
                for (const auto& move : possibleMoves) {
                    if (move.x == c && move.y == r) {
                        if (board_state[r][c] && board_state[r][c]->color != currentTurn)
                            tile.setFillColor(sf::Color(50, 150, 250, 150));
                        else
                            tile.setFillColor(sf::Color(100, 250, 50, 150));
                        break;
                    }
                }
            }
            tile.setPosition({c * static_cast<float>(TILE_SIZE), r * static_cast<float>(TILE_SIZE)});
            window.draw(tile);
        }

        for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
            if (board_state[r][c]) {
                Piece piece = board_state[r][c].value();
                bool isLosingKing = (currentGameState == GameState::GameOver &&
                                     piece.type == PieceType::King &&
                                     piece.color == currentTurn);
                piece.sprite.setColor(isLosingKing ? sf::Color::Red : sf::Color::White);
                window.draw(piece.sprite);
            }
        }

        window.draw(whiteTimerText);
        window.draw(blackTimerText);

        if (!gameMessageStr.empty()) {
            messageText.setString(gameMessageStr);
            sf::FloatRect msgBounds = messageText.getLocalBounds();
            messageText.setPosition({BOARD_WIDTH + (BUTTON_PANEL_WIDTH - msgBounds.size.x) / 2.f - msgBounds.position.x,
                                     (WINDOW_HEIGHT - msgBounds.size.y) / 2.f - msgBounds.position.y});
            messageText.setFillColor(sf::Color::Black);
            messageText.setStyle(sf::Text::Regular);
            if (currentGameState == GameState::GameOver || gameMessageStr.find("wins by") != std::string::npos)
                messageText.setFillColor(sf::Color::Red), messageText.setStyle(sf::Text::Bold);
            else if (gameMessageStr.find("Check!") != std::string::npos)
                messageText.setFillColor(sf::Color(200,0,0)), messageText.setStyle(sf::Text::Bold);

            window.draw(messageText);
        }

        if (currentGameState == GameState::GameOver) {
            window.draw(popupBackground);
            popupMessageText.setString(gameMessageStr);
            sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
            popupMessageText.setPosition({popupBackground.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x,
                                          popupBackground.getPosition().y - popupBackground.getSize().y / 2.f + 30.f - popupMsgBounds.position.y});
            window.draw(popupMessageText);

            homeButtonShape.setPosition({popupBackground.getPosition().x,
                                         popupBackground.getPosition().y + popupBackground.getSize().y / 2.f - 40.f - homeButtonShape.getSize().y / 2.f});
            window.draw(homeButtonShape);

            sf::FloatRect homeTextBounds = homeButtonText.getLocalBounds();
            homeButtonText.setPosition({homeButtonShape.getPosition().x - homeTextBounds.size.x / 2.f - homeTextBounds.position.x,
                                        homeButtonShape.getPosition().y - homeTextBounds.size.y / 2.f - homeTextBounds.position.y});
            window.draw(homeButtonText);
        }
    }

    window.display();
}