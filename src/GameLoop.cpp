#include <iostream> // for std::cerr, std::cout
#include <iomanip>  // For std::setfill, std::setw
#include <sstream>  // For std::ostringstream
#include "GameLoop.hpp"
#include "GameLogic.hpp" // isKingInCheck, findKing, isCheckmate, getPossibleMoves 사용
#include "BoardRenderer.hpp"
#include "GameStateUpdater.hpp"
#include "InputHandler.hpp"
#include <nlohmann/json.hpp>     // ✅ 헤더 추가
using json = nlohmann::json;    // ✅ 별칭 선언
#include "SharedState.hpp"

// formatTime 함수 정의
std::string formatTime(sf::Time time) {
    int totalSeconds = static_cast<int>(time.asSeconds());
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

void gameLoop(
    sf::RenderWindow& window,
    sf::Font& font,
    sf::RectangleShape& tile,
    sf::Color& lightColor,
    sf::Color& darkColor,
    sf::Color& checkedKingTileColor,
    sf::Text& chooseSidePromptText,
    sf::Text& messageText,
    sf::Text& whiteTimerText,
    sf::Text& blackTimerText,
    sf::RectangleShape& whiteStartButton,
    sf::Text& whiteStartText,
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
    float timerPadding
) {
    while (window.isOpen()) {
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        updateTimersAndCheckState(currentGameState, currentTurn, whiteTimeLeft, blackTimeLeft, frameClock,
                                  gameMessageStr, board_state, kingIsCurrentlyChecked, checkedKingCurrentPos);

        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));

        sf::FloatRect wt_bounds_loop = whiteTimerText.getLocalBounds();
        whiteTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds_loop.size.x) / 2.f - wt_bounds_loop.position.x, timerPadding - wt_bounds_loop.position.y });
        sf::FloatRect bt_bounds_loop = blackTimerText.getLocalBounds();
        blackTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - bt_bounds_loop.size.x) / 2.f - bt_bounds_loop.position.x, whiteTimerText.getPosition().y + wt_bounds_loop.size.y + wt_bounds_loop.position.y + 5.f - bt_bounds_loop.position.y });

        while (const auto event_opt = window.pollEvent()) {
            const sf::Event& event = *event_opt;
            if (event.is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            } else if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    handleMouseClick(mouseButtonPressed->position, currentGameState, whiteStartButton, blackStartButton,
                                     whiteStartText, blackStartText, frameClock, currentTurn, gameMessageStr,
                                     selectedPiecePos, possibleMoves, board_state, homeButtonShape, actualResetGame, socket, myColor);
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(messageMutex);
            while (!messageQueue.empty()) {
                std::string msg = messageQueue.front();
                messageQueue.pop();

                try {
                    json parsed = json::parse(msg);
                    if (parsed["type"] == "move") {
                        std::string from = parsed["from"];
                        std::string to = parsed["to"];
                        std::string piece = parsed["piece"];
                        std::string color = parsed["color"];

                        int fromRow = 8 - (from[1] - '0');
                        int fromCol = from[0] - 'a';
                        int toRow = 8 - (to[1] - '0');
                        int toCol = to[0] - 'a';

                        auto& movingPiece = board_state[fromRow][fromCol];
                        if (movingPiece) {
                            board_state[toRow][toCol] = movingPiece;
                            board_state[fromRow][fromCol] = std::nullopt;
                            auto& sprite = board_state[toRow][toCol]->sprite;
                            sf::FloatRect spriteBounds = sprite.getGlobalBounds();
                            float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                            float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                            sprite.setPosition(sf::Vector2f(toCol * TILE_SIZE + x_offset, toRow * TILE_SIZE + y_offset));
                        }
                    } else if (parsed["type"] == "turn") {
                        std::string turn = parsed["currentTurn"];
                        currentTurn = (turn == "white") ? PieceColor::White : PieceColor::Black;
                    }
                } catch (...) {
                    std::cerr << "[gameLoop] JSON 파싱 실패: " << msg << "\n";
                }
            }
        }

        drawBoardAndUI(window, tile, lightColor, darkColor, checkedKingTileColor,
                       selectedPiecePos, possibleMoves,
                       whiteTimerText, blackTimerText, messageText, gameMessageStr,
                       popupMessageText, popupBackground, homeButtonShape, homeButtonText,
                       currentGameState, currentTurn, checkedKingCurrentPos, board_state,
                       chooseSidePromptText, whiteStartButton, whiteStartText, blackStartButton, blackStartText);
    }
}
