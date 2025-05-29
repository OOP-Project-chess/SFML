#include <SFML/Graphics.hpp>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <iostream> // for std::cerr
#include <filesystem> // For paths
#include <iomanip> // For std::setfill, std::setw (타이머 포맷팅)
#include <sstream> // For std::ostringstream (타이머 포맷팅)

// TODO: 소켓 통신 연동
// TODO: 사운드 (마우스 클릭, 기물 놓기, 잔잔한 배경음악...?, 졌을 때/이겼을 때 효과음)
// TODO: 기물 움직임 색 바꾸는 것 (부가적 요소)
// TODO: 한 사람 당 전체 시간 (타이머) 20분 -> 현재 1분으로 테스트 중


// 전역 상수 정의
const int TILE_SIZE = 100;
const int BOARD_WIDTH = 8 * TILE_SIZE;
const int BOARD_HEIGHT = 8 * TILE_SIZE;
const int BUTTON_PANEL_WIDTH = 300;
const int WINDOW_WIDTH = BOARD_WIDTH + BUTTON_PANEL_WIDTH;
const int WINDOW_HEIGHT = BOARD_HEIGHT;
const float INITIAL_TIME_SECONDS = 60.f; // 플레이어당 초기 시간 (초)

enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn, None };
enum class PieceColor { White, Black, None };
enum class GameState { ChoosingPlayer, Playing, GameOver };

struct Piece {
    PieceType type;
    PieceColor color;
    sf::Sprite sprite;
    Piece(PieceType t, PieceColor c, sf::Sprite s) : type(t), color(c), sprite(std::move(s)) {}
};

