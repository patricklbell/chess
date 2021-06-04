#include <imgui.h>
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <string>
#include <vector>
#include <ctype.h>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <bitset>

#include <types.hpp>
#include <constants.hpp>

void recalculate_board(Board &board, const MoveGenerationConstants &mgc, const Color &c);
void generate_moves(Board &, const MoveGenerationConstants &, const Color &c);
void render_board(sf::RenderWindow &app, const Board &board, const GameState &gamestate, const UserSettings &user_settings, sf::Sprite piece_sprites[12]);
Square find_square_from_px(sf::RenderWindow &, Board &, GameState &, UserSettings &, sf::Vector2i &);
Board load_fen(std::string, GameState &);
std::string generate_fen(Board &, GameState &);

void render_overlay_bitboard(sf::RenderWindow &app, const Bitboard &bb, const sf::Color &col, const GameState &gamestate, const UserSettings &user_settings)
{
	float s = std::min(app.getSize().x * user_settings.board_max_horizontal_percentage, app.getSize().y * user_settings.board_max_vertical_percentage);
	auto offset = sf::Vector2f((app.getSize().x - s) / 2, (app.getSize().y - s) / 2);
	auto sqr_size = sf::Vector2f(s / 8, s / 8);
	auto sqr = sf::RectangleShape(sqr_size);
	for (auto &sq : serialize_bitboard(bb))
	{
		sqr.setPosition(offset.x + file_of(sq) * sqr_size.x, offset.y + (7 - rank_of(sq)) * sqr_size.y);
		sqr.setFillColor(col);
		app.draw(sqr);
	}
}
void render_board(sf::RenderWindow &app, const Board &board, const GameState &gamestate, const UserSettings &user_settings, sf::Sprite piece_sprites[12])
{
	float s = std::min(app.getSize().x * user_settings.board_max_horizontal_percentage, app.getSize().y * user_settings.board_max_vertical_percentage);
	std::string h_text = "abcdefgh", v_text = "12345678";
	sf::Text text;
	text.setFont(user_settings.font);
	int f_size = s / 16;
	text.setCharacterSize(f_size);
	text.setFillColor(user_settings.board_label_col);

	auto offset = sf::Vector2f((app.getSize().x - s) / 2, (app.getSize().y - s) / 2);
	auto sqr_size = sf::Vector2f(s / 8, s / 8);
	auto sqr = sf::RectangleShape(sqr_size);
	auto circle = sf::CircleShape(s / 32);
	circle.setFillColor(user_settings.move_circle_col);
	for (Rank r = RANK_1; r < RANK_NB; ++r)
	{
		for (File f = FILE_A; f < FILE_NB; ++f)
		{
			sqr.setPosition(offset.x + f * sqr_size.x, offset.y + (7 - r) * sqr_size.y);

			if (board.check &&
				(board.bitboards[index_of(make_piece(gamestate.color, KING))] >> make_square(f, r)) & 1ULL)
			{
				sqr.setFillColor(user_settings.check_squares_col);
			}
			else if (f == file_of(gamestate.selected_square) && r == rank_of(gamestate.selected_square))
			{
				sqr.setFillColor(user_settings.selected_squares_col);
			}
			else if ((r + f) % 2 == 0)
			{
				sqr.setFillColor(user_settings.black_squares_col);
			}
			else
			{
				sqr.setFillColor(user_settings.white_squares_col);
			}
			app.draw(sqr);

			// Check bit boards and draw pieces
			for (Piece p = W_PAWN; p <= B_KING; ++p)
			{
				if ((board.bitboards[index_of(p)] >> make_square(f, r)) & 1ULL)
				{
					piece_sprites[index_of(p)].setPosition(offset.x + f * sqr_size.x, offset.y + (7 - r) * sqr_size.y);
					auto size = piece_sprites[index_of(p)].getTexture()->getSize();
					piece_sprites[index_of(p)].setScale(sqr_size.x / size.x, sqr_size.y / size.y);
					app.draw(piece_sprites[index_of(p)]);
				}
			}
		}
		text.setString(v_text[r]);
		text.setPosition(offset.x - (f_size + 5), offset.y + (7 - r + 0.5) * sqr_size.y - f_size / 2 - 3);
		app.draw(text);

		if (r == 0)
		{
			for (size_t t_x = 0; t_x < 8; t_x++)
			{
				text.setString(h_text[t_x]);
				text.setPosition(offset.x + (t_x + 0.5) * sqr_size.x - f_size / 2, offset.y + 8.5 * sqr_size.y - f_size / 2);
				app.draw(text);
			}
		}
	}
	// Draw movable squares
	if (board.moves.find(gamestate.selected_square) != board.moves.end())
	{
		auto it = board.moves.find(gamestate.selected_square);
		if (it != board.moves.end())
		{
			for (auto &m : it->second)
			{
				auto to = to_sq(m);
				circle.setPosition(
					offset.x + (file_of(to) + 0.5) * sqr_size.x - circle.getRadius(),
					offset.y + (7 - rank_of(to) + 0.5) * sqr_size.y - circle.getRadius());
				app.draw(circle);
			}
		}
	}

	std::string buf(16, '\0');
	auto n_size = std::snprintf(&buf[0], buf.size(), "%.2f", gamestate.time_b);
	buf.resize(n_size);

	if (gamestate.color == WHITE)
	{
		text.setColor(user_settings.board_label_disabled_col);
	}
	text.setString(buf);
	text.setPosition(offset.x + 8.5 * sqr_size.x, offset.y + 3 * sqr_size.y + f_size / 2);
	app.draw(text);

	buf = std::string(16, '\0');
	n_size = std::snprintf(&buf[0], buf.size(), "%.2f", gamestate.time_w);
	buf.resize(n_size);

	if (gamestate.color == BLACK)
	{
		text.setColor(user_settings.board_label_disabled_col);
	}
	else
	{
		text.setColor(user_settings.board_label_col);
	}
	text.setString(buf);
	text.setPosition(offset.x + 8.5 * sqr_size.x, offset.y + 4 * sqr_size.y + f_size / 2);
	app.draw(text);
}

