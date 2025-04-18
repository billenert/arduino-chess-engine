/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "types.h"

Value PieceValue[2][16] = {
{ 0, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
{ 0, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg } };

uint32_t NonPawnPieceValue[16];

#define S(mg, eg) make_score(mg, eg)

// Bonus[PieceType][Square / 2] contains Piece-Square scores. For each
// piece type on a given square a (middlegame, endgame) score pair is
// assigned. Table is defined for files A..D and white side: it is
// symmetric for black side and second half of the files.

static const Score Bonus[][8][4] = {
  {{0}},
  {{0}},
  { // Knight
    { S(-175, -96), S(-92,-65), S(-74,-49), S(-73,-21) },
    { S( -77, -67), S(-41,-54), S(-27,-18), S(-15,  8) },
    { S( -61, -40), S(-17,-27), S(  6, -8), S( 12, 29) },
    { S( -35, -35), S(  8, -2), S( 40, 13), S( 49, 28) },
    { S( -34, -45), S( 13,-16), S( 44,  9), S( 51, 39) },
    { S(  -9, -51), S( 22,-44), S( 58,-16), S( 53, 17) },
    { S( -67, -69), S(-27,-50), S(  4,-51), S( 37, 12) },
    { S(-201,-100), S(-83,-88), S(-56,-56), S(-26,-17) }
  },
  { // Bishop
    { S(-53,-57), S( -5,-30), S( -8,-37), S(-23,-12) },
    { S(-15,-37), S(  8,-13), S( 19,-17), S(  4,  1) },
    { S( -7,-16), S( 21, -1), S( -5, -2), S( 17, 10) },
    { S( -5,-20), S( 11, -6), S( 25,  0), S( 39, 17) },
    { S(-12,-17), S( 29, -1), S( 22,-14), S( 31, 15) },
    { S(-16,-30), S(  6,  6), S(  1,  4), S( 11,  6) },
    { S(-17,-31), S(-14,-20), S(  5, -1), S(  0,  1) },
    { S(-48,-46), S(  1,-42), S(-14,-37), S(-23,-24) }
  },
  { // Rook
    { S(-31, -9), S(-20,-13), S(-14,-10), S(-5, -9) },
    { S(-21,-12), S(-13, -9), S( -8, -1), S( 6, -2) },
    { S(-25,  6), S(-11, -8), S( -1, -2), S( 3, -6) },
    { S(-13, -6), S( -5,  1), S( -4, -9), S(-6,  7) },
    { S(-27, -5), S(-15,  8), S( -4,  7), S( 3, -6) },
    { S(-22,  6), S( -2,  1), S(  6, -7), S(12, 10) },
    { S( -2,  4), S( 12,  5), S( 16, 20), S(18, -5) },
    { S(-17, 18), S(-19,  0), S( -1, 19), S( 9, 13) }
  },
  { // Queen
    { S(  3,-69), S(-5,-57), S(-5,-47), S( 4,-26) },
    { S( -3,-55), S( 5,-31), S( 8,-22), S(12, -4) },
    { S( -3,-39), S( 6,-18), S(13, -9), S( 7,  3) },
    { S(  4,-23), S( 5, -3), S( 9, 13), S( 8, 24) },
    { S(  0,-29), S(14, -6), S(12,  9), S( 5, 21) },
    { S( -4,-38), S(10,-18), S( 6,-12), S( 8,  1) },
    { S( -5,-50), S( 6,-27), S(10,-24), S( 8, -8) },
    { S( -2,-75), S(-2,-52), S( 1,-43), S(-2,-36) }
  },
  { // King
    { S(271,  1), S(327, 45), S(271, 85), S(198, 76) },
    { S(278, 53), S(303,100), S(234,133), S(179,135) },
    { S(195, 88), S(258,130), S(169,169), S(120,175) },
    { S(164,103), S(190,156), S(138,172), S( 98,172) },
    { S(154, 96), S(179,166), S(105,199), S( 70,199) },
    { S(123, 92), S(145,172), S( 81,184), S( 31,191) },
    { S( 88, 47), S(120,121), S( 65,116), S( 33,131) },
    { S( 59, 11), S( 89, 59), S( 45, 73), S( -1, 78) }
  }
};

static const Score PBonus[8][8] = {
  { 0 },
  { S(  3,-10), S(  3, -6), S( 10, 10), S( 19,  0), S( 16, 14), S( 19,  7), S(  7, -5), S( -5,-19) },
  { S( -9,-10), S(-15,-10), S( 11,-10), S( 15,  4), S( 32,  4), S( 22,  3), S(  5, -6), S(-22, -4) },
  { S( -8,  6), S(-23, -2), S(  6, -8), S( 20, -4), S( 40,-13), S( 17,-12), S(  4,-10), S(-12, -9) },
  { S( 13,  9), S(  0,  4), S(-13,  3), S(  1,-12), S( 11,-12), S( -2, -6), S(-13, 13), S(  5,  8) },
  { S( -5, 28), S(-12, 20), S( -7, 21), S( 22, 28), S( -8, 30), S( -5,  7), S(-15,  6), S(-18, 13) },
  { S( -7,  0), S(  7,-11), S( -3, 12), S(-13, 21), S(  5, 25), S(-16, 19), S( 10,  4), S( -8,  7) }
};

#undef S

struct PSQT psqt;

// init() initializes piece-square tables: the white halves of the tables
// are copied from Bonus[] adding the piece value, then the black  halves
// of the tables are initialized by flipping and changing the sign of the
// white scores.

void psqt_init(void) {
  for (int pt = PAWN; pt <= KING; pt++) {
    PieceValue[MG][make_piece(BLACK, pt)] = PieceValue[MG][pt];
    PieceValue[EG][make_piece(BLACK, pt)] = PieceValue[EG][pt];

    Score score = make_score(PieceValue[MG][pt], PieceValue[EG][pt]);

    for (Square s = 0; s < 64; s++) {
      int f = min(file_of(s), FILE_H - file_of(s));
      psqt.psq[make_piece(WHITE, pt)][s] =
        score + (type_of_p(pt) == PAWN ? PBonus[rank_of(s)][file_of(s)]
                                       : Bonus[pt][rank_of(s)][f]);
      psqt.psq[make_piece(BLACK, pt)][s ^ 0x38] =
        -psqt.psq[make_piece(WHITE, pt)][s];
    }
  }
  union {
    uint16_t val[2];
    uint32_t combi;
  } tmp;
  NonPawnPieceValue[W_PAWN] = NonPawnPieceValue[B_PAWN] = 0;
  for (int pt = KNIGHT; pt < KING; pt++) {
    tmp.val[0] = PieceValue[MG][pt];
    tmp.val[1] = 0;
    NonPawnPieceValue[pt] = tmp.combi;
    tmp.val[0] = 0;
    tmp.val[1] = PieceValue[MG][pt];
    NonPawnPieceValue[pt + 8] = tmp.combi;
  }
}
