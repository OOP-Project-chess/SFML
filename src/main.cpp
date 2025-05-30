#include "GameData.hpp"     // GameState, Piece, 상수 등 모든 정의 포함
#include "GameLogic.hpp"    // getPossibleMoves 등 게임 로직 함수

#include <SFML/Graphics.hpp>
// Boost.Asio 관련 헤더는 네트워크 기능 추가 시 다시 포함
// #include <boost/asio.hpp>
// #include <boost/asio/ip/address.hpp>

#include <iostream> // for std::cerr, std::cout
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>
// #include <thread> // 네트워크 기능 추가 시 필요할 수 있음
// #include <chrono> // 네트워크 기능 추가 시 필요할 수 있음
#include <filesystem> // For paths
#include <iomanip>    // For std::setfill, std::setw
#include <sstream>    // For std::ostringstream

// using boost::asio::ip::tcp; // 네트워크 기능 추가 시 다시 포함
using namespace std; // 사용자 코드 스타일 유지

// 시간 포맷팅 함수
std::string formatTime(sf::Time time) {
    int totalSeconds = static_cast<int>(time.asSeconds());
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

// 게임 루프 함수 선언
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
    float timerPadding // timerPadding 파라미터 추가
) {
    // --- 메인 게임 루프 ---
    while (window.isOpen()) {
        sf::Time deltaTime = frameClock.restart();
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        // 게임 상태 업데이트 (시간, 체크/메이트, 메시지)
        if (currentGameState == GameState::Playing && currentTurn != PieceColor::None) {
            if (currentTurn == PieceColor::White) {
                if (whiteTimeLeft > sf::Time::Zero) whiteTimeLeft -= deltaTime;
                if (whiteTimeLeft <= sf::Time::Zero) { whiteTimeLeft = sf::Time::Zero; currentGameState = GameState::GameOver; gameMessageStr = "Black wins on time!"; }
            } else if (currentTurn == PieceColor::Black) {
                if (blackTimeLeft > sf::Time::Zero) blackTimeLeft -= deltaTime;
                if (blackTimeLeft <= sf::Time::Zero) { blackTimeLeft = sf::Time::Zero; currentGameState = GameState::GameOver; gameMessageStr = "White wins on time!"; }
            }
            if (currentGameState != GameState::GameOver) {
                 kingIsCurrentlyChecked = isKingInCheck(board_state, currentTurn);
                if (kingIsCurrentlyChecked) {
                    checkedKingCurrentPos = findKing(board_state, currentTurn);
                    if (isCheckmate(board_state, currentTurn)) {
                        currentGameState = GameState::GameOver; gameMessageStr = std::string((currentTurn == PieceColor::White) ? "Black" : "White") + " wins by Checkmate!";
                    } else { gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " King is in Check!"; }
                } else { gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " to move"; }
            }
        } else if (currentGameState == GameState::ChoosingPlayer) {
            gameMessageStr = "";
        }
        
        // 타이머 텍스트 업데이트
        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));
        
        sf::FloatRect wt_bounds_loop = whiteTimerText.getLocalBounds();
        whiteTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds_loop.size.x) / 2.f - wt_bounds_loop.position.x, timerPadding - wt_bounds_loop.position.y}); // 전달받은 timerPadding 사용
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
                    sf::Vector2i mousePos = mouseButtonPressed->position;

                    if (currentGameState == GameState::ChoosingPlayer) {
                        if (whiteStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::White; currentGameState = GameState::Playing;
                            gameMessageStr = "White to move"; frameClock.restart();
                        } else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::Black; currentGameState = GameState::Playing;
                            gameMessageStr = "Black to move"; frameClock.restart();
                        }
                    } else if (currentGameState == GameState::Playing) {
                        int clickedCol = mousePos.x / TILE_SIZE; int clickedRow = mousePos.y / TILE_SIZE;
                        if (clickedCol >=0 && clickedCol < 8 && clickedRow >=0 && clickedRow < 8) {
                            bool moved = false;
                            int fromR_local = -1, fromC_local = -1;

                            if (selectedPiecePos.has_value()) {
                                fromR_local = selectedPiecePos->y;
                                fromC_local = selectedPiecePos->x;
                                for (const auto& move_coord : possibleMoves) {
                                    if (move_coord.x == clickedCol && move_coord.y == clickedRow) {
                                        std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board_state;
                                        std::optional<Piece> pieceToMoveOpt = tempBoard[fromR_local][fromC_local];
                                        if (pieceToMoveOpt.has_value()){
                                            tempBoard[clickedRow][clickedCol] = Piece(pieceToMoveOpt->type, pieceToMoveOpt->color, pieceToMoveOpt->sprite);
                                            tempBoard[fromR_local][fromC_local].reset();
                                        }
                                        if (!isKingInCheck(tempBoard, currentTurn)) {
                                            std::optional<Piece> actualPieceToMoveOpt = board_state[fromR_local][fromC_local];
                                            if(actualPieceToMoveOpt.has_value()){
                                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                                board_state[fromR_local][fromC_local].reset();
                                            }
                                            if (board_state[clickedRow][clickedCol].has_value()) {
                                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds();
                                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                                movedSprite.setPosition({clickedCol*static_cast<float>(TILE_SIZE)+x_offset, clickedRow*static_cast<float>(TILE_SIZE)+y_offset});
                                            }
                                            moved = true;
                                            currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                                            frameClock.restart();
                                            break;
                                        } else gameMessageStr = "Invalid move: King would be in check!";
                                    }
                                }
                            }
                            if (moved) {
                                selectedPiecePos.reset(); possibleMoves.clear();
                            } else {
                                if (board_state[clickedRow][clickedCol].has_value() && board_state[clickedRow][clickedCol]->color == currentTurn) {
                                    if (selectedPiecePos.has_value() && selectedPiecePos->x == clickedCol && selectedPiecePos->y == clickedRow) {
                                        selectedPiecePos.reset(); possibleMoves.clear();
                                    } else {
                                        selectedPiecePos = sf::Vector2i(clickedCol, clickedRow);
                                        possibleMoves = getPossibleMoves(board_state, clickedRow, clickedCol);
                                        
                                        std::vector<sf::Vector2i> valid_moves;
                                        for(const auto& p_move : possibleMoves){
                                            std::array<std::array<std::optional<Piece>, 8>, 8> temp_board_check = board_state;
                                            std::optional<Piece> piece_to_sim = temp_board_check[clickedRow][clickedCol];
                                            if(piece_to_sim.has_value()){
                                                temp_board_check[p_move.y][p_move.x] = Piece(piece_to_sim->type, piece_to_sim->color, piece_to_sim->sprite);
                                                temp_board_check[clickedRow][clickedCol].reset();
                                                if(!isKingInCheck(temp_board_check, currentTurn)){
                                                    valid_moves.push_back(p_move);
                                                }
                                            }
                                        }
                                        possibleMoves = valid_moves;
                                    }
                                } else {
                                    selectedPiecePos.reset(); possibleMoves.clear();
                                }
                            }
                        } else { selectedPiecePos.reset(); possibleMoves.clear(); }
                    } else if (currentGameState == GameState::GameOver) {
                         if (homeButtonShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            actualResetGame(); 
                         }
                    }
                }
            }
        }

        // 그리기
        window.clear(sf::Color(240,240,240));

        if (currentGameState == GameState::ChoosingPlayer) {
            window.draw(chooseSidePromptText);
            window.draw(whiteStartButton); window.draw(whiteStartText);
            window.draw(blackStartButton); window.draw(blackStartText);
        } else { // Playing or GameOver
            for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
                bool isLight = (r + c) % 2 == 0;
                tile.setFillColor(isLight ? lightColor : darkColor);
                if (kingIsCurrentlyChecked && checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r) tile.setFillColor(checkedKingTileColor);
                else if (selectedPiecePos.has_value() && selectedPiecePos->x == c && selectedPiecePos->y == r) tile.setFillColor(sf::Color(255, 255, 0, 150));
                else {
                    for (const auto& move_coord : possibleMoves) {
                        if (move_coord.x == c && move_coord.y == r) {
                            if (board_state[r][c].has_value() && board_state[r][c]->color != currentTurn) tile.setFillColor(sf::Color(50, 150, 250, 150));
                            else tile.setFillColor(sf::Color(100, 250, 50, 150));
                            break;
                        }
                    }
                }
                tile.setPosition({c * static_cast<float>(TILE_SIZE), r * static_cast<float>(TILE_SIZE)});
                window.draw(tile);
            }
            for (int r_idx = 0; r_idx < 8; ++r_idx) for (int c_idx = 0; c_idx < 8; ++c_idx) {
                if (board_state[r_idx][c_idx].has_value()) {
                    Piece piece_to_draw = board_state[r_idx][c_idx].value();
                    bool isLosingKing = (currentGameState == GameState::GameOver && piece_to_draw.type == PieceType::King && piece_to_draw.color == currentTurn);
                    if (isLosingKing) piece_to_draw.sprite.setColor(sf::Color::Red);
                    else piece_to_draw.sprite.setColor(sf::Color::White);
                    window.draw(piece_to_draw.sprite);
                }
            }
            window.draw(whiteTimerText); window.draw(blackTimerText);
            if (!gameMessageStr.empty()) {
                messageText.setString(gameMessageStr);
                sf::FloatRect gameMsgLocalBounds = messageText.getLocalBounds();
                messageText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - gameMsgLocalBounds.size.x) / 2.f - gameMsgLocalBounds.position.x, (WINDOW_HEIGHT - gameMsgLocalBounds.size.y) / 2.f - gameMsgLocalBounds.position.y });
                messageText.setFillColor(sf::Color::Black); messageText.setStyle(sf::Text::Regular);
                if(currentGameState == GameState::GameOver || gameMessageStr.find("wins by") != std::string::npos ) { messageText.setFillColor(sf::Color::Red); messageText.setStyle(sf::Text::Bold); }
                else if (gameMessageStr.find("Check!") != std::string::npos ) { messageText.setFillColor(sf::Color(200,0,0)); messageText.setStyle(sf::Text::Bold); }
                window.draw(messageText);
            }

            if (currentGameState == GameState::GameOver) {
                window.draw(popupBackground);
                popupMessageText.setString(gameMessageStr);
                sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
                popupMessageText.setPosition({ popupBackground.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x, popupBackground.getPosition().y - popupBackground.getSize().y / 2.f + 30.f - popupMsgBounds.position.y });
                window.draw(popupMessageText);
                homeButtonShape.setPosition({ popupBackground.getPosition().x, popupBackground.getPosition().y + popupBackground.getSize().y / 2.f - 40.f - homeButtonShape.getSize().y / 2.f });
                window.draw(homeButtonShape);
                sf::FloatRect homeButtonTextBounds = homeButtonText.getLocalBounds();
                homeButtonText.setPosition({ homeButtonShape.getPosition().x - homeButtonTextBounds.size.x / 2.f - homeButtonTextBounds.position.x, homeButtonShape.getPosition().y - homeButtonTextBounds.size.y / 2.f - homeButtonTextBounds.position.y });
                window.draw(homeButtonText);
            }
        }
        window.display();
    }
}