Square find_square_from_px(const sf::RenderWindow &app, const Board &board, const GameState &gamestate, const UserSettings &user_settings, const sf::Vector2i &px_coords)
{
	float s = std::min(app.getSize().x * user_settings.board_max_horizontal_percentage, app.getSize().y * user_settings.board_max_vertical_percentage);
	auto offset = sf::Vector2f((app.getSize().x - s) / 2, (app.getSize().y - s) / 2);
	auto sqr_size = sf::Vector2f(s / 8, s / 8);

	if (px_coords.x > offset.x && px_coords.x < offset.x + 8 * sqr_size.x && px_coords.y > offset.y && px_coords.y < offset.y + 8 * sqr_size.y)
	{
		return make_square(File((px_coords.x - offset.x) / sqr_size.x), Rank(8 - (px_coords.y - offset.y) / sqr_size.y));
	}
	return SQ_NONE;
}

Board load_fen(std::string fen, GameState &gamestate, const MoveGenerationConstants &mgc)
{
	Board board;

	std::vector<std::string> tokens;
	size_t prev = 0, str_pos = 0, i = 0;
	const std::string piece_characters = "PNBRQKpnbrqk";

	do
	{
		str_pos = fen.find(" ", prev);
		if (str_pos == std::string::npos)
			str_pos = fen.length();
		std::string token = fen.substr(prev, str_pos - prev);

		switch (i)
		{
		case 0:
		{
			Rank r = RANK_8;
			File f = FILE_A;
			for (auto &c : token)
			{
				if (isalpha(c))
				{
					board.bitboards[piece_characters.find(c)] |= 1ULL << make_square(f, r);
					++f;
				}
				else if (isdigit(c))
				{
					f = File(f + (int)c - 48);
				}
				else if (c == '/')
				{
					f = FILE_A;
					--r;
				}
			}
			break;
		}
		case 1:
			(token == "b") ? gamestate.color = BLACK : gamestate.color = WHITE;
			break;
		case 2:
			for (auto &c : token)
			{
				switch (c)
				{
				case 'K':
					board.castle = CastlingRights(board.castle | WHITE_OO);
				case 'Q':
					board.castle = CastlingRights(board.castle | WHITE_OOO);
				case 'k':
					board.castle = CastlingRights(board.castle | BLACK_OO);
				case 'q':
					board.castle = CastlingRights(board.castle | BLACK_OOO);
				default:
					break;
				}
			}
			break;
		case 3:
			if (token[0] != '-')
			{
				auto enp_b = 1ULL << make_square(File((int)token[0] - (int)'a' + FILE_A), Rank((int)token[1] - (int)'0' + RANK_1));
				gamestate.color == WHITE ? board.en_passant_target_w = enp_b : board.en_passant_target_b = enp_b;
			}
			break;
		case 4:
			gamestate.halfmove_clock = std::stoi(token);
			break;
		case 5:
			gamestate.fullmove_clock = std::stoi(token);
			break;
		default:
			break;
		}

		prev = str_pos + 1;
		i++;
	} while (str_pos < fen.length() && prev < fen.length());

	recalculate_board(board, mgc, ~gamestate.color);

	board.check = board.attacked & board.bitboards[index_of(make_piece(gamestate.color, KING))];
	board.block_check = 0ULL;

	// Note: pretty inefficient but doesn't really matter
	// Determine squCare which when blocked break check
	if (board.check)
	{
		Square king_sq = serialize_bitboard(board.bitboards[index_of(make_piece(gamestate.color, KING))])[0];

		// Check all atacks to find attacking piece
		PieceType mpt;
		Square att_sq;
		for (auto& sq_m : board.moves) {
			for (auto& m : sq_m.second) {
				if (to_sq(m) == king_sq) {
					att_sq = from_sq(m);
					for (PieceType pt = PAWN; pt <= KING; ++pt)
					{
						Bitboard from_bitboard = 1ULL << att_sq;
						if (board.bitboards[index_of(make_piece(~gamestate.color, pt))] & from_bitboard)
						{
							mpt = pt;
							break;
						}
					}
					goto end;
				}
			}
		}
		end:

		if (mpt == ROOK || mpt == BISHOP || mpt == QUEEN)
		{
			Direction d = direction_between_squares(king_sq, att_sq);

			// Add moved pieces attack squares if in correct direction
			for (auto& m : board.moves[att_sq])
			{
				Direction dm = direction_between_squares(to_sq(m), att_sq);
				if (dm == d)
				{
					board.block_check |= 1ULL << to_sq(m);
				}
			}
		}
		board.block_check |= 1ULL << att_sq;
	}

	recalculate_board(board, mgc, gamestate.color);

	return board;
}

