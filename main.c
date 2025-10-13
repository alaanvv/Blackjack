#include <string.h>
#include <termios.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

#define USE_ASCII 1
#define SHOW_LABELS 0

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)
#define RAND(min, max) (rand() % (max - min) + min)
#define PRINT(...) { printf(__VA_ARGS__); printf("\n"); }
#define PRINTLN(...) { printf(__VA_ARGS__); printf("\n"); }
#define ASSERT(x, ...) if (!(x)) { PRINT(__VA_ARGS__); exit(1); }

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"
#define RESET   "\033[0m"

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;
typedef char     c8;

c8 c;

// ---

typedef enum { ACE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING } Rank;

typedef enum { SPADES, CLUBS, HEARTS, DIAMONDS } Suit;

typedef struct {
  Rank rank;
  Suit suit;
} Card;

typedef struct {
  Card cards[52];
  u8 size;
} Deck;

const c8 *ranks[] = { "Ace", "2", "3",  "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King" };
const c8 *suits[] = { "spades", "hearts", "diamonds", "clubs" };
const c8 *ranks_ascii[] = { "A ", "2 ", "3 ",  "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "10", "J ", "Q ", "K " };
const c8 *_ranks_ascii[] = { " A", " 2", " 3",  " 4", " 5", " 6", " 7", " 8", " 9", "10", " J", " Q", " K" };
const c8 *suits_ascii[] = { "♠", "♣", "❤", "♦" };

void scan(const char *fmt, void *arg);
void write_c(u32 value);
int is_soft(Deck deck);
void flush_read(c8* c);
void msleep(u16 time);
u32 read_c();


// ---

void print_card(Card card, u8 face_mask) {
  if (face_mask) PRINTLN("- %s of %s", ranks[card.rank], suits[card.suit])
  else PRINTLN("- ???")
}

void print_deck(Deck deck, i64 face_mask) {
  for (int i = 0; i < deck.size; i++)
    print_card(deck.cards[i], (i64) pow(2, i) & face_mask);
}

void print_deck_ascii(Deck deck, i64 face_mask) {
  char ascii[7][512] = { 0 };

  char buffer[32];
  for (int i = 0; i < deck.size; i++) {
    if (!((i64) pow(2, i) & face_mask)) {
      strcat(ascii[0], BOLD "┌───────┐");
      strcat(ascii[1], BOLD "│" BLUE "+.~*+.~" RESET BOLD "│");
      strcat(ascii[2], BOLD "│" BLUE "*+.~*+." RESET BOLD "│");
      strcat(ascii[3], BOLD "│" BLUE "~*+.~*+" RESET BOLD "│");
      strcat(ascii[4], BOLD "│" BLUE ".~*+.~*" RESET BOLD "│");
      strcat(ascii[5], BOLD "│" BLUE "+.~*+.~" RESET BOLD "│");
      strcat(ascii[6], BOLD "└───────┘");
      continue;
    }

    strcat(ascii[0], BOLD "┌───────┐");
    sprintf(buffer,  BOLD "│ %s%s%s    │", deck.cards[i].suit >= HEARTS ? RED : "", ranks_ascii[deck.cards[i].rank], RESET);
    strcat(ascii[1], buffer);
    strcat(ascii[2], BOLD "│       │");
    sprintf(buffer,  BOLD "│   %s%s%s   │", deck.cards[i].suit >= HEARTS ? RED : "", suits_ascii[deck.cards[i].suit], RESET);
    strcat(ascii[3], buffer);
    strcat(ascii[4], BOLD "│       │");
    sprintf(buffer,  BOLD "│    %s%s%s │", deck.cards[i].suit >= HEARTS ? RED : "", _ranks_ascii[deck.cards[i].rank], RESET);
    strcat(ascii[5], buffer);
    strcat(ascii[6], BOLD "└───────┘");
  }

  for (int i = 0; i < 7; i++)
    printf("%s\n", ascii[i]);
  printf(RESET);
}

void fill_deck(Deck* deck) {
  for (int i = 0; i < 52; i++)
    deck->cards[i] = (Card) { i % 13, i % 4 };
  deck->size = 52;
}

void empty_deck(Deck* deck) {
  deck->size = 0;
}

void shuffle_deck(Deck* deck) {
  for (int i = deck->size - 1; i > 0; i--) {
    int j = RAND(0, i + 1);
    Card tmp = deck->cards[i];
    deck->cards[i] = deck->cards[j];
    deck->cards[j] = tmp;
  }
}

int move_card(Deck* from, Deck* to) {
  if (from->size == 0 || to->size == 52) return 0;
  to->cards[to->size++] = from->cards[--from->size];
  return 1;
}

// ---

Deck shoe, player, dealer;
u32 bet, total_chips;

int has_ace(Deck deck) {
  for (int i = 0; i < deck.size; i++)
    if (deck.cards[i].rank == ACE) return 1;
  return 0;
}

int hand_points(Deck deck, int should_soft) {
  int acc = 0;

  for (int i = 0; i < deck.size; i++)
    acc += MIN(deck.cards[i].rank + 1, 10);

  if (should_soft && is_soft(deck))
    acc += 10;

  return acc;
}

int is_soft(Deck deck) {
  if (has_ace(deck) && hand_points(deck, 0) <= 11)
    return 1;
  return 0;
}

int is_blackjack(Deck deck) {
  return hand_points(deck, 1) == 21 && deck.size == 2;
}

int is_21(Deck deck) {
  return hand_points(deck, 0) == 21;
}

int is_bust(Deck deck) {
  return hand_points(deck, 1) > 21;
}

void display(int show_hole) {
  printf("\033[H\033[2J");

  if (SHOW_LABELS) {
    if (dealer.size == 2 && !show_hole) PRINTLN("dealer's hand (%d + ?):", MIN(dealer.cards[0].rank + 1, 10))
    else PRINTLN("dealer's hand (%s%d):", is_soft(dealer) ? "soft " : "", hand_points(dealer, 1))
  }
  if (USE_ASCII) print_deck_ascii(dealer, show_hole ? ~0 : 1);
  else print_deck(dealer, show_hole ? ~0 : 1);
  PRINTLN("");

  if (SHOW_LABELS) PRINTLN("your hand (%s%d):", is_soft(player) ? "soft " : "", hand_points(player, 1))
  if (USE_ASCII) print_deck_ascii(player, ~0);
  else print_deck(player, ~0);
  PRINTLN("");
}

void players_turn() {
  while (!is_bust(player) && !is_blackjack(player)) {
    display(0);
    PRINTLN((player.size == 2 && bet <= total_chips / 2) ? "hit, stand or double down?" : "hit or stand?")
    flush_read(&c);

    if (c == 'd' && player.size == 2 && bet <= total_chips / 2) { 
      bet *= 2;
      PRINTLN("betting %d", bet);
      move_card(&shoe, &player); 
      display(0); 
      break;
    }
    if (c == 'h') { move_card(&shoe, &player); display(0); }
    if (c == 's') break;
    if (c == 27) exit(0);
  }
}

void dealers_turn() {
  display(1);
  msleep(1000);
  while (hand_points(dealer, 1) < 17) {
    move_card(&shoe, &dealer);
    display(1);
    msleep(1000);
  }
}

void game() {
  printf("\033[H\033[2J");

  total_chips = MAX(1, read_c());
  if (total_chips > 1) {
    PRINTLN(BOLD "you have %d casino chips" RESET, total_chips);
    printf("place your bets -> ");
    scan("%d", &bet);
  }
  bet = MAX(1, MIN(total_chips, bet));
  PRINTLN("betting %d", bet)
  msleep(500);

  empty_deck(&player);
  empty_deck(&dealer);
  fill_deck(&shoe);
  shuffle_deck(&shoe);

  display(0);
  msleep(500);
  move_card(&shoe, &dealer);
  display(0);
  msleep(500);
  move_card(&shoe, &dealer);
  display(0);
  msleep(500);
  move_card(&shoe, &player);
  display(0);
  msleep(500);
  move_card(&shoe, &player);
  display(0);
  msleep(500);

  if (!is_blackjack(dealer)) players_turn();
  else display(1);
  if (!is_blackjack(player) && !is_bust(player)) dealers_turn();
  else display(1);

  i8 res;
  if (is_blackjack(player) && is_blackjack(dealer)) { PRINTLN(BOLD MAGENTA "blackjack push" RESET); res = 0; }
  else if (is_blackjack(player)) { PRINTLN(BOLD GREEN "blackjack!" RESET); res = 1; }
  else if (is_blackjack(dealer)) { PRINTLN(BOLD RED "dealer blackjacked..." RESET); res = -1; }
  else if (is_bust(dealer)) { PRINTLN(BOLD GREEN "dealer busted!" RESET); res = 1; }
  else if (is_bust(player)) { PRINTLN(BOLD RED "you busted..." RESET); res = -1; }
  else if (is_21(player) && is_soft(player)) { PRINTLN(BOLD GREEN "soft win!" RESET); res = 1; }
  else if (is_21(dealer) && is_soft(dealer)) { PRINTLN(BOLD RED "dealer soft wins..." RESET); res = -1; }
  else if (is_21(player)) { PRINTLN(BOLD GREEN "you got 21!" RESET); res = 1; }
  else if (is_21(dealer)) { PRINTLN(BOLD RED "dealer got 21..." RESET); res = -1; }
  else if (hand_points(player, 1) > hand_points(dealer, 1)) { PRINTLN(BOLD GREEN "you win!" RESET); res = 1; }
  else if (hand_points(dealer, 1) > hand_points(player, 1)) { PRINTLN(BOLD RED "dealer wins..." RESET); res = -1; }
  else { PRINTLN(BOLD MAGENTA "tie." RESET); res = 0; }

  total_chips += bet * res;
  write_c(total_chips);
}

// ---

struct termios t_old;

void restore_terminal() { 
  tcsetattr(0, TCSANOW, &t_old); 
}

void init_terminal() { 
  tcgetattr(0, &t_old);
  struct termios t = t_old;
  t.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, 0, &t);
}

