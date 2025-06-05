#pragma once
#include <SFML/Graphics.hpp>
#include "GameData.hpp"

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
    PieceColor& currentTurn, // Pass currentTurn for renderer logic if needed
    sf::Vector2i checkedKingCurrentPos,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Sprite& startButtonSprite,
    sf::RectangleShape& blackStartButton,
    sf::Text& blackStartText,
    sf::Sprite& backgroundSprite, // 배경 스프라이트 추가
    sf::Sprite& logoSprite,      // 로고 스프라이트 추가
    sf::Sprite& uiPanelBgSprite
);