void generate_moves(Board &board, const MoveGenerationConstants &mgc, const Color &c)
{
	board.moves.clear();
	Bitboard attacked = 0ULL;

	// Find pinables by treating king as queen
	auto king_sq = serialize_bitboard(board.bitboards[index_of(make_piece(c, KING))])[0];
	auto mb_b = mgc.bishop_magic[king_sq];
	auto mb_r = mgc.rook_magic[king_sq];
	Bitboard pinned_bitboard = mb_b.attacks[magic_bitboard(mb_b.mask & board.composite, mb_b.magic, mb_b.bits)];
	pinned_bitboard |= mb_r.attacks[magic_bitboard(mb_r.mask & board.composite, mb_r.magic, mb_r.bits)];
	c == WHITE ? pinned_bitboard &= board.composite_w : pinned_bitboard &= board.composite_b;

	board.pins.clear();
	for (auto& sq : serialize_bitboard(pinned_bitboard)) {
		Direction d_to_king = direction_between_squares(king_sq, sq);

		// Check whether opposition rooks, bishops, queens attack in that direction
		switch (d_to_king)
		{
		case NORTH: case SOUTH:
		{
			auto mb = mgc.rook_magic[sq];
			Bitboard to_bitboard = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];

			// Mask everything not in column out
			to_bitboard &= shift_bitboard(0x8080808080808080ULL, EAST * file_of(king_sq));

			// Remove everything in incorrect direction
			to_bitboard = shift_bitboard(to_bitboard, d_to_king * abs(rank_of(king_sq) - rank_of(sq)));

			// Include only opposition pieces
			if (to_bitboard & (board.bitboards[make_piece(~c, ROOK)] | board.bitboards[make_piece(~c, QUEEN)]))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));

			break;
		}
		case EAST: case WEST:
		{
			auto mb = mgc.rook_magic[sq];
			Bitboard pinners = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];

			// Mask everything not in row out
			pinners &= shift_bitboard(0xFFULL, NORTH * rank_of(king_sq));

			// Remove everything in incorrect direction
			pinners = shift_bitboard(pinners, d_to_king * abs(file_of(king_sq) - file_of(sq)));

			// Include only opposition pieces
			if (pinners & (board.bitboards[make_piece(~c, ROOK)] | board.bitboards[make_piece(~c, QUEEN)]))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));

			break;
		}
		case NORTH_EAST:
		{
			auto mb = mgc.bishop_magic[sq];
			Bitboard pinners = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];
			pinners = shift_bitboard(pinners, -sq + 7);
			if (pinners & (board.bitboards[make_piece(~c, BISHOP)] | board.bitboards[make_piece(~c, QUEEN)]))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));
			break;
		}
		case NORTH_WEST:
		{
			auto mb = mgc.bishop_magic[sq];
			Bitboard pinners = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];
			if (shift_bitboard((board.bitboards[make_piece(~c, BISHOP)] | board.bitboards[make_piece(~c, QUEEN)]), -sq))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));
			break;
		}
		case SOUTH_EAST:
		{
			auto mb = mgc.bishop_magic[sq];
			Bitboard pinners = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];
			if (shift_bitboard((board.bitboards[make_piece(~c, BISHOP)] | board.bitboards[make_piece(~c, QUEEN)]), sq))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));
			break;
		}
		case SOUTH_WEST:
		{
			auto mb = mgc.bishop_magic[sq];
			Bitboard pinners = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];
			if (shift_bitboard((board.bitboards[make_piece(~c, BISHOP)] | board.bitboards[make_piece(~c, QUEEN)]), sq-7))
				board.pins.push_back(std::pair<Square, Direction>(sq, d_to_king));
			break;
		}
		default:
			break;
		}
	}
	// Note: temporary so we can display pinned pieces
	board.pins_bb = 0ULL;
	for (auto &pin : board.pins) {
		board.pins_bb |= 1ULL << pin.first;
	}


	for (PieceType pt = PAWN; pt <= KING; ++pt)
	{
		Direction d = pawn_push(c);
		for (Square from = SQ_A1; from < SQ_NONE; ++from)
		{
			// Get piece in bitboard by itself
			if ((board.bitboards[index_of(make_piece(c, pt))] >> (from - SQ_A1)) & 1U)
			{
				Bitboard piece_bitboard = ((board.bitboards[index_of(make_piece(c, pt))] >> (from - SQ_A1)) & 1U) << (from - SQ_A1);
				switch (pt)
				{
				case PAWN:
				{
					Bitboard to_bitboard = 0, promotion_bitboard = 0, en_passant_bitboard = 0;
					// Single, double moves
					to_bitboard = shift_bitboard(piece_bitboard, d) & ~board.composite;
					to_bitboard |= shift_bitboard(to_bitboard, d) & PAWN_DOUBLE[c - WHITE] & ~board.composite;

					// Diagonal attacks and en passant
					Bitboard diagonal_bitboard = 0ULL;
					c == WHITE ? diagonal_bitboard |= shift_bitboard(5ULL, from + d - 1) & board.composite_b & shift_bitboard(0xFFULL, NORTH * rank_of(from) + d) : diagonal_bitboard |= shift_bitboard(5ULL, from + d - 1) & board.composite_w & shift_bitboard(0xFFULL, NORTH * rank_of(from) + d);
					c == WHITE ? en_passant_bitboard |= shift_bitboard(5ULL, from + d - 1) & board.en_passant_target_w & shift_bitboard(0xFFULL, NORTH * rank_of(from) + d) : en_passant_bitboard |= shift_bitboard(5ULL, from + d - 1) & board.en_passant_target_b & shift_bitboard(0xFFULL, NORTH * rank_of(from) + d);

					if (board.check)
					{
						to_bitboard &= board.block_check;
						diagonal_bitboard &= board.block_check;
						en_passant_bitboard &= board.block_check;
					}

					// Add attacking moves to attacked squares
					attacked |= diagonal_bitboard | en_passant_bitboard;

					to_bitboard |= diagonal_bitboard;

					// Promotion
					c == WHITE ? promotion_bitboard = to_bitboard & 0xFF00000000000000 : promotion_bitboard = to_bitboard & 0xFF;

					// Remove promotions from normal moves
					to_bitboard &= ~promotion_bitboard;

					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					auto en_passant_moves = bitboard_to_moves(en_passant_bitboard, from, EN_PASSANT);
					board.moves[from].insert(board.moves[from].end(), en_passant_moves.begin(), en_passant_moves.end());

					// Make moves for each type of promotion
					auto promotion_moves = bitboard_to_moves(promotion_bitboard, from, PROMOTION);
					for (PieceType pt = KNIGHT; pt < QUEEN; ++pt)
					{
						for (auto &m : promotion_moves)
						{
							m = Move(m & ~(3 << 12));
							m = Move(m | ((pt - KNIGHT) << 12));
						}
						board.moves[from].insert(board.moves[from].end(), promotion_moves.begin(), promotion_moves.end());
					}

					break;
				}
				case KNIGHT:
				{
					Bitboard to_bitboard = 0;
					c == WHITE ? to_bitboard = mgc.knight_attacks[from] &~board.composite_w : to_bitboard = mgc.knight_attacks[from] & ~board.composite_b;
					if (board.check)
						to_bitboard &= board.block_check;
					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					// Add moves to attacked squares
					attacked |= to_bitboard;
					break;
				}
				case KING:
				{
					Bitboard to_bitboard = 0;
					if (c == WHITE)
					{
						to_bitboard = mgc.king_attacks[from] & ~board.composite_w & ~board.attacked;
						if (board.check)
							to_bitboard &= ~(board.block_check & ~board.composite_b);
					}
					else
					{
						to_bitboard = mgc.king_attacks[from] & ~board.composite_b & ~board.attacked;
						if (board.check)
							to_bitboard &= ~(board.block_check & ~board.composite_w);
					}
					// King can't move into check
					to_bitboard &= ~board.attacked;

					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					// Add attack moves to attacked squares
					attacked |= to_bitboard;

					if (c == WHITE)
					{
						if (board.castle & WHITE_OO && !((board.composite >> SQ_F1) & 3ULL))
							board.moves[from].push_back(Move(make_move(from, Square(SQ_G1)) | CASTLING));
						if (board.castle & WHITE_OOO && !((board.composite >> SQ_B1) & 3ULL))
							board.moves[from].push_back(Move(make_move(from, Square(SQ_C1)) | CASTLING));
					}
					else
					{
						if (board.castle & BLACK_OO && !((board.composite >> SQ_F8) & 3ULL))
							board.moves[from].push_back(Move(make_move(from, Square(SQ_G8)) | CASTLING));
						if (board.castle & BLACK_OOO && !((board.composite >> SQ_B8) & 3ULL))
							board.moves[from].push_back(Move(make_move(from, Square(SQ_C8)) | CASTLING));
					}
					break;
				}
				case ROOK:
				{
					auto mb = mgc.rook_magic[from];
					Bitboard to_bitboard = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];

					c == WHITE ? to_bitboard &= ~board.composite_w : to_bitboard &= ~board.composite_b;
					if (board.check)
						to_bitboard &= board.block_check;
					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					// Add moves to attacked squares
					attacked |= to_bitboard;

					// If providing check add attacked squares to check blocking squares

					break;
				}
				case BISHOP:
				{
					auto mb = mgc.bishop_magic[from];
					Bitboard to_bitboard = mb.attacks[magic_bitboard(mb.mask & board.composite, mb.magic, mb.bits)];

					c == WHITE ? to_bitboard &= ~board.composite_w : to_bitboard &= ~board.composite_b;
					if (board.check)
						to_bitboard &= board.block_check;
					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					// Add moves to attacked squares
					attacked |= to_bitboard;
					break;
				}
				case QUEEN:
				{
					auto mb_b = mgc.bishop_magic[from];
					auto mb_r = mgc.rook_magic[from];
					Bitboard to_bitboard = mb_b.attacks[magic_bitboard(mb_b.mask & board.composite, mb_b.magic, mb_b.bits)];
					to_bitboard |= mb_r.attacks[magic_bitboard(mb_r.mask & board.composite, mb_r.magic, mb_r.bits)];

					c == WHITE ? to_bitboard &= ~board.composite_w : to_bitboard &= ~board.composite_b;
					if (board.check)
						to_bitboard &= board.block_check;
					board.moves[from] = bitboard_to_moves(to_bitboard, from);

					// Add moves to attacked squares
					attacked |= to_bitboard;
					break;
				}
				default:
					break;
				}
			}
		}
	}
	board.attacked = attacked;
}

