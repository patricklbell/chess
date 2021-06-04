#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <algorithm>
#include <iostream>
#include <types.hpp>

int magic_bitboard(Bitboard b, Bitboard magic, int bits);

const Bitboard PAWN_DOUBLE[2] = {0xFF000000, 0xFF00000000 };
const int ROOK_DISTANCES[64] = {
	12, 11, 11, 11, 11, 11, 11, 12,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	12, 11, 11, 11, 11, 11, 11, 12
};
const int BISHOP_DISTANCES[64] = {
	6, 5, 5, 5, 5, 5, 5, 6,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 5, 5, 5, 5, 5, 5, 6
};
struct MagicBitboard {
	std::vector<Bitboard> attacks;
	Bitboard mask;
	uint64_t magic;
	int bits;
} typedef MagicBitboard;

struct MoveGenerationConstants 
{
	Bitboard knight_attacks[64], king_attacks[64];
	MagicBitboard rook_magic[64], bishop_magic[64];
} typedef MoveGenerationConstants;

MoveGenerationConstants init_move_generation_constants(bool);

#endif // !CONSTANTS_H_INCLUDED