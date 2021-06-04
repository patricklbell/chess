#include <constants.hpp>

Bitboard random_bitboard() {
    Bitboard u1, u2, u3, u4;
    u1 = (Bitboard)(rand()) & 0xFFFF; u2 = (Bitboard)(rand()) & 0xFFFF;
    u3 = (Bitboard)(rand()) & 0xFFFF; u4 = (Bitboard)(rand()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

Bitboard random_bitboard_fewbits() {
    return random_bitboard() & random_bitboard() & random_bitboard();
}

const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

// Removes 1st bit and returns its position
int pop_1st_bit(Bitboard* bb) {
    Bitboard b = *bb ^ (*bb - 1);
    unsigned int fold = (unsigned)((b & 0xffffffff) ^ (b >> 32));
    *bb &= (*bb - 1);
    return BitTable[(fold * 0x783a9b23) >> 26];
}

// Create permuation from its index by masking index's bits
Bitboard index_to_Bitboard(int index, int bits, Bitboard m) {
    int i, j;
    Bitboard result = 0ULL;
    for (i = 0; i < bits; i++) {
        j = pop_1st_bit(&m);
        if (index & (1 << i)) result |= (1ULL << j);
    }
    return result;
}

Bitboard make_rook_mask(Square sq) {
    Bitboard result = 0ULL;
    int rk = sq / 8, fl = sq % 8, r, f;
    for (r = rk + 1; r <= 6; r++) result |= (1ULL << (fl + r * 8));
    for (r = rk - 1; r >= 1; r--) result |= (1ULL << (fl + r * 8));
    for (f = fl + 1; f <= 6; f++) result |= (1ULL << (f + rk * 8));
    for (f = fl - 1; f >= 1; f--) result |= (1ULL << (f + rk * 8));
    return result;
}

Bitboard make_bishop_mask(Square sq) {
    Bitboard result = 0ULL;
    int rk = sq / 8, fl = sq % 8, r, f;
    for (r = rk + 1, f = fl + 1; r <= 6 && f <= 6; r++, f++) result |= (1ULL << (f + r * 8));
    for (r = rk + 1, f = fl - 1; r <= 6 && f >= 1; r++, f--) result |= (1ULL << (f + r * 8));
    for (r = rk - 1, f = fl + 1; r >= 1 && f <= 6; r--, f++) result |= (1ULL << (f + r * 8));
    for (r = rk - 1, f = fl - 1; r >= 1 && f >= 1; r--, f--) result |= (1ULL << (f + r * 8));
    return result;
}

Bitboard make_rook_attacks(Square sq, Bitboard block) {
    Bitboard result = 0ULL;
    int rk = rank_of(sq) , fl = file_of(sq), r, f;

    // Advance in each direction until collision
    for (r = rk + 1; r <= 7; r++) {
        result |= (1ULL << (fl + r * 8));
        if (block & (1ULL << (fl + r * 8))) break;
    }
    for (r = rk - 1; r >= 0; r--) {
        result |= (1ULL << (fl + r * 8));
        if (block & (1ULL << (fl + r * 8))) break;
    }
    for (f = fl + 1; f <= 7; f++) {
        result |= (1ULL << (f + rk * 8));
        if (block & (1ULL << (f + rk * 8))) break;
    }
    for (f = fl - 1; f >= 0; f--) {
        result |= (1ULL << (f + rk * 8));
        if (block & (1ULL << (f + rk * 8))) break;
    }
    return result;
}

Bitboard make_bishop_attacks(Square sq, Bitboard block) {
    Bitboard result = 0ULL;
    int rk = rank_of(sq), fl = file_of(sq), r, f;

    // Advance in each direction until collision
    for (r = rk + 1, f = fl + 1; r <= 7 && f <= 7; r++, f++) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk + 1, f = fl - 1; r <= 7 && f >= 0; r++, f--) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk - 1, f = fl + 1; r >= 0 && f <= 7; r--, f++) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    for (r = rk - 1, f = fl - 1; r >= 0 && f >= 0; r--, f--) {
        result |= (1ULL << (f + r * 8));
        if (block & (1ULL << (f + r * 8))) break;
    }
    return result;
}

// Apply magic to bitboard
int magic_bitboard(Bitboard b, Bitboard magic, int bits) {
#ifndef IS_64BIT
    return
        (unsigned)((int)b * (int)magic ^ (int)(b >> 32) * (int)(magic >> 32)) >> (32 - bits);
#else
    return (int)((b * magic) >> (64 - bits));
#endif
}

