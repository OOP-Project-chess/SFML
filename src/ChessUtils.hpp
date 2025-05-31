#pragma once
#include <string>
#include "GameData.hpp"  // PieceType 정의가 필요

// 보드 좌표를 체스 표기법 (예: e2)으로 변환
std::string toChessNotation(int col, int row);

// 말 타입을 문자열로 변환 (예: "king", "queen")
std::string pieceTypeToString(PieceType type);