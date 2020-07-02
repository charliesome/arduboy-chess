#include <Arduboy.h>
#include "lib/ArduboyExtra/simple_buttons.h"
#include "chess.hh"
#include "util.hh"

Arduboy arduboy;
SimpleButtons buttons (arduboy);

typedef enum {
    NONE    = 0,
    PAWN    = 1,
    ROOK    = 2,
    KNIGHT  = 3,
    BISHOP  = 4,
    QUEEN   = 5,
    KING    = 6,
} rank_t;

static const uint8_t PROGMEM RANK_SPRITES[7][8] = {
    /* none */      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* pawn */      { 0x00, 0x40, 0x60, 0x7e, 0x7e, 0x60, 0x40, 0x00 },
    /* rook */      { 0x00, 0x66, 0x7c, 0x7e, 0x7e, 0x7c, 0x66, 0x00 },
    /* knight */    { 0x00, 0x1a, 0x1c, 0x4e, 0x7c, 0x7c, 0x78, 0x00 },
    /* bishop */    { 0x00, 0x40, 0x4c, 0x7e, 0x7e, 0x4c, 0x40, 0x00 },
    /* queen */     { 0x00, 0x5e, 0x70, 0x7e, 0x7e, 0x70, 0x5e, 0x00 },
    /* king */      { 0x00, 0x18, 0x18, 0x7e, 0x7e, 0x18, 0x18, 0x00 },
};

// arduboy headers define WHITE and BLACK, so just use their definition to prevent cooked bugs
typedef uint8_t color_t;

struct Coords {
    int x;
    int y;

    Coords() : x(-1), y(-1) {}
    Coords(int x, int y) : x(x), y(y) {}

    operator bool() const {
        return x >= 0 && y >= 0;
    }

    bool operator ==(const Coords& other) {
        return x == other.x && y == other.y;
    }

    bool operator !=(const Coords& other) {
        return !(*this == other);
    }

    void receive_input() {
        if (buttons.justPressed(LEFT_BUTTON)) {
            if (x > 0) {
                x--;
            }
        }

        if (buttons.justPressed(RIGHT_BUTTON)) {
            if (x < 7) {
                x++;
            }
        }

        if (buttons.justPressed(UP_BUTTON)) {
            if (y > 0) {
                y--;
            }
        }

        if (buttons.justPressed(DOWN_BUTTON)) {
            if (y < 7) {
                y++;
            }
        }
    }
};

class Piece {
    uint8_t packed;
public:
    Piece() : packed(0) {}

    Piece(color_t color, rank_t rank)
        : packed(((color & 1) << 7) | (rank & 7))
    {}

    operator bool() const {
        return packed != 0;
    }

    color_t color() const {
        return (color_t)(packed >> 7);
    }

    rank_t rank() const {
        return (rank_t)(packed & 7);
    }
};