// Finds a magic hashing function for a single square
MagicBitboard make_magic_bitboard(Square sq, bool is_bishop) {
    MagicBitboard mb;
    is_bishop ? mb.mask = make_bishop_mask(sq) : mb.mask = make_rook_mask(sq);

    mb.bits = count_bits(mb.mask);

    // Number of permutations of mask bits
    int perms = 1 << mb.bits;

    // Create all possible permuations
    Bitboard *p = new Bitboard[perms], *a = new Bitboard[perms];
    for (int i = 0; i < perms; i++) {
        p[i] = index_to_Bitboard(i, mb.bits, mb.mask);
        is_bishop ? a[i] = make_bishop_attacks(sq, p[i]) : a[i] = make_rook_attacks(sq, p[i]);
    }

    uint64_t magic;
    std::vector<Bitboard> used(perms);
    for (int k = 0; k < 100000000; k++) {
        // Create our magic number
        magic = random_bitboard_fewbits();

        // Magicking mask must lead to no moves
        if (count_bits((mb.mask * magic) & 0xFF00000000000000ULL) < 6) continue;
        
        // Clear used attacks
        for (int i = 0; i < perms; i++) used[i] = 0ULL;

        // Check all permutations to ensure magic hash doesn't collide
        bool fail = false;
        for (int i = 0; !fail && i < perms; i++) {
            // Get the attack for this permutation
            int j = magic_bitboard(p[i], magic, mb.bits);

            // Check whether we have encountered this yet
            if (used[j] == 0ULL) {
                used[j] = a[i];
            }
            // If we have, check hash didn't collide
            else if (used[j] != a[i]) {
                fail = true;
            }
        }
        if (!fail) {
            mb.attacks = used;
            mb.magic = magic;
            return mb;
        }
    }
    std::cout << "Failed to produce hash after 100000000 iterations" << std::endl;
    return mb;
}

MagicBitboard make_magic_bitboard_constant(Square sq, uint64_t magic, bool is_bishop) {
    MagicBitboard mb;
    mb.magic = magic;
    is_bishop ? mb.mask = make_bishop_mask(sq) : mb.mask = make_rook_mask(sq);
    is_bishop ? mb.bits = BISHOP_DISTANCES[sq] : mb.bits = ROOK_DISTANCES[sq];

    // Number of permutations of mask bits
    int perms = 1 << mb.bits;

    // Create all possible permuations
    Bitboard *p = new Bitboard[perms], *a = new Bitboard[perms];
    for (int i = 0; i < perms; i++) {
        p[i] = index_to_Bitboard(i, mb.bits, mb.mask);
        is_bishop ? a[i] = make_bishop_attacks(sq, p[i]) : a[i] = make_rook_attacks(sq, p[i]);
    }

    std::vector<Bitboard> used(perms);

    // Fill permutations with correct attacks
    for (int i = 0; i < perms; i++) {
        // Get the attack for this permutation
        int j = magic_bitboard(p[i], mb.magic, mb.bits);

        used[j] = a[i];
    }
    mb.attacks = used;
    return mb;
}

