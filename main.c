#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

#define DELAY 150000
#define SIZE_MULT 0.7f
#define ROW_PER_COL 0.3f
#define COLOR_LEN 7

typedef struct {
    int x;
    int y;
} Pos;

typedef struct {
    Pos pos;
    char symbol;
} Cookie;

typedef struct {
    bool gameOver;
    Cookie *cookie;
} State;

typedef struct SnakePiece {
    struct SnakePiece* next; 
    Pos pos;
} SnakePiece;

typedef struct {
    SnakePiece *tail;
    SnakePiece *head;
    int length;
} Player;


typedef enum {
    LEFT,
    RIGHT,
    UP,
    DOWN
} Direction;

typedef struct {
    int rows;
    int cols;
} BoardSize;

Direction updateDirection(Direction prev, WINDOW *board);
void initPlayer(Player* player, BoardSize b);
void movePlayer(Player *p, Direction d, State* state, BoardSize b);
void drawBoard(BoardSize b, Player p, State state, WINDOW *board);
void drawScore(Player p, WINDOW *score);
void addSnakePiece(Player *player);
void moveCookie(Cookie* cookie, BoardSize b, SnakePiece* tail);
void freeSnake(Player* p);

int main(void){
    srand(time(NULL));
    int rows, cols, boardRows, boardCols;
    bool isPlaying = true;
    initscr();
    getmaxyx(stdscr, rows, cols);

    if(rows < cols) {
        boardRows = SIZE_MULT * rows;
        boardCols = boardRows / ROW_PER_COL;
    } else {
        boardCols = SIZE_MULT * cols;
        boardRows = ROW_PER_COL * boardCols;
    }
    WINDOW *score = newwin(1, boardCols, (rows - boardRows) / 2 - 1, (cols - boardCols) / 2);
    WINDOW *board = newwin(boardRows, boardCols, (rows - boardRows) / 2, (cols - boardCols) / 2);

    if(!has_colors()) {
        printf("you don't support colors weirdo\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    

    // wattron(board, COLOR_PAIR(0));
    // wattrset(board, COLOR_PAIR(1));


    BoardSize boardSize = { 
        .rows = boardRows,
        .cols = boardCols
    };  

    curs_set(false);
    noecho();
    wtimeout(board, 15);
    cbreak();

    Direction cur = LEFT;
    Direction prev;
    Player p = {0};

    initPlayer(&p, boardSize);
    
    Cookie c = {
        .pos = {0},
        .symbol = 'o'
    };

    moveCookie(&c, boardSize, p.tail);

    State gameState = {
        .gameOver = false,
        .cookie = &c
    };
    while(isPlaying) {
        
        prev = cur;
        cur = updateDirection(prev, board);
        movePlayer(&p, cur, &gameState, boardSize);
        if(gameState.gameOver) isPlaying = false;

        werase(board);
        werase(score);

        drawBoard(boardSize, p, gameState, board);
        drawScore(p, score);

        wrefresh(board);
        wrefresh(score);
        usleep(DELAY);

    }
    freeSnake(&p);
    delwin(board);
    delwin(score);
    endwin();
    return 0;
}

void drawBoard(BoardSize b, Player p, State state, WINDOW *board) {
    wborder(board, 0, 0, 0, 0, 0, 0, 0, 0);
    SnakePiece* tail = p.tail;
    // wattroff(board, COLOR_PAIR(0));
    for(int i = 0; i < p.length; i++) {
        wattron(board, COLOR_PAIR((i % COLOR_LEN) + 1));
        mvwaddch(board, tail->pos.y, tail->pos.x, ACS_BLOCK);
        tail = tail->next;
        wattroff(board, COLOR_PAIR((i % COLOR_LEN) + 1));
    }

    // LeCookie
    int col = (time(NULL) % COLOR_LEN) + 1;
    wattron(board, COLOR_PAIR(col));
    mvwaddch(board, state.cookie->pos.y, state.cookie->pos.x, state.cookie->symbol);
    wattroff(board, COLOR_PAIR(col));
}

void drawScore(Player p, WINDOW *score) {
    wattron(score, COLOR_PAIR(3));
    wprintw(score, "Length: %d", p.length);
    wattroff(score, COLOR_PAIR(3));
}

void freeSnake(Player* p) {
    SnakePiece* cur = p->tail;
    for(int i = 0; i < p->length; i++) {
        cur = cur->next;
        free(p->tail);
        p->tail = cur;

    }

}

void moveCookie(Cookie* cookie, BoardSize b, SnakePiece* tail) {
    Pos newPos;

    bool inSnake; // assume false
    SnakePiece* cur;
    do {
        inSnake = false;
        newPos.x = rand() % (b.cols - 3) + 1;
        newPos.y = rand() % (b.rows - 3) + 1;
        for(cur = tail; cur != NULL; cur = cur->next) {
            if(cur->pos.x == newPos.x && cur->pos.y == newPos.y) {
                inSnake = true;
                break;
            }
        }
    } while(inSnake);
    cookie->pos = newPos;
}

void movePlayer(Player *p, Direction d, State* state, BoardSize b) {
    Pos newPos = {
        .x = p->head->pos.x,
        .y = p->head->pos.y
    };

    switch(d) {
        case LEFT:
            newPos.x -= 1;
            break;
        case RIGHT:
            newPos.x += 1;
            break;
        case UP:
            newPos.y -= 1;
            break;
        case DOWN:
            newPos.y += 1;
            break;
    }

    if(newPos.x == b.cols - 1 || newPos.x == 0
        || newPos.y == b.rows - 1 || newPos.y == 0) {
            state->gameOver = true;
            return;
    }

    // valid move

    SnakePiece *snakePiece = p->tail;
    for(int i = 0; i < p->length - 1; i++) {
        snakePiece->pos.x = snakePiece->next->pos.x;
        snakePiece->pos.y = snakePiece->next->pos.y;
        if(newPos.x == snakePiece->pos.x && newPos.y == snakePiece->pos.y) {
            state->gameOver = true;
            return;
        }
        snakePiece = snakePiece->next;
    }

    // head
    snakePiece->pos = newPos;
    if(newPos.x == state->cookie->pos.x && newPos.y == state->cookie->pos.y) {
        // LeEat
        moveCookie(state->cookie, b, p->tail);
        addSnakePiece(p);
    }
}

void addSnakePiece(Player* player) {
    SnakePiece* newPiece = (SnakePiece *) malloc(sizeof(SnakePiece));
    newPiece->pos = player->tail->pos;
    newPiece->next = player->tail;
    player->tail = newPiece;
    player->length++;
}

Direction updateDirection(Direction prev, WINDOW *board) {
    int ch = wgetch(board);
    flushinp();
    if(ch == 'w' || ch == KEY_UP) return UP;
    if(ch == 'a' || ch == KEY_LEFT) return LEFT;
    if(ch == 's' || ch == KEY_DOWN) return DOWN;
    if(ch == 'd' || ch == KEY_RIGHT) return RIGHT;
    return prev;
}

void initPlayer(Player *player, BoardSize b) {
    SnakePiece *tail = (SnakePiece *) malloc(sizeof(SnakePiece));
    Pos p = {
        .x = b.cols / 2,
        .y = b.rows / 2
    };
    tail->pos = p;
    tail->next = NULL;
    player->tail = tail;
    player->head = tail;
    player->length = 1;
}