void recalculate_board(Board &board, const MoveGenerationConstants &mgc, const Color &c)
{
	board.composite = 0ULL;
	board.composite_w = 0ULL;
	board.composite_b = 0ULL;
	for (Piece p = W_PAWN; p <= B_KING; ++p)
	{
		board.composite |= board.bitboards[index_of(p)];
		color_of(p) == WHITE ? board.composite_w |= board.bitboards[index_of(p)] : board.composite_b |= board.bitboards[index_of(p)];
	}

	generate_moves(board, mgc, c);
}

void make_move(Board &board, const Move &move, const MoveGenerationConstants &mgc, const Color &c)
{
	Bitboard from_bitboard = 1ULL << from_sq(move);
	Bitboard to_bitboard = 1ULL << to_sq(move);

	// Determine piece type
	Piece mp;
	PieceType mpt;
	for (PieceType pt = PAWN; pt <= KING; ++pt)
	{
		if (board.bitboards[index_of(make_piece(c, pt))] & from_bitboard)
		{
			mp = make_piece(c, pt);
			mpt = pt;
			break;
		}
	}

	// Handle castling state
	if (type_of(move) == CASTLING || mpt == KING)
	{
		if (c == WHITE)
			board.castle = CastlingRights(board.castle & ~WHITE_CASTLING);
		else
			board.castle = CastlingRights(board.castle & ~BLACK_CASTLING);
	}
	if (mpt == ROOK && board.castle)
	{
		if (from_sq(move) == SQ_H1)
			board.castle = CastlingRights(board.castle & ~WHITE_OO);
		else if (to_sq(move) == SQ_A1)
			board.castle = CastlingRights(board.castle & ~WHITE_OOO);
		else if (to_sq(move) == SQ_H8)
			board.castle = CastlingRights(board.castle & ~BLACK_OO);
		else if (to_sq(move) == SQ_A8)
			board.castle = CastlingRights(board.castle & ~BLACK_OOO);
	}

	// Handle en passant
	board.en_passant_target_b = 0ULL;
	board.en_passant_target_w = 0ULL;
	if (mpt == PAWN && abs(rank_of(to_sq(move)) - rank_of(from_sq(move))) > 1)
	{
		if (c == WHITE)
			board.en_passant_target_b = shift_bitboard(to_bitboard, SOUTH);
		else
			board.en_passant_target_w = shift_bitboard(to_bitboard, NORTH);
	}

	// Remove old location and add new
	board.bitboards[index_of(mp)] &= ~from_bitboard;
	board.bitboards[index_of(mp)] |= to_bitboard;
	auto overlaps = to_bitboard;

	// Move rooks after castling
	if (type_of(move) == CASTLING)
	{
		int i = index_of(make_piece(c, ROOK));
		if (to_sq(move) == SQ_G1)
		{
			board.bitboards[i] &= ~(1ULL << SQ_H1);
			board.bitboards[i] |= 1ULL << SQ_F1;
		}
		else if (to_sq(move) == SQ_C1)
		{
			board.bitboards[i] &= ~(1ULL << SQ_A1);
			board.bitboards[i] |= 1ULL << SQ_D1;
		}
		else if (to_sq(move) == SQ_G8)
		{
			board.bitboards[i] &= ~(1ULL << SQ_H8);
			board.bitboards[i] |= 1ULL << SQ_F8;
		}
		else if (to_sq(move) == SQ_C8)
		{
			board.bitboards[i] &= ~(1ULL << SQ_A8);
			board.bitboards[i] |= 1ULL << SQ_D8;
		}
	}

	// Add en passant to overlaps
	else if (type_of(move) == EN_PASSANT)
	{
		if (c == WHITE)
			overlaps |= shift_bitboard(to_bitboard, SOUTH);
		else
			overlaps |= shift_bitboard(to_bitboard, NORTH);
	}

	// Remove any overlaps
	for (Piece p = W_PAWN; p <= B_KING; ++p)
	{
		if (p != mp)
		{
			board.bitboards[index_of(p)] &= ~overlaps;
		}
	}

	if (type_of(move) == PROMOTION)
	{
		board.bitboards[index_of(make_piece(c, PAWN))] &= ~to_bitboard;
		board.bitboards[index_of(make_piece(c, promotion_type(move)))] |= to_bitboard;
	}

	generate_moves(board, mgc, c);
	board.check = board.attacked & board.bitboards[index_of(make_piece(~c, KING))];
	board.block_check = 0ULL;

	// Determine square which when blocked break check
	if (board.check)
	{
		Square att_sq = serialize_bitboard(to_bitboard)[0];
		if (mpt == ROOK || mpt == BISHOP || mpt == QUEEN)
		{
			Square king_sq = serialize_bitboard(board.bitboards[index_of(make_piece(~c, KING))])[0];
			Direction d = direction_between_squares(king_sq, att_sq);

			// Add moved pieces attack squares if in correct direction
			for (auto &m : board.moves[att_sq])
			{
				Direction dm = direction_between_squares(to_sq(m), att_sq);
				if (dm == d)
				{
					board.block_check |= 1ULL << to_sq(m);
				}
			}
		}
		board.block_check |= to_bitboard;
	}
}

