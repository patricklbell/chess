#include <types.hpp>

std::string bitboard_to_string(Bitboard b) {
    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r)
    {
        for (File f = FILE_A; f < FILE_NB; ++f)
            s += (b >> (make_square(f, r) - SQ_A1) & 1U ? "| X " : "|   ");

        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
    }
    s += "  a   b   c   d   e   f   g   h\n";

    return s;
}

int index_of(const Piece& p) {
    return (color_of(p) == WHITE) ? p - W_PAWN : p - B_PAWN + W_KING;
}
Piece from_index(int i) {
    return (i > (W_KING - W_PAWN)) ? Piece(i - W_KING + B_PAWN) : Piece(i - W_PAWN);
}

/**
 * bitScanForward
 * @author Martin Läuter (1997)
 *         Charles E. Leiserson
 *         Harald Prokop
 *         Keith H. Randall
 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
int bitScanForward(const Bitboard& bb) {
    static const int index64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
       61, 58, 50, 42, 38, 29, 17,  4,
       62, 55, 59, 36, 53, 51, 43, 22,
       45, 39, 33, 30, 24, 18, 12,  5,
       63, 47, 56, 27, 60, 41, 37, 16,
       54, 35, 52, 21, 44, 32, 23, 11,
       46, 26, 40, 15, 34, 20, 31, 10,
       25, 14, 19,  9, 13,  8,  7,  6
    };
    static const Bitboard debruijn64 = 0x03f79d71b4cb0a89;
    return index64[((bb & -bb) * debruijn64) >> 58];
}

std::vector<Square> serialize_bitboard(Bitboard b) {
    std::vector<Square> list;
    if (b) {
        do {
            int idx = bitScanForward(b); // square index from 0..63
            list.push_back(Square(idx + SQ_A1));
        } while (b &= b - 1); // reset LS1B
    }
    return list;
}

std::vector<Move> bitboard_to_moves(const Bitboard& b, Square from, MoveType mt) {
    std::vector<Move> moves;
    for (auto& to : serialize_bitboard(b))
    {
        moves.push_back(Move((int)make_move(from, to) | mt));
    }
    return moves;
}
