#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include "GameData.hpp"

void handleMouseClick(
    const sf::Vector2i& mousePos,
    GameState& currentGameState,
    // sf::RectangleShape& whiteStartButton, // Removed
    // sf::Text& whiteStartText,          // Removed
    sf::Sprite& startButtonSprite,       // Added
    sf::RectangleShape& blackStartButton, // Keep for now
    sf::Text& blackStartText,          // Keep for now
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
);