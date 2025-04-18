/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include <assert.h>

#include "bitboard.h"
#include "pawns.h"
#include "position.h"
#include "thread.h"

#define V(v) ((Value)(v))
#define S(mg, eg) make_score(mg, eg)

// Pawn penalties
static const Score Backward      = S( 9, 24);
static const Score Doubled       = S(11, 56);
static const Score Isolated      = S( 5, 15);
static const Score WeakLever     = S( 0, 56);
static const Score WeakUnopposed = S(13, 27);

// Connected pawn bonus by opposed, phalanx, #support and rank
static Score Connected[2][2][3][8];

// Strength of pawn shelter for our king by [distance from edge][rank].
// RANK_1 = 0 is used for files where we have no pawn, or pawn is behind
// our king.
static const Value ShelterStrength[4][8] = {
  { V( -6), V( 81), V( 93), V( 58), V( 39), V( 18), V(  25) },
  { V(-43), V( 61), V( 35), V(-49), V(-29), V(-11), V( -63) },
  { V(-10), V( 75), V( 23), V( -2), V( 32), V(  3), V( -45) },
  { V(-39), V(-13), V(-29), V(-52), V(-48), V(-67), V(-166) }
};

// Danger of enemry pawns moving toward our king by [distance from edge][rank].
// RANK_1 = 0 is used for files where the enemy has no pawn or where their
// pawn is behind our king. Note that UnblockedStorm[0][1-2] accommodates
// opponent pawn on edge, likely blocked by our king.
static const Value UnblockedStorm[4][8] = {
  { V( 85), V(-289), V(-166), V(97), V(50), V( 45), V( 50) },
  { V( 46), V( -25), V( 122), V(45), V(37), V(-10), V( 20) },
  { V( -6), V(  51), V( 168), V(34), V(-2), V(-22), V(-14) },
  { V(-15), V( -11), V( 101), V( 4), V(11), V(-15), V(-29) }
};

#undef S
#undef V

INLINE Score pawn_evaluate(const Pos *pos, PawnEntry *e, const int Us)
{
  const int Them  = (Us == WHITE ? BLACK      : WHITE);
  const int Up    = (Us == WHITE ? NORTH      : SOUTH);

  Bitboard neighbours, stoppers, doubled, support, phalanx, opposed;
  Bitboard lever, leverPush, blocked;
  Square s;
  bool backward, passed;
  Score score = SCORE_ZERO;

  Bitboard ourPawns   = pieces_cp(Us, PAWN);
  Bitboard theirPawns = pieces_p(PAWN) ^ ourPawns;

  Bitboard doubleAttackThem = pawn_double_attacks_bb(theirPawns, Them);

  e->passedPawns[Us] = 0;
  e->semiopenFiles[Us] = 0xFF;
  e->kingSquares[Us] = SQ_NONE;
  e->pawnAttacks[Us] = e->pawnAttacksSpan[Us] = pawn_attacks_bb(ourPawns, Us);
  e->pawnsOnSquares[Us][BLACK] = popcount(ourPawns & DarkSquares);
  e->pawnsOnSquares[Us][WHITE] = popcount(ourPawns & LightSquares);

  // Loop through all pawns of the current color and score each pawn
  loop_through_pieces(Us, PAWN, s) {
    assert(piece_on(s) == make_piece(Us, PAWN));

    uint32_t f = file_of(s);

    e->semiopenFiles[Us] &= ~(1 << f);

    // Flag the pawn
    opposed    = theirPawns & forward_file_bb(Us, s);
    blocked    = theirPawns & sq_bb(s + Up);
    stoppers   = theirPawns & passed_pawn_span(Us, s);
    lever      = theirPawns & PawnAttacks[Us][s];
    leverPush  = theirPawns & PawnAttacks[Us][s + Up];
    doubled    = ourPawns   & sq_bb(s - Up);
    neighbours = ourPawns   & adjacent_files_bb(f);
    phalanx    = neighbours & rank_bb_s(s);
    support    = neighbours & rank_bb_s(s - Up);

    // A pawn is backward when it is behind all pawns of the same color on
    // the adjacent files and cannot safely advance.
    backward =   !(neighbours & forward_ranks_bb(Them, rank_of(s + Up)))
              &&  (leverPush | blocked);

    // Compute additional span if pawn is neither backward nor blocked
    if (!backward && !blocked)
      e->pawnAttacksSpan[Us] |= pawn_attack_span(Us, s);

    // A pawn is passed if one of the three following conditions is true:
    // (a) there are no stoppers except some levers
    // (b) the only stoppers are the leverPush, but we outnumber them
    // (c) there is only one front stopper which can be levered
    passed =   !(stoppers ^ lever)
            || (   !(stoppers ^ leverPush)
                && popcount(phalanx) >= popcount(leverPush))
            || (   stoppers == blocked && relative_rank_s(Us, s) >= RANK_5
                && (shift_bb(Up, support) & ~(theirPawns | doubleAttackThem)));

    // Passed pawns will be properly scored later in evaluation when we have
    // full attack info.
    if (passed)
      e->passedPawns[Us] |= sq_bb(s);

    // Score this pawn
    if (support | phalanx)
      score += Connected[!!opposed][!!phalanx][popcount(support)][relative_rank_s(Us, s)];

    else if (!neighbours)
      score -= Isolated + (opposed ? 0 : WeakUnopposed);

    else if (backward)
      score -= Backward + (opposed ? 0 : WeakUnopposed);

    if (!support)
      score -=  (doubled ? Doubled : 0)
              + (more_than_one(lever) ? WeakLever : 0);
  }

  return score;
}