// Precomputed magics
const uint64_t BISHOP_MAGICS[64] = {
1213929573326976ULL,
600113481890409472ULL,
577658137729826816ULL,
581263702564339872ULL,
73188029430236424ULL,
90637150651090960ULL,
72136776661213188ULL,
4901359538151694336ULL,
74938864370816ULL,
4521200681292288ULL,
4578933421391874ULL,
144119603311166592ULL,
5260512538353734656ULL,
146772167053411360ULL,
36596153630073112ULL,
288230943096321152ULL,
1447336199325760ULL,
1196337647452227ULL,
4613400037066670592ULL,
9077585195573248ULL,
288511886025555968ULL,
70514778309656ULL,
2450802630829088768ULL,
2900883622540214833ULL,
2256198934218752ULL,
649189117292709925ULL,
1442278914805792804ULL,
18018797797574672ULL,
4629981960951840768ULL,
1441232145141006593ULL,
578722447755378976ULL,
4785358105616544ULL,
1178951344198848ULL,
74328102732301313ULL,
577024458173318020ULL,
2310390623668144640ULL,
290279659676160ULL,
21401040352060032ULL,
76562294530180096ULL,
1185010236606448648ULL,
218714891723227136ULL,
4612814435453764224ULL,
35270305058848ULL,
27030531581417472ULL,
5307511960723653120ULL,
27031012877860929ULL,
148621295983005760ULL,
9570703359688968ULL,
565566125899848ULL,
6917811606980133004ULL,
162134126500512002ULL,
692035534850ULL,
1153203804489974274ULL,
2305879851478941728ULL,
2287019652827144ULL,
166916868806935081ULL,
602533513930890ULL,
4611721481981272128ULL,
4556376756015105ULL,
2314855156308640264ULL,
864691403888804096ULL,
19140642586757184ULL,
2295789205864968ULL,
2359922497551352192ULL,
};
const uint64_t ROOK_MAGICS[64] = {
36029071906791552ULL,
1171006272128950272ULL,
5224210760980236672ULL,
5944760304490513538ULL,
144119594712829984ULL,
2377909403639743488ULL,
4791830553294799360ULL,
72068881841259264ULL,
11540615787446336ULL,
4972888783367180292ULL,
146648600321998872ULL,
1238771415721713920ULL,
2533309151315200ULL,
75998346866720784ULL,
11540491258560768ULL,
2306406015031313042ULL,
36037867991993408ULL,
4503875579027458ULL,
144399962124337172ULL,
576480543684821024ULL,
83599167728914948ULL,
6992120996115513346ULL,
671037448284930052ULL,
36092568706286212ULL,
36029076194001024ULL,
35194036814144ULL,
9007475207440384ULL,
4618451326360096768ULL,
5188710863146590212ULL,
2306124522845372418ULL,
333302673490051752ULL,
2416199350618178561ULL,
18084767798919296ULL,
612560055509716996ULL,
4508032035725312ULL,
433542975373447200ULL,
2306406098384458768ULL,
37717664067551360ULL,
4792392970756163595ULL,
2306124485280923778ULL,
36028934462652416ULL,
148618925188464640ULL,
4619039556493180948ULL,
1409642636967945ULL,
150870609142808592ULL,
4611756404620394784ULL,
2233400033296ULL,
11544873173909521ULL,
4647785184802898560ULL,
360323155636520000ULL,
2918354686224384256ULL,
4692786271137829376ULL,
111466839624122496ULL,
613615518486103041ULL,
36318516216832ULL,
576747206305120768ULL,
4900198008069685378ULL,
70927092088850ULL,
18691833988161ULL,
2310346746348177665ULL,
576742261640135185ULL,
5191805962616440833ULL,
585485827231449220ULL,
1153207390808580418ULL,
};

void make_move_attacks(std::vector<std::pair<int, int>> offsets, Bitboard map[64]) {
	for (Square sq = SQ_A1; sq < SQ_NONE; ++sq)
	{
		Bitboard b = 0;
		for (auto& off : offsets)
			if (rank_of(sq) + off.second >= 0 && rank_of(sq) + off.second < 8 &&
                file_of(sq) + off.first >= 0 && file_of(sq) + off.first < 8)
				b |= 1ULL << (int)(sq) + off.first + off.second * 8;
		map[sq] = b;
	}
}

MoveGenerationConstants init_move_generation_constants(bool calculate_magic)
{
	MoveGenerationConstants mgc;

    // Make king and knight tables
	auto knight_offsets = std::vector<std::pair<int, int>>{ {2, 1}, {-2, 1}, {2, -1}, {-2, -1}, {1, 2}, {-1, 2}, {1, -2}, {-1, -2} };
	auto king_offsets = std::vector<std::pair<int, int>>{ {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1} };
	make_move_attacks(knight_offsets, mgc.knight_attacks);
	make_move_attacks(king_offsets, mgc.king_attacks);

	// Make bishop and rook tables
	for (Square sq = SQ_A1; sq < SQ_NONE; ++sq)
	{
        calculate_magic ? mgc.bishop_magic[sq] = make_magic_bitboard(sq, true) :
            mgc.bishop_magic[sq] = make_magic_bitboard_constant(sq, BISHOP_MAGICS[sq], true);
        calculate_magic ? mgc.rook_magic[sq] = make_magic_bitboard(sq, false) :
            mgc.rook_magic[sq] = make_magic_bitboard_constant(sq, ROOK_MAGICS[sq], false);
	}


	return mgc;
}
