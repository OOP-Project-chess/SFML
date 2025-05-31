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
    PieceColor& currentTurn,
    sf::Vector2i checkedKingCurrentPos,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Text& chooseSidePromptText,
    sf::RectangleShape& whiteStartButton,
   sf::Text& whiteStartText,
   sf::RectangleShape& blackStartButton,
   sf::Text& blackStartText

);