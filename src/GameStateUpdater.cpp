#include "GameStateUpdater.hpp"
#include "GameLogic.hpp"

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
) {
    if (gameState == GameState::Playing && currentTurn != PieceColor::None) {
        sf::Time deltaTime = frameClock.restart();

        if (currentTurn == PieceColor::White) {
            if (whiteTimeLeft > sf::Time::Zero) whiteTimeLeft -= deltaTime;
            if (whiteTimeLeft <= sf::Time::Zero) {
                whiteTimeLeft = sf::Time::Zero;
                gameState = GameState::GameOver;
                gameMessageStr = "Black wins on time!";
            }
        } else if (currentTurn == PieceColor::Black) {
            if (blackTimeLeft > sf::Time::Zero) blackTimeLeft -= deltaTime;
            if (blackTimeLeft <= sf::Time::Zero) {
                blackTimeLeft = sf::Time::Zero;
                gameState = GameState::GameOver;
                gameMessageStr = "White wins on time!";
            }
        }

        if (gameState != GameState::GameOver) {
            kingIsCurrentlyChecked = isKingInCheck(board, currentTurn);
            if (kingIsCurrentlyChecked) {
                checkedKingCurrentPos = findKing(board, currentTurn);
                if (isCheckmate(board, currentTurn)) {
                    gameState = GameState::GameOver;
                    gameMessageStr = (currentTurn == PieceColor::White ? "Black" : "White") + std::string(" wins by Checkmate!");
                } else {
                    gameMessageStr = (currentTurn == PieceColor::White ? "White" : "Black") + std::string(" King is in Check!");
                }
            } else {
                gameMessageStr = (currentTurn == PieceColor::White ? "White" : "Black") + std::string(" to move");
            }
        }
    } else if (gameState == GameState::ChoosingPlayer) {
        gameMessageStr.clear();
    }
}