#include "GameData.hpp"     // GameState, Piece, 상수 등 모든 정의 포함
#include "GameLogic.hpp"    // getPossibleMoves 등 게임 로직 함수
#include "GameLoop.hpp"     // gameLoop 함수 선언 포함

#include <SFML/Graphics.hpp>
// Boost.Asio 관련 헤더는 네트워크 기능 추가 시 다시 포함
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>

#include <iostream> // for std::cerr, std::cout
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <map>
// #include <thread> // 네트워크 기능 추가 시 필요할 수 있음
// #include <chrono> // 네트워크 기능 추가 시 필요할 수 있음
#include <filesystem> // For paths
#include <iomanip>    // For std::setfill, std::setw (formatTime 함수가 GameLoop.cpp로 이동)
#include <sstream>    // For std::ostringstream (formatTime 함수가 GameLoop.cpp로 이동)

using boost::asio::ip::tcp;
using namespace std; // 사용자 코드 스타일 유지

// formatTime 함수는 GameLoop.cpp로 이동했습니다.

int main() {
    boost::asio::io_context io;

    tcp::socket socket(io);
    // 수정된 부분
    tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234);
    socket.connect(endpoint);



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

    float timerPadding = 15.f;

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
        socket,
        timerPadding
    );

    return 0;
}
