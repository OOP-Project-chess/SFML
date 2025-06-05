#include "GameData.hpp"
#include "GameLoop.hpp"
#include "NetworkClient.hpp"
#include <SFML/Graphics.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>
#include <queue>
#include <mutex>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "SharedState.hpp"

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;
std::queue<std::string> messageQueue;
std::mutex messageMutex;
PieceColor myColor = PieceColor::None;

int main() {
    NetworkClient client("10.2.6.60", 1234);

    client.startReceiving([&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(messageMutex);
        messageQueue.push(msg);

        try {
            auto parsed = json::parse(msg);
            if (parsed["type"] == "assignColor") {
                std::string color = parsed["color"];
                myColor = (color == "white") ? PieceColor::White : PieceColor::Black;
            } else if (parsed["type"] == "turn") {
                std::string turn = parsed["currentTurn"];
                std::cout << "Rotation: " << turn << '\n';
            }
        } catch (...) {
            cout << "Error parsing json: " << msg << '\n';
        }
    });

    tcp::socket& socket = client.getSocket();
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Chess Game Project");
    window.setFramerateLimit(60);

    sf::RectangleShape tile{ {static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)} };
    sf::Color lightColor{ 255, 231, 193 };
    sf::Color darkColor{ 120, 77, 51 };
    sf::Color checkedKingTileColor{255, 0, 0, 180};

    sf::Font font;
    std::filesystem::path fontPath = std::filesystem::path("Textures") / "BMJUA_ttf.ttf";
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


    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("Textures/background.png")) {
        std::cerr << "Failed to load Textures/background.png" << std::endl;
        return -1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    // 배경 스프라이트의 크기를 창 크기에 맞게 조절 (선택 사항)
    backgroundSprite.setScale({ // 중괄호로 sf::Vector2f 생성
        static_cast<float>(WINDOW_WIDTH) / backgroundTexture.getSize().x,
        static_cast<float>(WINDOW_HEIGHT) / backgroundTexture.getSize().y
    });

    /*sf::Text chooseSidePromptText(font);
    chooseSidePromptText.setString("Click to start");
    chooseSidePromptText.setCharacterSize(30);
    chooseSidePromptText.setFillColor(sf::Color::Black);
    sf::FloatRect chooseSideBounds = chooseSidePromptText.getLocalBounds();
    chooseSidePromptText.setPosition({
        (WINDOW_WIDTH - chooseSideBounds.size.x) / 2.f - chooseSideBounds.position.x, // Adjusted for width and left
        WINDOW_HEIGHT / 2.f - 120.f - chooseSideBounds.position.y // Adjusted for top
    });*/

    sf::Texture logoTexture;
    if (!logoTexture.loadFromFile("Textures/LOGO.png")) {
        std::cerr << "Failed to load Textures/LOGO.png" << std::endl;
        return -1;
    }
    sf::Sprite logoSprite(logoTexture);

    logoSprite.setScale({0.5f, 0.5f});

    sf::FloatRect logoBounds = logoSprite.getLocalBounds();
    logoSprite.setOrigin({logoBounds.size.x / 2.f, logoBounds.size.y / 2.f}); // 중괄호로 sf::Vector2f 생성
    logoSprite.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f - 170.f}); // 로고를 살짝 위로 이동 (선택 사항)


    sf::Text messageText(font);
    messageText.setCharacterSize(20);
    messageText.setFillColor(sf::Color::Black);

    sf::Text whiteTimerText(font, "00:00", 26);
    sf::Color TimerFontColor(54, 26, 9); // 타이머 폰트 색 설정
    whiteTimerText.setFillColor(TimerFontColor);
    sf::Text blackTimerText(font, "00:00", 26);
    blackTimerText.setFillColor(TimerFontColor);

    float timerPadding = 250.f; // 타이머 위치

    // float buttonWidth = BUTTON_PANEL_WIDTH - 80; // No longer used for whiteStartButton
    // float buttonHeight = 50.f; // No longer used for whiteStartButton

    // sf::RectangleShape whiteStartButton({buttonWidth, buttonHeight}); // Replaced by sprite
    // whiteStartButton.setFillColor(sf::Color(220, 220, 220));
    // whiteStartButton.setPosition({ // Position logic moved to sprite
    //     (WINDOW_WIDTH - buttonWidth) / 2.f,
    //     chooseSidePromptText.getPosition().y + chooseSidePromptText.getGlobalBounds().size.y + 30.f // Adjusted based on chooseSidePromptText global bounds
    // });

    // sf::Text whiteStartText(font, "START", 24); // Replaced by sprite
    // whiteStartText.setFillColor(sf::Color::Black);
    // sf::FloatRect whiteTextBounds = whiteStartText.getLocalBounds();
    // whiteStartText.setPosition({ // Position logic moved to sprite
    //     whiteStartButton.getPosition().x + (whiteStartButton.getSize().x - whiteTextBounds.size.x) / 2.f - whiteTextBounds.position.x,
    //     whiteStartButton.getPosition().y + (whiteStartButton.getSize().y - whiteTextBounds.size.y) / 2.f - whiteTextBounds.position.y
    // });

    sf::Texture startButtonTexture;
    if (!startButtonTexture.loadFromFile("Textures/START.png")) {
        std::cerr << "Failed to load Textures/START.png" << std::endl;
        return -1;
    }
    sf::Sprite startButtonSprite(startButtonTexture);

    startButtonSprite.setScale({0.5f, 0.5f});

    // Adjust scale if needed, e.g., startButtonSprite.setScale(0.5f, 0.5f);
    sf::FloatRect startSpriteBounds = startButtonSprite.getLocalBounds();
    startButtonSprite.setOrigin({startSpriteBounds.size.x / 2.f, startSpriteBounds.size.y / 2.f
}); // Set origin to center for easier positioning
    startButtonSprite.setPosition({
        WINDOW_WIDTH / 2.f,
        logoSprite.getPosition().y + logoSprite.getGlobalBounds().size.y / 2.f + startButtonSprite.getGlobalBounds().size.y / 2.f - 10.f // 로고 아래에 배치
    });


    sf::Texture uiPanelBgTexture;
    // ▼▼▼ 파일 이름 수정 ▼▼▼
    if (!uiPanelBgTexture.loadFromFile("Textures/side.png")) {
        std::cerr << "Failed to load Textures/side.png" << std::endl;
        // 파일 로드 실패 시 오류 처리 (예: 기본 색상으로 대체 또는 프로그램 종료)
    }
    sf::Sprite uiPanelBgSprite(uiPanelBgTexture);
    uiPanelBgSprite.setPosition({static_cast<float>(BOARD_WIDTH), 0.f}); // UI 패널 위치 (보드 오른쪽)

    // UI 패널 배경 이미지 크기를 패널 크기(BUTTON_PANEL_WIDTH x WINDOW_HEIGHT)에 맞게 조절
    if (uiPanelBgTexture.getSize().x > 0 && uiPanelBgTexture.getSize().y > 0) { // 텍스처 로드 성공 여부 확인
        uiPanelBgSprite.setScale({
            static_cast<float>(BUTTON_PANEL_WIDTH) / uiPanelBgTexture.getSize().x,
            static_cast<float>(WINDOW_HEIGHT) / uiPanelBgTexture.getSize().y
        });
    }


    // Black start button remains for now, though its functionality might be reviewed
    float blackButtonWidth = BUTTON_PANEL_WIDTH - 80;
    float blackButtonHeight = 50.f;
    sf::RectangleShape blackStartButton({blackButtonWidth, blackButtonHeight});
    // blackStartButton.setFillColor(sf::Color(80, 80, 80)); // Original color
    blackStartButton.setPosition({
        (WINDOW_WIDTH - blackButtonWidth) / 2.f,
        startButtonSprite.getPosition().y + startSpriteBounds.size.y / 2.f + 20.f + blackButtonHeight / 2.f // Position below new start sprite
    });
    // Making black button invisible as requested for the general START button
    blackStartButton.setFillColor(sf::Color::Transparent);
    blackStartButton.setOutlineThickness(0);
    blackStartButton.setSize({0.f, 0.f});


    // sf::Text blackStartText(font, "Start as Black", 24); // Original text
    sf::Text blackStartText(font, "", 24); // Keep text object but make it empty / transparent
    // blackStartText.setFillColor(sf::Color::White); // Original color
    sf::FloatRect blackTextBounds = blackStartText.getLocalBounds();
    // blackStartText.setPosition({ // Original position logic
    //     blackStartButton.getPosition().x + (blackStartButton.getSize().x - blackTextBounds.size.x) / 2.f - blackTextBounds.position.x,
    //     blackStartButton.getPosition().y + (blackStartButton.getSize().y - blackTextBounds.size.y) / 2.f - blackTextBounds.position.y
    // });
    blackStartText.setFillColor(sf::Color::Transparent); // Make text transparent


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
        sprite.setScale({0.15f, 0.15f}); // Ensure scale is appropriate
        sf::FloatRect sprite_bounds = sprite.getGlobalBounds(); // Use getGlobalBounds after scaling
        float x_offset = (static_cast<float>(TILE_SIZE) - sprite_bounds.size.x) / 2.f;
        float y_offset = (static_cast<float>(TILE_SIZE) - sprite_bounds.size.y) / 2.f;
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
        currentGameState = GameState::ChoosingPlayer; // Reset to choosing player
        currentTurn = PieceColor::None;
        selectedPiecePos.reset();
        possibleMoves.clear();
        gameMessageStr = ""; // Clear message
        whiteTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        blackTimeLeft = sf::seconds(INITIAL_TIME_SECONDS);
        frameClock.restart();
    };

    actualSetupBoard();
    frameClock.restart();

    gameLoop(
        window, font, tile, lightColor, darkColor, checkedKingTileColor,
        messageText, whiteTimerText, blackTimerText,
        startButtonSprite,
        blackStartButton, blackStartText,
        popupBackground, popupMessageText, homeButtonShape, homeButtonText,
        currentGameState, selectedPiecePos, possibleMoves, currentTurn, gameMessageStr,
        textures, board_state, whiteTimeLeft, blackTimeLeft, frameClock,
        actualResetGame_lambda,
        socket, myColor,
        timerPadding,
        backgroundSprite,      // 새로 추가된 배경 스프라이트 인자
        logoSprite,             // 새로 추가된 로고 스프라이트 인자
        uiPanelBgSprite // << UI 패널 배경 스프라이트 (side.png 로드됨)
    );

    return 0;
}