void flush_read(c8* c) {
  int flags = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, flags | O_NONBLOCK);
  while (read(0, c, 1) > 0);
  fcntl(0, F_SETFL, flags);
  read(0, c, 1);
}

void scan(const char *fmt, void *arg) {
  int flags = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, flags | O_NONBLOCK);
  while (read(0, &c, 1) > 0);
  fcntl(0, F_SETFL, flags);
  struct termios t;
  tcgetattr(0, &t);
  struct termios t_block = t;
  t_block.c_lflag |= (ICANON | ECHO);
  tcsetattr(0, TCSANOW, &t_block);
  scanf(fmt, arg);
  tcsetattr(0, TCSANOW, &t);
}

void msleep(u16 time) {
  usleep(time * 1e3);
}

void write_c(u32 value) {
  FILE* f = fopen("data", "wb");
  ASSERT(f, "cant open coins file")
  fwrite(&value, sizeof(u32), 1, f);
  fclose(f);
}

u32 read_c() {
  u32 value = 0;
  FILE* f = fopen("data", "rb");
  if (!f) return 0;
  fread(&value, sizeof(u32), 1, f);
  fclose(f);
  return value;
}

// ---

int main() {
  srand(time(0));
  init_terminal();
  atexit(restore_terminal);
  do { game(); flush_read(&c); } while (c != 27);
}
