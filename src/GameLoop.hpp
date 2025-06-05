#ifndef GAMELOOP_HPP
#define GAMELOOP_HPP

#include "GameData.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>
#include <functional>
#include <boost/asio.hpp>

// formatTime 함수 선언
std::string formatTime(sf::Time time);

// 게임 루프 함수 선언
void gameLoop(
    sf::RenderWindow& window,
    sf::Font& font,
    sf::RectangleShape& tile,
    sf::Color& lightColor,
    sf::Color& darkColor,
    sf::Color& checkedKingTileColor,
    // sf::Text& chooseSidePromptText, // <--- 이 줄을 삭제하거나 주석 처리하세요.
    sf::Text& messageText,
    sf::Text& whiteTimerText,
    sf::Text& blackTimerText,
    sf::Sprite& startButtonSprite,
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
    std::map<std::string, sf::Texture>& textures,
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Time& whiteTimeLeft,
    sf::Time& blackTimeLeft,
    sf::Clock& frameClock,
    std::function<void()> actualResetGame,
    boost::asio::ip::tcp::socket& socket,
    PieceColor myColor,
    float timerPadding,
    sf::Sprite& backgroundSprite,
    sf::Sprite& logoSprite,
    sf::Sprite& uiPanelBgSprite
);

#endif // GAMELOOP_HPP