// getPossibleMoves, findKing, isKingInCheck, isCheckmate 함수 (이전과 동일)
std::vector<sf::Vector2i> getPossibleMoves(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, int row, int col) {
    std::vector<sf::Vector2i> moves;
    if (!board[row][col].has_value()) return moves;
    PieceType type = board[row][col]->type;
    PieceColor color = board[row][col]->color;
    auto addMovesInDirection = [&](int dr, int dc, bool canMoveMultipleSteps) {
        for (int i = 1; i < 8; ++i) {
            int targetRow = row + dr * i; int targetCol = col + dc * i;
            if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8) break;
            if (board[targetRow][targetCol].has_value()) {
                if (board[targetRow][targetCol]->color != color) moves.push_back({targetCol, targetRow});
                break;
            } else moves.push_back({targetCol, targetRow});
            if (!canMoveMultipleSteps) break;
        }
    };
    if (type == PieceType::Pawn) {
        int direction = (color == PieceColor::White) ? -1 : 1;
        int nextRow = row + direction;
        if (nextRow >= 0 && nextRow < 8 && !board[nextRow][col].has_value()) {
            moves.push_back({col, nextRow});
            bool isInitialPosition = (color == PieceColor::White && row == 6) || (color == PieceColor::Black && row == 1);
            if (isInitialPosition) {
                int twoStepsRow = row + 2 * direction;
                if (twoStepsRow >=0 && twoStepsRow < 8 && !board[row + direction][col].has_value() && !board[twoStepsRow][col].has_value()){
                     moves.push_back({col, twoStepsRow});
                }
            }
        }
        int attackColLeft = col - 1; int attackRow = row + direction;
        if (attackColLeft >= 0 && attackRow >= 0 && attackRow < 8) {
            if (board[attackRow][attackColLeft].has_value() && board[attackRow][attackColLeft]->color != color) moves.push_back({attackColLeft, attackRow});
        }
        int attackColRight = col + 1;
        if (attackColRight < 8 && attackRow >= 0 && attackRow < 8) {
            if (board[attackRow][attackColRight].has_value() && board[attackRow][attackColRight]->color != color) moves.push_back({attackColRight, attackRow});
        }
    } else if (type == PieceType::Rook) {
        addMovesInDirection(1, 0, true); addMovesInDirection(-1, 0, true); addMovesInDirection(0, 1, true); addMovesInDirection(0, -1, true);
    } else if (type == PieceType::Bishop) {
        addMovesInDirection(1, 1, true); addMovesInDirection(1, -1, true); addMovesInDirection(-1, 1, true); addMovesInDirection(-1, -1, true);
    } else if (type == PieceType::Queen) {
        addMovesInDirection(1, 0, true); addMovesInDirection(-1, 0, true); addMovesInDirection(0, 1, true); addMovesInDirection(0, -1, true);
        addMovesInDirection(1, 1, true); addMovesInDirection(1, -1, true); addMovesInDirection(-1, 1, true); addMovesInDirection(-1, -1, true);
    } else if (type == PieceType::Knight) {
        std::vector<std::pair<int, int>> knightMoves = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for (const auto& move : knightMoves) {
            int targetRow = row + move.first; int targetCol = col + move.second;
            if (targetRow >= 0 && targetRow < 8 && targetCol >= 0 && targetCol < 8) {
                if (!board[targetRow][targetCol].has_value() || board[targetRow][targetCol]->color != color) moves.push_back({targetCol, targetRow});
            }
        }
    } else if (type == PieceType::King) {
        for (int dr = -1; dr <= 1; ++dr) for (int dc = -1; dc <= 1; ++dc) {
            if (dr==0 && dc==0) continue;
            int targetRow = row + dr; int targetCol = col + dc;
            if (targetRow >=0 && targetRow < 8 && targetCol >=0 && targetCol < 8) {
                if (!board[targetRow][targetCol].has_value() || board[targetRow][targetCol]->color != color) moves.push_back({targetCol, targetRow});
            }
        }
    }
    return moves;
}
sf::Vector2i findKing(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor) {
    for (int r=0; r<8; ++r) for (int c=0; c<8; ++c)
        if (board[r][c].has_value() && board[r][c]->type == PieceType::King && board[r][c]->color == kingColor) return {c, r};
    return {-1,-1};
}
bool isKingInCheck(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor kingColor) {
    sf::Vector2i kingPos = findKing(board, kingColor);
    if (kingPos.x == -1) return false;
    PieceColor opponentColor = (kingColor == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    for (int r=0; r<8; ++r) for (int c=0; c<8; ++c) {
        if (board[r][c].has_value() && board[r][c]->color == opponentColor) {
            std::vector<sf::Vector2i> opponentMoves = getPossibleMoves(board, r, c);
            for (const auto& move : opponentMoves) if (move.x == kingPos.x && move.y == kingPos.y) return true;
        }
    }
    return false;
}
bool isCheckmate(const std::array<std::array<std::optional<Piece>, 8>, 8>& board, PieceColor currentColor) {
    if (!isKingInCheck(board, currentColor)) return false;
    for (int r=0; r<8; ++r) for (int c=0; c<8; ++c) {
        if (board[r][c].has_value() && board[r][c]->color == currentColor) {
            std::vector<sf::Vector2i> possibleMovesForPiece = getPossibleMoves(board, r, c);
            for (const auto& move : possibleMovesForPiece) {
                std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board;
                std::optional<Piece> originalPieceOpt = tempBoard[r][c];
                if (originalPieceOpt.has_value()) {
                    tempBoard[move.y][move.x] = Piece(originalPieceOpt->type, originalPieceOpt->color, originalPieceOpt->sprite);
                    tempBoard[r][c].reset();
                    if (!isKingInCheck(tempBoard, currentColor)) return false;
                }
            }
        }
    }
    return true;
}

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

    sf::Text messageText(font); // 게임 중 메시지 (턴, 체크 등)
    messageText.setCharacterSize(20);
    messageText.setFillColor(sf::Color::Black);

    // 타이머 관련 UI
    sf::Text whiteTimerText(font, "00:00", 22);
    whiteTimerText.setFillColor(sf::Color::Black);
    sf::Text blackTimerText(font, "00:00", 22);
    blackTimerText.setFillColor(sf::Color::Black);

    // 오른쪽 패널 상단에 타이머 위치 설정
    // (x 좌표는 messageText와 유사하게, y 좌표는 위에서부터)
    float timerPadding = 15.f;
    sf::FloatRect wt_bounds = whiteTimerText.getLocalBounds();
    whiteTimerText.setPosition({
        BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds.size.x) / 2.f - wt_bounds.position.x,
        timerPadding - wt_bounds.position.y
    });
    sf::FloatRect bt_bounds = blackTimerText.getLocalBounds();
    blackTimerText.setPosition({
        BOARD_WIDTH + (BUTTON_PANEL_WIDTH - bt_bounds.size.x) / 2.f - bt_bounds.position.x,
        whiteTimerText.getPosition().y + wt_bounds.size.y + wt_bounds.position.y + 5.f - bt_bounds.position.y // 흰색 타이머 아래
    });


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

    // 게임 오버 팝업 UI
    sf::RectangleShape popupBackground({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 3.f});
    popupBackground.setFillColor(sf::Color(100, 100, 100, 220)); // 반투명 회색
    popupBackground.setOrigin({popupBackground.getSize().x / 2.f, popupBackground.getSize().y / 2.f});
    popupBackground.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f});

    sf::Text popupMessageText(font, "", 28);
    popupMessageText.setFillColor(sf::Color::White);
    // (위치는 gameMessageStr 설정 후 중앙 정렬)

    sf::RectangleShape homeButtonShape({150.f, 50.f});
    homeButtonShape.setFillColor(sf::Color(70, 130, 180)); // SteelBlue
    homeButtonShape.setOrigin({homeButtonShape.getSize().x / 2.f, homeButtonShape.getSize().y / 2.f});
    // (위치는 popupBackground 내부 하단에 설정)

    sf::Text homeButtonText(font, "HOME", 24);
    homeButtonText.setFillColor(sf::Color::White);
    // (위치는 homeButtonShape 내부 중앙에 설정)


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

    // 타이머 관련 변수
    sf::Time whiteTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
    sf::Time blackTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
    sf::Clock frameClock; // 프레임 간 시간 측정


    auto place_piece = [&](int r, int c, PieceType type, PieceColor piece_color, const std::string& name) {
        std::string key = (piece_color == PieceColor::White ? "w_" : "b_") + name;
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

    // resetGame에서 사용할 수 있도록 setupBoard를 람다로 다시 정의
    auto actualSetupBoard = [&]() {
        board_state = {};
        place_piece(7,0,PieceType::Rook,PieceColor::White,"rook"); place_piece(7,1,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,2,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,3,PieceType::Queen,PieceColor::White,"queen"); place_piece(7,4,PieceType::King,PieceColor::White,"king"); place_piece(7,5,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,6,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,7,PieceType::Rook,PieceColor::White,"rook");
        for(int c=0;c<8;++c)place_piece(6,c,PieceType::Pawn,PieceColor::White,"pawn");
        place_piece(0,0,PieceType::Rook,PieceColor::Black,"rook"); place_piece(0,1,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,2,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,3,PieceType::Queen,PieceColor::Black,"queen"); place_piece(0,4,PieceType::King,PieceColor::Black,"king"); place_piece(0,5,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,6,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,7,PieceType::Rook,PieceColor::Black,"rook");
        for(int c=0;c<8;++c)place_piece(1,c,PieceType::Pawn,PieceColor::Black,"pawn");
    };

    auto resetGame = [&]() {
        actualSetupBoard(); // setupBoard는 board_state를 초기화
        currentGameState = GameState::ChoosingPlayer;
        currentTurn = PieceColor::None;
        selectedPiecePos.reset();
        possibleMoves.clear();
        gameMessageStr = "";
        whiteTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        blackTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        // kingIsCurrentlyChecked = false; // 이 변수들은 지역변수이므로 자동 초기화
        // checkedKingCurrentPos = {-1,-1};
        frameClock.restart(); // 게임 시작 시 클럭 재시작
    };

    // setupBoard는 resetGame 내부에서 호출되므로, 여기서 직접 호출하지 않아도 됨.
    // 단, resetGame의 setupBoard 호출 부분을 유지해야 함.
    // setupBoard 정의를 resetGame보다 위로 옮기거나, resetGame이 setupBoard를 알고 있어야 함.
    // 여기서는 setupBoard를 먼저 정의했으므로 OK.

    auto setupBoardLambda = [&]() { // 람다 이름 변경 및 resetGame에서 사용하기 위해
        board_state = {};
        place_piece(7,0,PieceType::Rook,PieceColor::White,"rook"); place_piece(7,1,PieceType::Knight,PieceColor::White,"knight"); /* ... 나머지 기물 ... */ place_piece(7,7,PieceType::Rook,PieceColor::White,"rook");
        for(int c=0;c<8;++c)place_piece(6,c,PieceType::Pawn,PieceColor::White,"pawn");
        place_piece(0,0,PieceType::Rook,PieceColor::Black,"rook"); /* ... 나머지 기물 ... */ place_piece(0,7,PieceType::Rook,PieceColor::Black,"rook");
        for(int c=0;c<8;++c)place_piece(1,c,PieceType::Pawn,PieceColor::Black,"pawn");
    };



    // resetGame 정의 시 actualSetupBoard를 사용하도록 수정
    auto actualResetGame = [&]() {
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

    actualSetupBoard(); // 최초 게임 보드 설정
    frameClock.restart(); // 최초 게임 시작 시 클럭 재시작

    while (window.isOpen()) {
        sf::Time deltaTime = frameClock.restart(); // 매 프레임 시작 시 델타 타임 계산
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        if (currentGameState == GameState::Playing && currentTurn != PieceColor::None) {
            // 시간 차감 로직
            if (currentTurn == PieceColor::White) {
                if (whiteTimeLeft > sf::Time::Zero) whiteTimeLeft -= deltaTime;
                if (whiteTimeLeft <= sf::Time::Zero) {
                    whiteTimeLeft = sf::Time::Zero; // 음수 방지
                    currentGameState = GameState::GameOver;
                    gameMessageStr = "Black wins on time!";
                }
            } else if (currentTurn == PieceColor::Black) {
                if (blackTimeLeft > sf::Time::Zero) blackTimeLeft -= deltaTime;
                if (blackTimeLeft <= sf::Time::Zero) {
                    blackTimeLeft = sf::Time::Zero; // 음수 방지
                    currentGameState = GameState::GameOver;
                    gameMessageStr = "White wins on time!";
                }
            }

            // 시간 초과가 아닌 경우에만 체크/체크메이트 판별 및 메시지 업데이트
            if (currentGameState != GameState::GameOver) {
                 kingIsCurrentlyChecked = isKingInCheck(board_state, currentTurn);
                if (kingIsCurrentlyChecked) {
                    checkedKingCurrentPos = findKing(board_state, currentTurn);
                    if (isCheckmate(board_state, currentTurn)) {
                        currentGameState = GameState::GameOver;
                        gameMessageStr = std::string((currentTurn == PieceColor::White) ? "Black" : "White") + " wins by Checkmate!";
                    } else {
                        gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " King is in Check!";
                    }
                } else {
                    gameMessageStr = std::string((currentTurn == PieceColor::White) ? "White" : "Black") + " to move";
                }
            }
        } else if (currentGameState == GameState::ChoosingPlayer) {
            gameMessageStr = "";
        }
        // GameOver 메시지는 시간 초과 또는 체크메이트 시 이미 설정됨

        whiteTimerText.setString("White: " + formatTime(whiteTimeLeft));
        blackTimerText.setString("Black: " + formatTime(blackTimeLeft));
        // 타이머 텍스트 위치 재조정 (매번 문자열 변경 시 크기가 바뀔 수 있으므로)
        wt_bounds = whiteTimerText.getLocalBounds();
        whiteTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - wt_bounds.size.x) / 2.f - wt_bounds.position.x, timerPadding - wt_bounds.position.y});
        bt_bounds = blackTimerText.getLocalBounds();
        blackTimerText.setPosition({ BOARD_WIDTH + (BUTTON_PANEL_WIDTH - bt_bounds.size.x) / 2.f - bt_bounds.position.x, whiteTimerText.getPosition().y + wt_bounds.size.y + wt_bounds.position.y + 5.f - bt_bounds.position.y });


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
                            gameMessageStr = "White to move"; frameClock.restart(); // 게임 시작 시 클럭 재시작
                        } else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::Black; currentGameState = GameState::Playing;
                            gameMessageStr = "Black to move"; frameClock.restart(); // 게임 시작 시 클럭 재시작
                        }
                    } else if (currentGameState == GameState::Playing) {
                        int clickedCol = mousePos.x / TILE_SIZE; int clickedRow = mousePos.y / TILE_SIZE;
                        if (clickedCol >=0 && clickedCol < 8 && clickedRow >=0 && clickedRow < 8) {
                            bool moved = false;
                            if (selectedPiecePos.has_value()) {
                                for (const auto& move : possibleMoves) {
                                    if (move.x == clickedCol && move.y == clickedRow) {
                                        std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board_state;
                                        std::optional<Piece> pieceToMoveOpt = tempBoard[selectedPiecePos->y][selectedPiecePos->x];
                                        if (pieceToMoveOpt.has_value()){
                                            tempBoard[move.y][move.x] = Piece(pieceToMoveOpt->type, pieceToMoveOpt->color, pieceToMoveOpt->sprite);
                                            tempBoard[selectedPiecePos->y][selectedPiecePos->x].reset();
                                        }
                                        if (!isKingInCheck(tempBoard, currentTurn)) { // 이 수로 인해 자신이 체크 상태가 되지 않는다면
                                            std::optional<Piece> actualPieceToMoveOpt = board_state[selectedPiecePos->y][selectedPiecePos->x];
                                            if(actualPieceToMoveOpt.has_value()){
                                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                                board_state[selectedPiecePos->y][selectedPiecePos->x].reset();
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
                                            frameClock.restart(); // 턴 변경 시 클럭 재시작 (다음 턴 시간 측정 위해)
                                            // 메시지 업데이트는 루프 상단에서 처리
                                            break;
                                        } else gameMessageStr = "Invalid move: King would be in check!"; // 이 메시지는 거의 즉시 다음 프레임에 덮어쓰여질 수 있음
                                    }
                                }
                            }
                            if (moved) {
                                selectedPiecePos.reset(); possibleMoves.clear();
                            } else { // 이동하지 않았다면, 새로운 기물 선택 또는 선택 해제 시도
                                if (board_state[clickedRow][clickedCol].has_value() && board_state[clickedRow][clickedCol]->color == currentTurn) {
                                    if (selectedPiecePos.has_value() && selectedPiecePos->x == clickedCol && selectedPiecePos->y == clickedRow) {
                                        selectedPiecePos.reset(); possibleMoves.clear(); // 같은 기물 다시 클릭: 선택 해제
                                    } else { // 새로운 아군 기물 선택
                                        selectedPiecePos = sf::Vector2i(clickedCol, clickedRow);
                                        std::vector<sf::Vector2i> rawPossibleMoves = getPossibleMoves(board_state, clickedRow, clickedCol);
                                        possibleMoves.clear();
                                        std::optional<Piece> pieceForSimOpt = board_state[clickedRow][clickedCol];
                                        for (const auto& move : rawPossibleMoves) {
                                            std::array<std::array<std::optional<Piece>, 8>, 8> tempBoard = board_state;
                                            if(pieceForSimOpt.has_value()){
                                                tempBoard[move.y][move.x] = Piece(pieceForSimOpt->type, pieceForSimOpt->color, pieceForSimOpt->sprite);
                                                tempBoard[clickedRow][clickedCol].reset();
                                            }
                                            if (!isKingInCheck(tempBoard, currentTurn)) possibleMoves.push_back(move);
                                        }
                                    }
                                } else { // 빈 칸 또는 상대방 기물 클릭: 선택 해제
                                    selectedPiecePos.reset(); possibleMoves.clear();
                                }
                            }
                        } else { selectedPiecePos.reset(); possibleMoves.clear(); } // 보드 바깥 클릭
                    } else if (currentGameState == GameState::GameOver) { // 게임 오버 상태에서 HOME 버튼 클릭 처리
                         if (homeButtonShape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            actualResetGame();
                         }
                    }
                }
            }
        }

        window.clear(sf::Color(240,240,240));

        if (currentGameState == GameState::ChoosingPlayer) {
            window.draw(chooseSidePromptText);
            window.draw(whiteStartButton); window.draw(whiteStartText);
            window.draw(blackStartButton); window.draw(blackStartText);
        } else { // Playing or GameOver
            // 보드 및 기물 그리기
            for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
                bool isLight = (r + c) % 2 == 0;
                tile.setFillColor(isLight ? lightColor : darkColor);
                if (kingIsCurrentlyChecked && checkedKingCurrentPos.x == c && checkedKingCurrentPos.y == r) {
                     tile.setFillColor(checkedKingTileColor);
                } else if (selectedPiecePos.has_value() && selectedPiecePos->x == c && selectedPiecePos->y == r) {
                    tile.setFillColor(sf::Color(255, 255, 0, 150));
                } else {
                    for (const auto& move : possibleMoves) {
                        if (move.x == c && move.y == r) {
                            if (board_state[r][c].has_value() && board_state[r][c]->color != currentTurn)
                                tile.setFillColor(sf::Color(50, 150, 250, 150));
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
                    Piece& piece = board_state[r_idx][c_idx].value();
                    // 게임오버 시 패배한 킹 (시간초과 또는 체크메이트 당한 킹) 색상 변경
                    bool isLosingKing = (currentGameState == GameState::GameOver &&
                                         piece.type == PieceType::King &&
                                         piece.color == currentTurn); // currentTurn은 게임이 끝날 때의 턴 (즉, 진 사람의 턴)
                    if (isLosingKing) {
                         piece.sprite.setColor(sf::Color::Red);
                    } else {
                         piece.sprite.setColor(sf::Color::White);
                    }
                    window.draw(piece.sprite);
                }
            }

            // 오른쪽 패널 UI (타이머, 메시지)
            window.draw(whiteTimerText);
            window.draw(blackTimerText);

            if (!gameMessageStr.empty()) {
                messageText.setString(gameMessageStr);
                sf::FloatRect gameMsgLocalBounds = messageText.getLocalBounds(); // 변수명 변경
                messageText.setPosition({
                    BOARD_WIDTH + (BUTTON_PANEL_WIDTH - gameMsgLocalBounds.size.x) / 2.f - gameMsgLocalBounds.position.x,
                    (WINDOW_HEIGHT - gameMsgLocalBounds.size.y) / 2.f - gameMsgLocalBounds.position.y
                });

                messageText.setFillColor(sf::Color::Black);
                messageText.setStyle(sf::Text::Regular);
                if(currentGameState == GameState::GameOver || gameMessageStr.find("wins by") != std::string::npos ) { // GameOver 또는 승리 메시지
                    messageText.setFillColor(sf::Color::Red); messageText.setStyle(sf::Text::Bold);
                } else if (gameMessageStr.find("Check!") != std::string::npos ) { // 체크 메시지
                    messageText.setFillColor(sf::Color(200,0,0)); messageText.setStyle(sf::Text::Bold);
                }
                window.draw(messageText);
            }

            // 게임 오버 팝업
            if (currentGameState == GameState::GameOver) {
                window.draw(popupBackground);

                popupMessageText.setString(gameMessageStr); // 이미 설정된 게임 오버 메시지 사용
                sf::FloatRect popupMsgBounds = popupMessageText.getLocalBounds();
                popupMessageText.setPosition({
                    popupBackground.getPosition().x - popupMsgBounds.size.x / 2.f - popupMsgBounds.position.x,
                    popupBackground.getPosition().y - popupBackground.getSize().y / 2.f + 30.f - popupMsgBounds.position.y
                });
                window.draw(popupMessageText);

                homeButtonShape.setPosition({
                    popupBackground.getPosition().x,
                    popupBackground.getPosition().y + popupBackground.getSize().y / 2.f - 40.f - homeButtonShape.getSize().y / 2.f
                });
                window.draw(homeButtonShape);

                sf::FloatRect homeButtonTextBounds = homeButtonText.getLocalBounds();
                homeButtonText.setPosition({
                    homeButtonShape.getPosition().x - homeButtonTextBounds.size.x / 2.f - homeButtonTextBounds.position.x,
                    homeButtonShape.getPosition().y - homeButtonTextBounds.size.y / 2.f - homeButtonTextBounds.position.y
                });
                window.draw(homeButtonText);
            }
        }
        window.display();
    }
    return 0;
}