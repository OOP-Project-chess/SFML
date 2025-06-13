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
#include <algorithm>
#include "SharedState.hpp"

using json = nlohmann::json;
using boost::asio::ip::tcp;
using namespace std;
std::queue<std::string> messageQueue;
std::mutex messageMutex;
PieceColor myColor = PieceColor::None;

int main() {
    NetworkClient client("10.2.19.156", 1234);

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
    backgroundSprite.setScale({
        static_cast<float>(WINDOW_WIDTH) / backgroundTexture.getSize().x,
        static_cast<float>(WINDOW_HEIGHT) / backgroundTexture.getSize().y
    });

    sf::Texture logoTexture;
    if (!logoTexture.loadFromFile("Textures/LOGO.png")) {
        std::cerr << "Failed to load Textures/LOGO.png" << std::endl;
        return -1;
    }
    sf::Sprite logoSprite(logoTexture);
    logoSprite.setScale({0.5f, 0.5f});
    sf::FloatRect logoBounds = logoSprite.getLocalBounds();
    logoSprite.setOrigin({logoBounds.size.x / 2.f, logoBounds.size.y / 2.f});
    logoSprite.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f - 170.f});

    sf::Text messageText(font);
    messageText.setCharacterSize(20);
    messageText.setFillColor(sf::Color::Black);

    sf::Text whiteTimerText(font, "00:00", 26);
    sf::Color TimerFontColor(54, 26, 9);
    whiteTimerText.setFillColor(TimerFontColor);
    sf::Text blackTimerText(font, "00:00", 26);
    blackTimerText.setFillColor(TimerFontColor);

    float timerPadding = 250.f;
    float interTimerSpacing = 20.f;

    sf::Texture startButtonTexture;
    if (!startButtonTexture.loadFromFile("Textures/START.png")) {
        std::cerr << "Failed to load Textures/START.png" << std::endl;
        return -1;
    }
    sf::Sprite startButtonSprite(startButtonTexture);
    startButtonSprite.setScale({0.5f, 0.5f});
    sf::FloatRect startSpriteBounds = startButtonSprite.getLocalBounds();
    startButtonSprite.setOrigin({startSpriteBounds.size.x / 2.f, startSpriteBounds.size.y / 2.f});
    startButtonSprite.setPosition({
        WINDOW_WIDTH / 2.f,
        logoSprite.getPosition().y + logoSprite.getGlobalBounds().size.y / 2.f + startButtonSprite.getGlobalBounds().size.y / 2.f - 10.f
    });

    sf::Texture uiPanelBgTexture;
    if (!uiPanelBgTexture.loadFromFile("Textures/side.png")) {
        std::cerr << "Failed to load Textures/side.png" << std::endl;
    }
    sf::Sprite uiPanelBgSprite(uiPanelBgTexture);
    uiPanelBgSprite.setPosition({static_cast<float>(BOARD_WIDTH), 0.f});
    if (uiPanelBgTexture.getSize().x > 0 && uiPanelBgTexture.getSize().y > 0) {
        uiPanelBgSprite.setScale({
            static_cast<float>(BUTTON_PANEL_WIDTH) / uiPanelBgTexture.getSize().x,
            static_cast<float>(WINDOW_HEIGHT) / uiPanelBgTexture.getSize().y
        });
    }

    sf::Texture player1Texture;
    if (!player1Texture.loadFromFile("Textures/Player1.png")) {
        std::cerr << "Failed to load Textures/Player1.png" << std::endl;
    }
    sf::Sprite player1Sprite(player1Texture);
    player1Sprite.setScale({0.5f, 0.5f});
    sf::FloatRect p1Bounds = player1Sprite.getLocalBounds();
    player1Sprite.setOrigin({p1Bounds.size.x / 2.f, p1Bounds.size.y / 2.f - 195.f});
    player1Sprite.setPosition({BOARD_WIDTH + BUTTON_PANEL_WIDTH / 2.f, WINDOW_HEIGHT * 2.f / 3.f });
    sf::Texture player2Texture;
    if (!player2Texture.loadFromFile("Textures/Player2.png")) {
        std::cerr << "Failed to load Textures/Player2.png" << std::endl;
    }
    sf::Sprite player2Sprite(player2Texture);
    player2Sprite.setScale({0.5f, 0.5f});
    sf::FloatRect p2Bounds = player2Sprite.getLocalBounds();
    player2Sprite.setOrigin({p2Bounds.size.x / 2.f, p2Bounds.size.y / 2.f + 270.f});
    player2Sprite.setPosition({BOARD_WIDTH + BUTTON_PANEL_WIDTH / 2.f, WINDOW_HEIGHT / 3.f});

    sf::Texture waitingTexture;
    if (!waitingTexture.loadFromFile("Textures/waiting.png")) {
        std::cerr << "Failed to load Textures/waiting.png" << std::endl;
    }

    sf::Text player1NameText(font, "Player 1", 48);
    player1NameText.setFillColor(sf::Color(85, 19, 0));
    sf::FloatRect p1NameBounds = player1NameText.getLocalBounds();
    player1NameText.setOrigin({p1NameBounds.position.x + p1NameBounds.size.x / 2.f, p1NameBounds.position.y + p1NameBounds.size.y / 2.f - 175.f});
    player1NameText.setPosition({player1Sprite.getPosition().x, player1Sprite.getPosition().y + player1Sprite.getGlobalBounds().size.y / 2.f});

    sf::Text player2NameText(font, "Player 2", 48);
    player2NameText.setFillColor(sf::Color(85, 19, 0));
    sf::FloatRect p2NameBounds = player2NameText.getLocalBounds();
    player2NameText.setOrigin({p2NameBounds.position.x + p2NameBounds.size.x / 2.f, p2NameBounds.position.y + p2NameBounds.size.y / 2.f + 55.f});
    player2NameText.setPosition({player2Sprite.getPosition().x, player2Sprite.getPosition().y + player2Sprite.getGlobalBounds().size.y / 2.f});

    sf::RectangleShape blackStartButton;
    sf::Text blackStartText(font, "");
    blackStartText.setFillColor(sf::Color::Transparent);

    sf::Texture popupTexture;
    if (!popupTexture.loadFromFile("Textures/popup.png")) {
        std::cerr << "Failed to load Textures/popup.png" << std::endl;
        return -1;
    }
    sf::Sprite popupImageSprite(popupTexture);
    popupImageSprite.setScale({0.6f, 0.6f});
    sf::FloatRect popupImageBounds = popupImageSprite.getLocalBounds();
    popupImageSprite.setOrigin({popupImageBounds.size.x / 2.f, popupImageBounds.size.y / 2.f});
    popupImageSprite.setPosition({WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f});

    sf::Text popupMessageText(font, "", 28);
    popupMessageText.setFillColor(sf::Color::White);

    sf::Texture homeButtonTexture;
    if (!homeButtonTexture.loadFromFile("Textures/HOME.png")) {
        std::cerr << "Failed to load Textures/HOME.png" << std::endl;
        return -1;
    }
    sf::Sprite homeButtonSprite(homeButtonTexture);
    homeButtonSprite.setScale({0.3f, 0.3f});
    sf::FloatRect homeBounds = homeButtonSprite.getLocalBounds();
    homeButtonSprite.setOrigin({homeBounds.size.x / 2.f, homeBounds.size.y / 2.f});

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
        sprite.setScale({0.25f, 0.25f});
        sf::FloatRect sprite_bounds = sprite.getGlobalBounds();
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

    gameLoop(
        window, font, tile, lightColor, darkColor, checkedKingTileColor,
        messageText, whiteTimerText, blackTimerText,
        startButtonSprite,
        blackStartButton, blackStartText,
        popupImageSprite,
        popupMessageText,
        homeButtonSprite,
        currentGameState, selectedPiecePos, possibleMoves, currentTurn, gameMessageStr,
        textures, board_state, whiteTimeLeft, blackTimeLeft, frameClock,
        actualResetGame_lambda,
        socket, myColor,
        timerPadding,
        interTimerSpacing,
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

    return 0;
}
