#include <Arduboy.h>
#include "lib/ArduboyExtra/simple_buttons.h"
#include "chess.hh"

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

    operator bool() {
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

    operator bool() {
        return packed != 0;
    }

    color_t color() {
        return (color_t)(packed >> 7);
    }

    rank_t rank() {
        return (rank_t)(packed & 7);
    }
};

class Chess {
    Piece board[8][8];
    Coords cursor;
    Coords piece;
    bool frame_count;
    color_t current_player;

public:
    Chess()
        : board({
                { Piece(BLACK, ROOK), Piece(BLACK, KNIGHT), Piece(BLACK, BISHOP), Piece(BLACK, QUEEN), Piece(BLACK, KING), Piece(BLACK, BISHOP), Piece(BLACK, KNIGHT), Piece(BLACK, ROOK) },
                { Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN), Piece(BLACK, PAWN) },
                { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
                { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
                { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
                { Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece(), Piece() },
                { Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN), Piece(WHITE, PAWN) },
                { Piece(WHITE, ROOK), Piece(WHITE, KNIGHT), Piece(WHITE, BISHOP), Piece(WHITE, QUEEN), Piece(WHITE, KING), Piece(WHITE, BISHOP), Piece(WHITE, KNIGHT), Piece(WHITE, ROOK) },
            })
        // start cursor at white's first pawn:
        , cursor(Coords(0, 6))
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
            if (piece) {
                if (can_move()) {
                    board[cursor.y][cursor.x] = board[piece.y][piece.x];
                    board[piece.y][piece.x] = Piece();
                    piece = Coords();
                }
            } else {
                if (board[cursor.y][cursor.x].color() == current_player) {
                    piece = cursor;
                }
            }
        }

        if (piece) {
            if (buttons.justPressed(B_BUTTON)) {
                cursor = piece;
                piece = Coords();
            }
        }
    }

    void render() {
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                draw_square(x, y, board[y][x]);
            }
        }

        // draw selection
        if (cursor) {
            if (piece) {
                int x = piece.x * 8 + 32;
                int y = piece.y * 8;
                arduboy.drawRect(x - 1, y - 1, 10, 10, WHITE);
            }

            if (piece != cursor) {
                int x = cursor.x * 8 + 32;
                int y = cursor.y * 8;
                arduboy.drawRect(x - 1, y - 1, 10, 10, frame_count);
            }
        }
    }

    void draw_square(int square_x, int square_y, Piece piece) {
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

        // draw square
        int square_sprite_idx = ~(square_x ^ square_y) & 1;
        arduboy.drawBitmap(x, y, SQUARE_SPRITES[square_sprite_idx], 8, 8, 1);

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

    bool can_move() {
        Piece moving = board[piece.y][piece.x];
        Piece target = board[cursor.y][cursor.x];

        if (!moving) {
            return false;
        }

        if (target && moving.color() == target.color()) {
            return false;
        }

        if (piece == cursor) {
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
                    if (piece.y == 6 && cursor.y == 4 && piece.x == cursor.x) {
                        // only if spot immediately in front is not occupied:
                        if (board[5][cursor.x]) {
                            return false;
                        }

                        return true;
                    }
                } else {
                    if (piece.y == 1 && cursor.y == 3 && piece.x == cursor.x) {
                        // ditto:
                        if (board[2][cursor.x]) {
                            return false;
                        }

                        return true;
                    }
                }

                // pawn may only ever move forward otherwise:
                if (moving.color() == WHITE) {
                    if (piece.y - 1 != cursor.y) {
                        return false;
                    }
                } else {
                    if (piece.y + 1 != cursor.y) {
                        return false;
                    }
                }

                // pawn can move directly forward only if not attacking:
                if (piece.x == cursor.x && !target) {
                    return true;
                }

                // pawn can attack diagonally
                if ((piece.x - 1 == cursor.x || piece.x + 1 == cursor.x) && target) {
                    return true;
                }

                // no other moves are permitted:
                return false;

                // TODO implement promotion
            }
            case ROOK: {
                // rook can move vertically if nothing's in the way
                if (piece.x == cursor.x) {
                    if (piece.y > cursor.y) {
                        for (int y = cursor.y + 1; y < piece.y; y++) {
                            if (board[y][cursor.x]) {
                                return false;
                            }
                        }
                        return true;
                    } else {
                        for (int y = piece.y + 1; y < cursor.y; y++) {
                            if (board[y][cursor.x]) {
                                return false;
                            }
                        }
                        return true;
                    }
                }

                // or horizontally
                if (piece.y == cursor.y) {
                    if (piece.x > cursor.x) {
                        for (int x = cursor.x + 1; x < piece.x; x++) {
                            if (board[cursor.y][x]) {
                                return false;
                            }
                        }
                        return true;
                    } else {
                        for (int x = piece.x + 1; x < cursor.x; x++) {
                            if (board[cursor.y][x]) {
                                return false;
                            }
                        }
                        return true;
                    }
                }

                return false;
            }
            default: {
                for (;;);
            }
        }

        return true;
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