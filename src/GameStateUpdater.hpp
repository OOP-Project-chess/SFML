#pragma once
#include <SFML/System.hpp>
#include "GameData.hpp"
#include <string>
#include <array>
#include <optional>

void updateTimersAndCheckState(
    GameState& gameState,
    PieceColor& currentTurn,
    sf::Time& whiteTimeLeft,
    sf::Time& blackTimeLeft,
    sf::Clock& frameClock,
    std::string& gameMessageStr,
    const std::array<std::array<std::optional<Piece>, 8>, 8>& board,
    bool& kingIsCurrentlyChecked,
    sf::Vector2i& checkedKingCurrentPos
);