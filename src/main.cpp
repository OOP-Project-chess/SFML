#include <SFML/Graphics.hpp>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <iostream> // for std::cerr
#include <filesystem> // For paths

// 전역 상수 정의
const int TILE_SIZE = 100;
const int BOARD_WIDTH = 8 * TILE_SIZE;
const int BOARD_HEIGHT = 8 * TILE_SIZE;
const int BUTTON_PANEL_WIDTH = 300;
const int WINDOW_WIDTH = BOARD_WIDTH + BUTTON_PANEL_WIDTH;
const int WINDOW_HEIGHT = BOARD_HEIGHT;

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

int main() {
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Chess Game Prj (SFML 3.0.x)");
    window.setFramerateLimit(60);

    sf::RectangleShape tile{ {static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)} };
    sf::Color lightColor{ 207, 202, 198 };
    sf::Color darkColor{ 64, 58, 53 };
    sf::Color checkedKingTileColor{255, 0, 0, 180};

    sf::Font font;
    // 에셋 경로를 "Textures"로 통일
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
        // Y 위치 조정을 위한 부분: 아래 값(-120.f)을 변경하여 전체 그룹 위치 조정 가능
        WINDOW_HEIGHT / 2.f + 80.f - chooseSideBounds.position.y // -120.f면 정중앙
    });

    sf::Text messageText(font);
    messageText.setCharacterSize(20);
    messageText.setFillColor(sf::Color::Black);

    float buttonWidth = BUTTON_PANEL_WIDTH - 80;
    float buttonHeight = 50.f;

    sf::RectangleShape whiteStartButton({buttonWidth, buttonHeight});
    whiteStartButton.setFillColor(sf::Color(220, 220, 220));
    whiteStartButton.setPosition({
        (WINDOW_WIDTH - buttonWidth) / 2.f,
        // Y 위치 조정을 위한 부분: 아래 값(+ 30.f)은 chooseSidePromptText와의 간격
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
        // Y 위치 조정을 위한 부분: 아래 값(+ 20.f)은 whiteStartButton과의 간격
        whiteStartButton.getPosition().y + whiteStartButton.getSize().y + 20.f
    });

    sf::Text blackStartText(font, "Start as Black", 24);
    blackStartText.setFillColor(sf::Color::White);
    sf::FloatRect blackTextBounds = blackStartText.getLocalBounds();
    blackStartText.setPosition({
        blackStartButton.getPosition().x + (blackStartButton.getSize().x - blackTextBounds.size.x) / 2.f - blackTextBounds.position.x,
        blackStartButton.getPosition().y + (blackStartButton.getSize().y - blackTextBounds.size.y) / 2.f - blackTextBounds.position.y
    });

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
            // 에셋 경로를 "Textures"로 통일
            std::filesystem::path texturePath = std::filesystem::path("Textures") / (key + ".png");
            if (!tex.loadFromFile(texturePath)) {
                std::cerr << "Failed to load texture: " << texturePath.string() << std::endl;
            }
            textures[key] = std::move(tex);
        }
    }

    std::array<std::array<std::optional<Piece>, 8>, 8> board_state;

    auto place_piece = [&](int r, int c, PieceType type, PieceColor piece_color, const std::string& name) {
        std::string key = (piece_color == PieceColor::White ? "w_" : "b_") + name;
        if (textures.count(key) == 0 || textures[key].getSize().x == 0) {
            std::cerr << "Texture for key '" << key << "' not found or invalid!" << std::endl;
            return;
        }
        sf::Sprite sprite(textures[key]);
        sprite.setScale({0.5f, 0.5f});
        sf::FloatRect spriteBounds = sprite.getGlobalBounds();
        // sf::FloatRect 크기 접근: .size.x, .size.y 사용
        float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
        float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
        sprite.setPosition({c * static_cast<float>(TILE_SIZE) + x_offset, r * static_cast<float>(TILE_SIZE) + y_offset});
        board_state[r][c] = Piece{type, piece_color, sprite};
    };

    auto setupBoard = [&]() {
        board_state = {};
        place_piece(7,0,PieceType::Rook,PieceColor::White,"rook"); place_piece(7,1,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,2,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,3,PieceType::Queen,PieceColor::White,"queen"); place_piece(7,4,PieceType::King,PieceColor::White,"king"); place_piece(7,5,PieceType::Bishop,PieceColor::White,"bishop"); place_piece(7,6,PieceType::Knight,PieceColor::White,"knight"); place_piece(7,7,PieceType::Rook,PieceColor::White,"rook");
        for(int c=0;c<8;++c)place_piece(6,c,PieceType::Pawn,PieceColor::White,"pawn");
        place_piece(0,0,PieceType::Rook,PieceColor::Black,"rook"); place_piece(0,1,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,2,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,3,PieceType::Queen,PieceColor::Black,"queen"); place_piece(0,4,PieceType::King,PieceColor::Black,"king"); place_piece(0,5,PieceType::Bishop,PieceColor::Black,"bishop"); place_piece(0,6,PieceType::Knight,PieceColor::Black,"knight"); place_piece(0,7,PieceType::Rook,PieceColor::Black,"rook");
        for(int c=0;c<8;++c)place_piece(1,c,PieceType::Pawn,PieceColor::Black,"pawn");
    };

    setupBoard();

    while (window.isOpen()) {
        bool kingIsCurrentlyChecked = false;
        sf::Vector2i checkedKingCurrentPos = {-1, -1};

        if (currentGameState == GameState::Playing && currentTurn != PieceColor::None) {
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
        } else if (currentGameState == GameState::ChoosingPlayer) {
            gameMessageStr = "";
        }


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
                            gameMessageStr = "White to move";
                        } else if (blackStartButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentTurn = PieceColor::Black; currentGameState = GameState::Playing;
                            gameMessageStr = "Black to move";
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
                                        if (!isKingInCheck(tempBoard, currentTurn)) {
                                            std::optional<Piece> actualPieceToMoveOpt = board_state[selectedPiecePos->y][selectedPiecePos->x];
                                            if(actualPieceToMoveOpt.has_value()){
                                                board_state[clickedRow][clickedCol] = Piece(actualPieceToMoveOpt->type, actualPieceToMoveOpt->color, actualPieceToMoveOpt->sprite);
                                                board_state[selectedPiecePos->y][selectedPiecePos->x].reset();
                                            }
                                            if (board_state[clickedRow][clickedCol].has_value()) {
                                                sf::Sprite& movedSprite = board_state[clickedRow][clickedCol]->sprite;
                                                sf::FloatRect spriteBounds = movedSprite.getGlobalBounds();
                                                // sf::FloatRect 크기 접근: .size.x, .size.y 사용
                                                float x_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.x) / 2.f;
                                                float y_offset = (static_cast<float>(TILE_SIZE) - spriteBounds.size.y) / 2.f;
                                                movedSprite.setPosition({clickedCol*static_cast<float>(TILE_SIZE)+x_offset, clickedRow*static_cast<float>(TILE_SIZE)+y_offset});
                                            }
                                            moved = true;
                                            currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
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
                                } else { selectedPiecePos.reset(); possibleMoves.clear(); }
                            }
                        } else { selectedPiecePos.reset(); possibleMoves.clear(); }
                    }
                }
            }
        }

        window.clear(sf::Color(240,240,240));

        if (currentGameState == GameState::ChoosingPlayer) {
            window.draw(chooseSidePromptText);
            window.draw(whiteStartButton); window.draw(whiteStartText);
            window.draw(blackStartButton); window.draw(blackStartText);
        } else {
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
                    if (currentGameState == GameState::GameOver && piece.type == PieceType::King && piece.color == currentTurn)
                         piece.sprite.setColor(sf::Color::Red);
                    else piece.sprite.setColor(sf::Color::White);
                    window.draw(piece.sprite);
                }
            }

            if (!gameMessageStr.empty()) {
                messageText.setString(gameMessageStr);
                sf::FloatRect gameMsgBounds = messageText.getLocalBounds();
                messageText.setPosition({
                    BOARD_WIDTH + (BUTTON_PANEL_WIDTH - gameMsgBounds.size.x) / 2.f - gameMsgBounds.position.x,
                    (WINDOW_HEIGHT - gameMsgBounds.size.y) / 2.f - gameMsgBounds.position.y
                });

                messageText.setFillColor(sf::Color::Black);
                messageText.setStyle(sf::Text::Regular);
                if(currentGameState == GameState::GameOver) {
                    messageText.setFillColor(sf::Color::Red); messageText.setStyle(sf::Text::Bold);
                } else if (gameMessageStr.find("Check!") != std::string::npos && gameMessageStr.find("Checkmate!") == std::string::npos) {
                    messageText.setFillColor(sf::Color(200,0,0)); messageText.setStyle(sf::Text::Bold);
                }
                window.draw(messageText);
            }
        }
        window.display();
    }
    return 0;
}