int main() {
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Chess Game Prj (SFML 3.0.x)");
    window.setFramerateLimit(60);

    sf::RectangleShape tile{ {static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)} };
    sf::Color lightColor{ 207, 202, 198 };
    sf::Color darkColor{ 64, 58, 53 };
    sf::Color checkedKingTileColor{255, 0, 0, 180};

    sf::Font font;
    std::filesystem::path fontPath = std::filesystem::path("Textures") / "Paperlogy-4R.ttf";
    if (!font.openFromFile(fontPath)) {
        std::cerr << "Failed to open font: " << fontPath.string() << std::endl;
        std::filesystem::path altFontPath = std::filesystem::path("arial.ttf");
        if (!font.openFromFile(altFontPath)) {
             std::cerr << "Failed to open alternative font (e.g., arial.ttf from system path)." << std::endl;
             return -1;
        } else {
            std::cout << "Loaded alternative font: " << altFontPath.string() << std::endl;
        }
    }

    sf::Text chooseSidePromptText(font);
    chooseSidePromptText.setString("Choose your side to start");
    chooseSidePromptText.setCharacterSize(30);
    chooseSidePromptText.setFillColor(sf::Color::Black);
    sf::FloatRect chooseSideBounds = chooseSidePromptText.getLocalBounds();
    chooseSidePromptText.setPosition({
        (WINDOW_WIDTH - chooseSideBounds.size.x) / 2.f - chooseSideBounds.position.x,
        WINDOW_HEIGHT / 2.f - 120.f - chooseSideBounds.position.y
    });

    sf::Text messageText(font);
    messageText.setCharacterSize(20);
    messageText.setFillColor(sf::Color::Black);

    sf::Text whiteTimerText(font, "00:00", 22);
    whiteTimerText.setFillColor(sf::Color::Black);
    sf::Text blackTimerText(font, "00:00", 22);
    blackTimerText.setFillColor(sf::Color::Black);

    float timerPadding = 15.f; // main 함수에 timerPadding 정의

    float buttonWidth = BUTTON_PANEL_WIDTH - 80;
    float buttonHeight = 50.f;

    sf::RectangleShape whiteStartButton({buttonWidth, buttonHeight});
    whiteStartButton.setFillColor(sf::Color(220, 220, 220));
    whiteStartButton.setPosition({
        (WINDOW_WIDTH - buttonWidth) / 2.f,
        chooseSidePromptText.getPosition().y + chooseSideBounds.size.y + chooseSideBounds.position.y + 30.f
    });

    sf::Text whiteStartText(font, "Start as White", 24);
    whiteStartText.setFillColor(sf::Color::Black);
    sf::FloatRect whiteTextBounds = whiteStartText.getLocalBounds();
    whiteStartText.setPosition({
        whiteStartButton.getPosition().x + (whiteStartButton.getSize().x - whiteTextBounds.size.x) / 2.f - whiteTextBounds.position.x,
        whiteStartButton.getPosition().y + (whiteStartButton.getSize().y - whiteTextBounds.size.y) / 2.f - whiteTextBounds.position.y
    });

    sf::RectangleShape blackStartButton({buttonWidth, buttonHeight});
    blackStartButton.setFillColor(sf::Color(80, 80, 80));
    blackStartButton.setPosition({
        (WINDOW_WIDTH - buttonWidth) / 2.f,
        whiteStartButton.getPosition().y + whiteStartButton.getSize().y + 20.f
    });

    sf::Text blackStartText(font, "Start as Black", 24);
    blackStartText.setFillColor(sf::Color::White);
    sf::FloatRect blackTextBounds = blackStartText.getLocalBounds();
    blackStartText.setPosition({
        blackStartButton.getPosition().x + (blackStartButton.getSize().x - blackTextBounds.size.x) / 2.f - blackTextBounds.position.x,
        blackStartButton.getPosition().y + (blackStartButton.getSize().y - blackTextBounds.size.y) / 2.f - blackTextBounds.position.y
    });

    sf::RectangleShape popupBackground({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 3.f});
    popupBackground.setFillColor(sf::Color(100, 100, 100, 220));
    popupBackground.setOrigin({popupBackground.getSize().x / 2.f, popupBackground.getSize().y / 2.f});
    popupBackground.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f});

    sf::Text popupMessageText(font, "", 28);
    popupMessageText.setFillColor(sf::Color::White);

    sf::RectangleShape homeButtonShape({150.f, 50.f});
    homeButtonShape.setFillColor(sf::Color(70, 130, 180));
    homeButtonShape.setOrigin({homeButtonShape.getSize().x / 2.f, homeButtonShape.getSize().y / 2.f});

    sf::Text homeButtonText(font, "HOME", 24);
    homeButtonText.setFillColor(sf::Color::White);

    GameState currentGameState = GameState::ChoosingPlayer;
    std::optional<sf::Vector2i> selectedPiecePos;
    std::vector<sf::Vector2i> possibleMoves;
    PieceColor currentTurn = PieceColor::None;
    std::string gameMessageStr = "";

    std::map<std::string, sf::Texture> textures;
    std::vector<std::string> names = { "king", "queen", "rook", "bishop", "knight", "pawn" };

    for (const auto& color_str : { "w", "b" }) {
        for (const auto& name : names) {
            std::string key = std::string(color_str) + "_" + name;
            sf::Texture tex;
            std::filesystem::path texturePath = std::filesystem::path("Textures") / (key + ".png");
            if (!tex.loadFromFile(texturePath)) {
                std::cerr << "Failed to load texture: " << texturePath.string() << std::endl;
            }
            textures[key] = std::move(tex);
        }
    }

    std::array<std::array<std::optional<Piece>, 8>, 8> board_state;

    sf::Time whiteTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
    sf::Time blackTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
    sf::Clock frameClock;

    auto place_piece = [&](int r, int c, PieceType type, PieceColor piece_color, const std::string& name_str) {
        std::string key = (piece_color == PieceColor::White ? "w_" : "b_") + name_str;
        if (textures.count(key) == 0 || textures[key].getSize().x == 0) {
            std::cerr << "Texture for key '" << key << "' not found or invalid!" << std::endl; return;
        }
        sf::Sprite sprite(textures[key]);
        sprite.setScale({0.5f, 0.5f});
        sf::FloatRect spriteBounds = sprite.getGlobalBounds();
        float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
        float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
        sprite.setPosition({c * static_cast<float>(TILE_SIZE) + x_offset, r * static_cast<float>(TILE_SIZE) + y_offset});
        board_state[r][c] = Piece{type, piece_color, sprite};
    };

    auto actualSetupBoard = [&]() {
        board_state = {};
        place_piece(7,0,PieceType::Rook,PieceColor::White,"rook"); place_piece(7,1,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,2,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,3,PieceType::Queen,PieceColor::White,"queen"); place_piece(7,4,PieceType::King,PieceColor::White,"king"); place_piece(7,5,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,6,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,7,PieceType::Rook,PieceColor::White,"rook");
        for(int c_idx=0;c_idx<8;++c_idx)place_piece(6,c_idx,PieceType::Pawn,PieceColor::White,"pawn");
        place_piece(0,0,PieceType::Rook,PieceColor::Black,"rook"); place_piece(0,1,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,2,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,3,PieceType::Queen,PieceColor::Black,"queen"); place_piece(0,4,PieceType::King,PieceColor::Black,"king"); place_piece(0,5,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,6,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,7,PieceType::Rook,PieceColor::Black,"rook");
        for(int c_idx=0;c_idx<8;++c_idx)place_piece(1,c_idx,PieceType::Pawn,PieceColor::Black,"pawn");
    };

    auto actualResetGame_lambda = [&]() {
        actualSetupBoard();
        currentGameState = GameState::ChoosingPlayer;
        currentTurn = PieceColor::None;
        selectedPiecePos.reset();
        possibleMoves.clear();
        gameMessageStr = "";
        whiteTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        blackTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        frameClock.restart();
    };

    actualSetupBoard();
    frameClock.restart();

    // 게임 루프 호출
    gameLoop(
        window, font, tile, lightColor, darkColor, checkedKingTileColor,
        chooseSidePromptText, messageText, whiteTimerText, blackTimerText,
        whiteStartButton, whiteStartText, blackStartButton, blackStartText,
        popupBackground, popupMessageText, homeButtonShape, homeButtonText,
        currentGameState, selectedPiecePos, possibleMoves, currentTurn, gameMessageStr,
        textures, board_state, whiteTimeLeft, blackTimeLeft, frameClock,
        actualResetGame_lambda,
        timerPadding // timerPadding 전달
    );

    return 0;
}