class Board {
    Piece squares[8][8];

public:
    Board()
        : squares({
            { Piece(BLACK, ROOK), Piece(BLACK, KNIGHT), Piece(BLACK, BISHOP), Piece(BLACK, QUEEN), Piece(BLACK, KING), Piece(BLACK, BISHOP), Piece(BLACK, KNIGHT), Piece(BLACK, ROOK) },
            { Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN) },
            { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
            { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
            { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
            { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
            { Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN) },
            { Piece(WHITE, ROOK), Piece(WHITE, KNIGHT), Piece(WHITE, BISHOP), Piece(WHITE, QUEEN), Piece(WHITE, KING), Piece(WHITE, BISHOP), Piece(WHITE, KNIGHT), Piece(WHITE, ROOK) },
        })
    {}

    const Piece& square(Coords coords) const {
        return squares[coords.y][coords.x];
    }

    Board move(Coords from, Coords to) const {
        Board new_ = *this;
        new_.squares[to.y][to.x] = new_.squares[from.y][from.x];
        new_.squares[from.y][from.x] = Piece();
        return new_;
    }

    bool is_valid_turn(Coords moving_coords, Coords target_coords, color_t player) const {
        if (!is_valid_move(moving_coords, target_coords)) {
            return false;
        }

        if (move(moving_coords, target_coords).is_check(player)) {
            return false;
        }

        return true;
    }

    bool is_check(color_t player_in_check) const {
        Coords king = find_king(player_in_check);

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                Coords coords = Coords(x, y);
                Piece piece = square(coords);

                if (piece.color() != player_in_check) {
                    if (is_valid_move(coords, king)) {
                        // check detected
                        return true;
                    }
                }
            }
        }

        return false;
    }

private:
    bool is_valid_move(Coords moving_coords, Coords target_coords) const {
        if (moving_coords == target_coords) {
            return false;
        }

        Piece moving = square(moving_coords);
        Piece target = square(target_coords);

        if (!moving) {
            return false;
        }

        if (target && moving.color() == target.color()) {
            return false;
        }

        switch (moving.rank()) {
            case NONE: {
                // shouldn't ever happen
                return false;
            }
            case PAWN: {
                // pawn can move two steps on first move:
                if (moving.color() == WHITE) {
                    if (moving_coords.y == 6 && target_coords.y == 4 && moving_coords.x == target_coords.x) {
                        // only if spot immediately in front is not occupied:
                        if (square(Coords(target_coords.x, 5))) {
                            return false;
                        }

                        return true;
                    }
                } else {
                    if (moving_coords.y == 1 && target_coords.y == 3 && moving_coords.x == target_coords.x) {
                        // ditto:
                        if (square(Coords(target_coords.x, 2))) {
                            return false;
                        }

                        return true;
                    }
                }

                // pawn may only ever move forward otherwise:
                if (moving.color() == WHITE) {
                    if (moving_coords.y - 1 != target_coords.y) {
                        return false;
                    }
                } else {
                    if (moving_coords.y + 1 != target_coords.y) {
                        return false;
                    }
                }

                // pawn can move directly forward only if not attacking:
                if (moving_coords.x == target_coords.x && !target) {
                    return true;
                }

                // pawn can attack diagonally
                if ((moving_coords.x - 1 == target_coords.x || moving_coords.x + 1 == target_coords.x) && target) {
                    return true;
                }

                // no other moves are permitted:
                return false;

                // TODO implement promotion
            }
            case ROOK: {
                return is_valid_cardinal_move(moving_coords, target_coords);
            }
            case KNIGHT: {
                if (moving_coords.y == target_coords.y - 2 && moving_coords.x == target_coords.x - 1) { return true; }
                if (moving_coords.y == target_coords.y + 2 && moving_coords.x == target_coords.x - 1) { return true; }
                if (moving_coords.y == target_coords.y - 2 && moving_coords.x == target_coords.x + 1) { return true; }
                if (moving_coords.y == target_coords.y + 2 && moving_coords.x == target_coords.x + 1) { return true; }

                if (moving_coords.y == target_coords.y - 1 && moving_coords.x == target_coords.x - 2) { return true; }
                if (moving_coords.y == target_coords.y + 1 && moving_coords.x == target_coords.x - 2) { return true; }
                if (moving_coords.y == target_coords.y - 1 && moving_coords.x == target_coords.x + 2) { return true; }
                if (moving_coords.y == target_coords.y + 1 && moving_coords.x == target_coords.x + 2) { return true; }

                return false;
            }
            case BISHOP: {
                return is_valid_diagonal_move(moving_coords, target_coords);
            }
            case QUEEN: {
                return is_valid_cardinal_move(moving_coords, target_coords) || is_valid_diagonal_move(moving_coords, target_coords);
            }
            case KING: {
                int dist_x = abs(target_coords.x - moving_coords.x);
                int dist_y = abs(target_coords.y - moving_coords.y);

                return dist_x <= 1 && dist_y <= 1;

                // TODO implement castling
            }
        }

        return true;
    }

    Coords find_king(color_t player) const {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                Coords coords = Coords(x, y);
                Piece piece = square(coords);

                if (piece.color() == player && piece.rank() == KING) {
                    return coords;
                }
            }
        }

        // TOOD shouldn't happen, we need a panic function
        for (;;) ;
    }

    bool is_valid_cardinal_move(Coords moving_coords, Coords target_coords) const {
        int dist_x = abs(target_coords.x - moving_coords.x);
        int dist_y = abs(target_coords.y - moving_coords.y);

        // make sure movement only occurs along one axis
        if (dist_x != 0 && dist_y != 0) {
            return false;
        }

        int scale_x = signum(target_coords.x - moving_coords.x);
        int scale_y = signum(target_coords.y - moving_coords.y);

        int dist = max(dist_x, dist_y);

        for (int i = 1; i < dist; i++) {
            Coords intermediate = Coords(moving_coords.x + i * scale_x, moving_coords.y + i * scale_y);
            if (square(intermediate)) {
                return false;
            }
        }

        return true;
    }

    bool is_valid_diagonal_move(Coords moving_coords, Coords target_coords) const {
        int dist_x = abs(target_coords.x - moving_coords.x);
        int dist_y = abs(target_coords.y - moving_coords.y);

        // make sure movement is diagonal
        if (dist_x != dist_y) {
            return false;
        }

        int scale_x = signum(target_coords.x - moving_coords.x);
        int scale_y = signum(target_coords.y - moving_coords.y);

        for (int i = 1; i < dist_x; i++) {
            Coords intermediate = Coords(moving_coords.x + i * scale_x, moving_coords.y + i * scale_y);
            if (square(intermediate)) {
                return false;
            }
        }

        return true;
    }
};