int main()
{
	// Init SFML and Imgui
	sf::RenderWindow app(sf::VideoMode(800, 600), "Chess 3");
	app.setFramerateLimit(30);

	ImGui::SFML::Init(app);

	sf::Clock clock;
	sf::Event event;

	// Init chess
	UserSettings user_settings;
	user_settings.font.loadFromFile("./fonts/arial.ttf");
	GameState gamestate;
	auto mgc = init_move_generation_constants(false);

	Board board = load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", gamestate, mgc);

	// Load piece sprites
	sf::Texture piece_textures[12];
	piece_textures[0].loadFromFile("./" + user_settings.image_folder + "/pl.png");
	piece_textures[1].loadFromFile("./" + user_settings.image_folder + "/nl.png");
	piece_textures[2].loadFromFile("./" + user_settings.image_folder + "/bl.png");
	piece_textures[3].loadFromFile("./" + user_settings.image_folder + "/rl.png");
	piece_textures[4].loadFromFile("./" + user_settings.image_folder + "/ql.png");
	piece_textures[5].loadFromFile("./" + user_settings.image_folder + "/kl.png");
	piece_textures[6].loadFromFile("./" + user_settings.image_folder + "/pd.png");
	piece_textures[7].loadFromFile("./" + user_settings.image_folder + "/nd.png");
	piece_textures[8].loadFromFile("./" + user_settings.image_folder + "/bd.png");
	piece_textures[9].loadFromFile("./" + user_settings.image_folder + "/rd.png");
	piece_textures[10].loadFromFile("./" + user_settings.image_folder + "/qd.png");
	piece_textures[11].loadFromFile("./" + user_settings.image_folder + "/kd.png");

	sf::Sprite piece_sprites[12] = {
		sf::Sprite(piece_textures[0]),
		sf::Sprite(piece_textures[1]),
		sf::Sprite(piece_textures[2]),
		sf::Sprite(piece_textures[3]),
		sf::Sprite(piece_textures[4]),
		sf::Sprite(piece_textures[5]),
		sf::Sprite(piece_textures[6]),
		sf::Sprite(piece_textures[7]),
		sf::Sprite(piece_textures[8]),
		sf::Sprite(piece_textures[9]),
		sf::Sprite(piece_textures[10]),
		sf::Sprite(piece_textures[11]),
	};

	bool running = true;
	while (running)
	{
		while (app.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);
			if (event.type == sf::Event::Closed)
				running = false;
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
					gamestate.selected_square = SQ_NONE;
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					auto sq = find_square_from_px(app, board, gamestate, user_settings, sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

					bool move_made = false;
					if (gamestate.selected_square != SQ_NONE)
					{
						for (auto &m : board.moves[gamestate.selected_square])
						{
							if (to_sq(m) == sq)
							{
								make_move(board, m, mgc, gamestate.color);
								gamestate.color == WHITE ? gamestate.color = BLACK : gamestate.color = WHITE;
								recalculate_board(board, mgc, gamestate.color);
								move_made = true;
								break;
							}
						}
					}
					gamestate.selected_square = SQ_NONE;

					// Check legal square
					if (!move_made)
					{
						for (PieceType pt = PAWN; pt <= KING; ++pt)
						{
							if ((board.bitboards[index_of(make_piece(gamestate.color, pt))] >> sq) & 1U)
							{
								gamestate.selected_square = sq;
								break;
							}
						}
					}
				}
			}
			if (event.type == sf::Event::Resized)
			{
				// update the view to the new size of the window
				sf::FloatRect visibleArea(0.f, 0.f, event.size.width, event.size.height);
				app.setView(sf::View(visibleArea));
			}
		}
		auto dt = clock.restart();
		if (gamestate.color == BLACK)
		{
			gamestate.time_b += dt.asSeconds();
		}
		else
		{
			gamestate.time_w += dt.asSeconds();
		}

		// Handle imgui
		ImGui::SFML::Update(app, dt);

		// Render
		app.clear(user_settings.background_col);
		render_board(app, board, gamestate, user_settings, piece_sprites);
		render_overlay_bitboard(app, board.block_check, sf::Color(0, 240, 20, 210), gamestate, user_settings);
		render_overlay_bitboard(app, board.pins_bb, sf::Color(255, 165, 0, 210), gamestate, user_settings);
		ImGui::SFML::Render(app);
		app.display();
	}
	app.close();
	return 0;
}
