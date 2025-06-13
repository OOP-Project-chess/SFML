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

std::string formatTime(sf::Time time);

void gameLoop(
    sf::RenderWindow& window,
    sf::Font& font,
    sf::RectangleShape& tile,
    sf::Color& lightColor,
    sf::Color& darkColor,
    sf::Color& checkedKingTileColor,
    sf::Text& messageText,
    sf::Text& whiteTimerText,
    sf::Text& blackTimerText,
    sf::Sprite& startButtonSprite,
    sf::RectangleShape& blackStartButton,
    sf::Text& blackStartText,
    sf::Sprite& popupImageSprite,
    sf::Text& popupMessageText,
    sf::Sprite& homeButtonSprite,
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
    float interTimerSpacing,
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
);

#endif // GAMELOOP_HPP