// pawn_init() initializes some tables needed by evaluation.

void pawn_init(void)
{
  static const int Seed[7] = { 0, 7, 8, 12, 29, 48, 86 };

  for (int opposed = 0; opposed < 2; opposed++)
    for (int phalanx = 0; phalanx < 2; phalanx++)
      for (int support = 0; support <= 2; support++)
        for (int r = RANK_2; r < RANK_8; ++r) {
          int v = Seed[r] * (2 + (!!phalanx) - opposed) + 21 * support;
          Connected[opposed][phalanx][support][r] = make_score(v, v * (r-2) / 4);
      }
}


// pawns_probe() looks up the current position's pawns configuration in
// the pawns hash table.

void pawn_entry_fill(const Pos *pos, PawnEntry *e, Key key)
{
  e->key = key;
  e->score = pawn_evaluate(pos, e, WHITE) - pawn_evaluate(pos, e, BLACK);
  e->openFiles = popcount(e->semiopenFiles[WHITE] & e->semiopenFiles[BLACK]);
  e->passedCount = popcount(e->passedPawns[WHITE] | e->passedPawns[BLACK]);
}


// evaluate_shelter() calculates the shelter bonus and the storm penalty
// for a king, by looking at the king file and the two closest files.

INLINE void evaluate_shelter(const Pos *pos, Square ksq, Score *shelter,
                             const int Us)
{
  const int Them = (Us == WHITE ? BLACK : WHITE);
  
  Bitboard b =  pieces_p(PAWN) & ~forward_ranks_bb(Them, rank_of(ksq));
  Bitboard ourPawns = b & pieces_c(Us);
  Bitboard theirPawns = b & pieces_c(Them);
  Value bonus[] = { 5, 5 };

  File center = clamp(file_of(ksq), FILE_B, FILE_G);

  for (File f = center - 1; f <= center + 1; f++) {
    b = ourPawns & file_bb(f);
    int ourRank = b ? relative_rank_s(Us, backmost_sq(Us, b)) : 0;

    b = theirPawns & file_bb(f);
    int theirRank = b ? relative_rank_s(Us, frontmost_sq(Them, b)) : 0;

    int d = min(f, FILE_H - f);
    bonus[MG] += ShelterStrength[d][ourRank];

    if (ourRank && (ourRank == theirRank - 1)) {
      bonus[MG] -= 82 * (theirRank == RANK_3);
      bonus[EG] -= 82 * (theirRank == RANK_3);
    } else
      bonus[MG] -= UnblockedStorm[d][theirRank];
  }

  if (bonus[MG] > mg_value(*shelter))
    *shelter = make_score(bonus[MG], bonus[EG]);
}


// do_king_safety() calculates a bonus for king safety. It is called only
// when king square changes, which is about 20% of total king_safety() calls.

INLINE Score do_king_safety(PawnEntry *pe, const Pos *pos, Square ksq,
                                   const int Us)
{
  pe->kingSquares[Us] = ksq;
  pe->castlingRights[Us] = can_castle_c(Us);
  int minKingPawnDistance = 0;

  Bitboard pawns = pieces_cp(Us, PAWN);
  if (pawns)
    while (!(DistanceRingBB[ksq][++minKingPawnDistance] & pawns)) {}

  Score shelter = make_score(-VALUE_INFINITE, 0);
  evaluate_shelter(pos, ksq, &shelter, Us);

  // If we can castle use the bonus after the castling if it is bigger
  if (can_castle_cr(make_castling_right(Us, KING_SIDE)))
    evaluate_shelter(pos, relative_square(Us, SQ_G1), &shelter, Us);

  if (can_castle_cr(make_castling_right(Us, QUEEN_SIDE)))
    evaluate_shelter(pos, relative_square(Us, SQ_C1), &shelter, Us);

  return shelter - make_score(0, 16 * minKingPawnDistance);
}

// "template" instantiation:
Score do_king_safety_white(PawnEntry *pe, const Pos *pos, Square ksq)
{
  return do_king_safety(pe, pos, ksq, WHITE);
}

Score do_king_safety_black(PawnEntry *pe, const Pos *pos, Square ksq)
{
  return do_king_safety(pe, pos, ksq, BLACK);
}
