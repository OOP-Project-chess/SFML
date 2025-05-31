#include <iostream> // for std::cerr, std::cout
#include <iomanip>  // For std::setfill, std::setw
#include <sstream>  // For std::ostringstream
#include "GameLoop.hpp"
#include "GameLogic.hpp" // isKingInCheck, findKing, isCheckmate, getPossibleMoves 사용
#include "BoardRenderer.hpp"
#include "GameStateUpdater.hpp"
#include "InputHandler.hpp"

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

// 게임 루프 함수 정의
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
    std::map<std::string, sf::Texture>& textures, // place_piece 람다 때문에 main에서 textures를 캡처하고,
                                                 // actualResetGame이 textures를 사용하므로 gameLoop에는 직접 필요 없을 수 있으나,
                                                 // 혹시 모를 확장성을 위해 전달 (현재는 actualResetGame이 main의 textures를 캡처)
    std::array<std::array<std::optional<Piece>, 8>, 8>& board_state,
    sf::Time& whiteTimeLeft,
    sf::Time& blackTimeLeft,
    sf::Clock& frameClock,
    std::function<void()> actualResetGame,
    boost::asio::ip::tcp::socket& socket,
    float timerPadding
) {
    // --- 메인 게임 루프 ---
    while (window.isOpen()) {
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        // 게임 상태 업데이트 (시간, 체크/메이트, 메시지)
        updateTimersAndCheckState(currentGameState, currentTurn, whiteTimeLeft, blackTimeLeft, frameClock,
                          gameMessageStr, board_state, kingIsCurrentlyChecked, checkedKingCurrentPos);

        // 타이머 텍스트 업데이트
        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));

        sf::FloatRect wt_bounds_loop = whiteTimerText.getLocalBounds();
        whiteTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds_loop.size.x) / 2.f - wt_bounds_loop.position.x, timerPadding - wt_bounds_loop.position.y});
        sf::FloatRect bt_bounds_loop = blackTimerText.getLocalBounds();
        blackTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - bt_bounds_loop.size.x) / 2.f - bt_bounds_loop.position.x, whiteTimerText.getPosition().y + wt_bounds_loop.size.y + wt_bounds_loop.position.y + 5.f - bt_bounds_loop.position.y });

        // 이벤트 처리
        while (const auto event_opt = window.pollEvent()) {
            const sf::Event& event = *event_opt;
            if (event.is<sf::Event::Closed>()) window.close();
            else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            } else if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    handleMouseClick(mouseButtonPressed->position, currentGameState, whiteStartButton, blackStartButton,
                                     whiteStartText, blackStartText, frameClock, currentTurn, gameMessageStr,
                                     selectedPiecePos, possibleMoves, board_state, homeButtonShape, actualResetGame, socket);
                }
            }
        }

        drawBoardAndUI(window, tile, lightColor, darkColor, checkedKingTileColor,
                       selectedPiecePos, possibleMoves,
                       whiteTimerText, blackTimerText, messageText, gameMessageStr,
                       popupMessageText, popupBackground, homeButtonShape, homeButtonText,
                       currentGameState, currentTurn, checkedKingCurrentPos, board_state,
                       chooseSidePromptText,whiteStartButton,whiteStartText,blackStartButton,blackStartText);
    }
}
