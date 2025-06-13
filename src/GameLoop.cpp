#include <iostream>
#include <iomanip>
#include <sstream>
#include "GameLoop.hpp"
#include "GameLogic.hpp"
#include "BoardRenderer.hpp"
#include "GameStateUpdater.hpp"
#include "InputHandler.hpp"
#include "ChessUtils.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "SharedState.hpp"

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
) {
    while (window.isOpen()) {
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        updateTimersAndCheckState(currentGameState, currentTurn, whiteTimeLeft, blackTimeLeft, frameClock,
                                  gameMessageStr, board_state, kingIsCurrentlyChecked, checkedKingCurrentPos);

        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));

        while (const auto event_opt = window.pollEvent()) {
            const sf::Event& event = *event_opt;
            if (event.is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            } else if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    handleMouseClick(mouseButtonPressed->position, currentGameState,
                                     startButtonSprite,
                                     blackStartButton, blackStartText,
                                     frameClock, currentTurn, gameMessageStr,
                                     selectedPiecePos, possibleMoves, board_state,
                                     homeButtonSprite,
                                     actualResetGame, socket, myColor);
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
                    if (parsed.contains("type")) {
                        if (parsed["type"] == "move") {
                            std::string from = parsed["from"];
                            std::string to = parsed["to"];
                            // Piece info not strictly needed for opponent move if server is authoritative
                            // std::string piece = parsed["piece"];
                            // std::string color = parsed["color"];

                            // Convert chess notation to board indices
                            int fromCol = from[0] - 'a';
                            int fromRow = '8' - from[1]; // Assuming '1' is bottom, '8' is top for notation
                            int toCol = to[0] - 'a';
                            int toRow = '8' - to[1];


                            if (fromRow >= 0 && fromRow < 8 && fromCol >= 0 && fromCol < 8 &&
                                toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
                                auto& movingPieceOpt = board_state[fromRow][fromCol];
                                if (movingPieceOpt) {
                                    // Create new piece for the new location to ensure sprite is handled correctly
                                    board_state[toRow][toCol] = Piece(movingPieceOpt->type, movingPieceOpt->color, movingPieceOpt->sprite);
                                    board_state[fromRow][fromCol] = std::nullopt; // Clear old position

                                    // Update sprite position for the moved piece
                                    auto& newPieceSprite = board_state[toRow][toCol]->sprite;
                                    // Scale should already be set from place_piece
                                    sf::FloatRect sprite_bounds = newPieceSprite.getGlobalBounds();
                                    float x_offset = (static_cast<float>(TILE_SIZE) - sprite_bounds.size.x) / 2.f;
                                    float y_offset = (static_cast<float>(TILE_SIZE) - sprite_bounds.size.y) / 2.f;
                                    newPieceSprite.setPosition(sf::Vector2f(toCol * TILE_SIZE + x_offset, toRow * TILE_SIZE + y_offset));
                                } else {
                                     std::cerr << "[gameLoop] Error: No piece at source for move: " << from << std::endl;
                                }
                            } else {
                                std::cerr << "[gameLoop] Error: Invalid move coordinates from server: " << from << " to " << to << std::endl;
                            }
                        } else if (parsed["type"] == "assignColor") { // Moved from main for centralized handling
                            std::string color_str = parsed["color"];
                            myColor = (color_str == "white") ? PieceColor::White : PieceColor::Black;
                            gameMessageStr = "You are " + color_str + ". Waiting for game to start.";
                            std::cout << "Assigned color: " << color_str << std::endl;
                        }
                         else if (parsed["type"] == "turn") {
                            std::string turnColorStr = parsed["currentTurn"];
                            currentTurn = (turnColorStr == "white") ? PieceColor::White : PieceColor::Black;
                             // gameMessageStr = (currentTurn == myColor ? "Your turn" : "Opponent's turn"); // This will be set by updateTimersAndCheckState
                             std::cout << "Server says turn: " << turnColorStr << std::endl;
                             frameClock.restart(); // Restart clock for the new turn
                        } else if (parsed["type"] == "gameState") { // Example: Server dictates game state
                            std::string stateStr = parsed["state"];
                            if (stateStr == "playing" && currentGameState != GameState::Playing) {
                                currentGameState = GameState::Playing;
                                frameClock.restart(); // Start timers if game is now playing
                                std::cout << "Game state set to Playing by server." << std::endl;
                            } else if (stateStr == "gameOver") {
                                currentGameState = GameState::GameOver;
                                if(parsed.contains("message")) gameMessageStr = parsed["message"];
                                std::cout << "Game state set to GameOver by server." << std::endl;
                            }
                            // Add more states if needed
                        }
                    }
                } catch (const json::parse_error& e) {
                    std::cerr << "[gameLoop] JSON parsing failed: " << e.what() << " for message: " << msg << "\n";
                } catch (const std::exception& e) {
                    std::cerr << "[gameLoop] Error processing message: " << e.what() << " for message: " << msg << "\n";
                }
            }
        }

        drawBoardAndUI(window, tile, lightColor, darkColor, checkedKingTileColor,
                       selectedPiecePos, possibleMoves,
                       whiteTimerText, blackTimerText, messageText, gameMessageStr,
                       popupMessageText,
                       popupImageSprite,
                       homeButtonSprite,
                       currentGameState, currentTurn, checkedKingCurrentPos, board_state,
                       startButtonSprite,
                       blackStartButton, blackStartText,
                       backgroundSprite,
                       logoSprite,
                       uiPanelBgSprite,
                       player1Sprite,
                       player2Sprite,
                       player1NameText,
                       player2NameText,
                       player1Texture,
                       player2Texture,
                       waitingTexture
                       );
    }
}