class Chess {
    Board board;
    Coords cursor;
    Coords selected;
    bool frame_count;
    color_t current_player;

public:
    Chess()
        : cursor(Coords(0, 6))
        , frame_count(false)
        , current_player(WHITE)
    {
    }

    void loop() {
        frame_count = !frame_count;
        receive_input();
        render();
    }

private:
    void receive_input() {
        cursor.receive_input();

        if (buttons.justPressed(A_BUTTON)) {
            if (selected) {
                if (board.is_valid_turn(selected, cursor, current_player)) {
                    Board new_board = board.move(selected, cursor);
                    board = new_board;
                    selected = Coords();
                    next_player();
                }
            } else {
                if (board.square(cursor).color() == current_player) {
                    selected = cursor;
                }
            }
        }

        if (selected) {
            if (buttons.justPressed(B_BUTTON)) {
                cursor = selected;
                selected = Coords();
            }
        }
    }

    void next_player() {
        int y_begin, y_scale;

        if (current_player == WHITE) {
            current_player = BLACK;
            y_begin = 0;
            y_scale = 1;
        } else {
            current_player = WHITE;
            y_begin = 7;
            y_scale = -1;
        }

        for (int i = 0; i < 8; i++) {
            int y = y_begin + i * y_scale;
            for (int x = 0; x < 8; x++) {
                const Piece& piece = board.square(Coords(x, y));
                if (piece && piece.color() == current_player) {
                    cursor = Coords(x, y);
                    return;
                }
            }
        }
    }

    void render() {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                draw_square(x, y);
            }
        }

        // draw selection
        if (cursor) {
            if (selected) {
                int x = selected.x * 8 + 32;
                int y = selected.y * 8;
                arduboy.drawRect(x - 1, y - 1, 10, 10, WHITE);
            }

            if (selected != cursor) {
                int x = cursor.x * 8 + 32;
                int y = cursor.y * 8;
                arduboy.drawRect(x - 1, y - 1, 10, 10, frame_count);
            }
        }

        if (current_player == WHITE) {
            arduboy.setCursor(0, 64 - 8);
            arduboy.print("White");

            if (board.is_check(WHITE)) {
                arduboy.setCursor(0, 64 - 16);
                arduboy.print("CHECK");
            }
        } else {
            arduboy.setCursor(0, 0);
            arduboy.print("Black");

            if (board.is_check(BLACK)) {
                arduboy.setCursor(0, 8);
                arduboy.print("CHECK");
            }
        }
    }

    void draw_square(int square_x, int square_y) {
        Piece piece = board.square(Coords(square_x, square_y));

        int x = square_x * 8 + 32;
        int y = square_y * 8;

        static const uint8_t PROGMEM SQUARE_SPRITES[3][8] = {
            // black:
            { 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa },
            // white:
            { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa },
            // selected:
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
        };

        // draw square only if there is no selection, or this square is a valid move target
        if (!selected || board.is_valid_turn(selected, Coords(square_x, square_y), current_player)) {
            int square_sprite_idx = ~(square_x ^ square_y) & 1;
            arduboy.drawBitmap(x, y, SQUARE_SPRITES[square_sprite_idx], 8, 8, 1);
        }

        // draw outline in reverse colour
        const uint8_t* sprite = RANK_SPRITES[piece.rank()];
        for (int y_off = -1; y_off <= 1; y_off++) {
            for (int x_off = -1; x_off <= 1; x_off++) {
                arduboy.drawBitmap(x + x_off, y + y_off, sprite, 8, 8, !piece.color());
            }
        }

        // draw sprite in piece colour
        arduboy.drawBitmap(x, y, RANK_SPRITES[piece.rank()], 8, 8, piece.color());
    }
};

Chess chess;

void chess_init() {
    arduboy.beginNoLogo();
}

void chess_loop() {
    if (arduboy.nextFrame()) {
        buttons.poll();
        arduboy.clear();
        chess.loop();
        arduboy.display();
    }
}
