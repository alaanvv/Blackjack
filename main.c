#include <string.h>
#include <termios.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

#define MIN(x, y) (x < y ? x : y)
#define RAND(min, max) (rand() % (max - min) + min)
#define PRINTLN(...) { printf(__VA_ARGS__); printf("\n"); }

#define RED   "\033[31m"
#define GREEN "\033[32m"
#define BLUE  "\033[34m"
#define BOLD  "\033[1m"
#define RESET "\033[0m"

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

int is_soft(Deck deck);
void flush_read(c8* c);
void msleep(u16 time);

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
  char ascii[7][128] = { 0 };

  char buffer[32];
  for (int i = 0; i < deck.size; i++) {
    if (!((i64) pow(2, i) & face_mask)) {
      strcat(ascii[0], BOLD "┌───────┐");
      strcat(ascii[1], BOLD "│" BLUE "+.~*+.~" RESET BOLD "│");
      strcat(ascii[2], BOLD "│" BLUE "*+.~*+." RESET BOLD "│");
      strcat(ascii[3], BOLD "│" BLUE "~*+.~*+" RESET BOLD "│");
      strcat(ascii[4], BOLD "│" BLUE ".~*+.~*" RESET BOLD "│");
      strcat(ascii[5], BOLD "│" BLUE "+.~*+.~" RESET BOLD "│");
      strcat(ascii[6], BOLD "└───────┘" RESET);
      continue;
    }

    strcat(ascii[0], BOLD "┌───────┐");
    sprintf(buffer,  "│ %s%s%s    │", deck.cards[i].suit >= HEARTS ? RED : "", ranks_ascii[deck.cards[i].rank], RESET);
    strcat(ascii[1], buffer);
    strcat(ascii[2], BOLD "│       │");
    sprintf(buffer,  "│   %s%s%s   │", deck.cards[i].suit >= HEARTS ? RED : "", suits_ascii[deck.cards[i].suit], RESET);
    strcat(ascii[3], buffer);
    strcat(ascii[4], BOLD "│       │");
    sprintf(buffer,  "│    %s%s%s │", deck.cards[i].suit >= HEARTS ? RED : "", _ranks_ascii[deck.cards[i].rank], RESET);
    strcat(ascii[5], buffer);
    strcat(ascii[6], BOLD "└───────┘" RESET);
  }

  for (int i = 0; i < 7; i++)
    printf("%s\n", ascii[i]);
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

Deck shoe, phand, dhand;

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

  if (dhand.size == 2 && !show_hole) PRINTLN("Dealer's hand (%d + ?):", MIN(dhand.cards[0].rank + 1, 10))
  else PRINTLN("Dealer's hand (%s%d):", is_soft(dhand) ? "soft " : "", hand_points(dhand, 1))
  print_deck_ascii(dhand, show_hole ? ~0 : 1);
  PRINTLN("");

  PRINTLN("Your hand (%s%d):", is_soft(phand) ? "soft " : "", hand_points(phand, 1))
  print_deck_ascii(phand, ~0);
  PRINTLN("");
}

void players_turn() {
  while (!is_bust(phand) && !is_blackjack(phand)) {
    display(0);
    PRINTLN("h - hit; s - stand;")
    flush_read(&c);

    if (c == 'h') { move_card(&shoe, &phand); display(0); }
    if (c == 's') break;
    if (c == 27) exit(0);
  }
}

void dealers_turn() {
  display(1);
  msleep(500);
  while (hand_points(dhand, 1) < 17) {
    move_card(&shoe, &dhand);
    display(1);
    msleep(500);
  }
}

void game() {
  empty_deck(&phand);
  empty_deck(&dhand);
  fill_deck(&shoe);
  shuffle_deck(&shoe);

  display(0);
  msleep(500);
  move_card(&shoe, &dhand);
  display(0);
  msleep(500);
  move_card(&shoe, &dhand);
  display(0);
  msleep(500);
  move_card(&shoe, &phand);
  display(0);
  msleep(500);
  move_card(&shoe, &phand);
  display(0);

  players_turn();
  if (!is_blackjack(phand) && !is_bust(phand)) dealers_turn();
  else display(1);

  if (is_blackjack(phand)) PRINTLN(BOLD GREEN "Blackjack!" RESET)
  else if (is_bust(dhand)) PRINTLN(BOLD GREEN "Dealer busted!" RESET)
  else if (is_bust(phand)) PRINTLN(BOLD RED "You busted..." RESET)
  else if (is_blackjack(dhand)) PRINTLN(BOLD RED "Dealer blackjacked..." RESET)
  else if (is_21(phand) && is_soft(phand)) PRINTLN(BOLD GREEN "Soft win!" RESET)
  else if (is_21(dhand) && is_soft(dhand)) PRINTLN(BOLD RED "Dealer soft wins..." RESET)
  else if (is_21(phand)) PRINTLN(BOLD GREEN "You got 21!" RESET)
  else if (is_21(dhand)) PRINTLN(BOLD RED "Dealer got 21..." RESET)
  else if (hand_points(phand, 1) > hand_points(dhand, 1)) PRINTLN(BOLD GREEN "You win..." RESET)
  else if (hand_points(dhand, 1) > hand_points(phand, 1)) PRINTLN(BOLD RED "Dealer wins..." RESET)
  else PRINTLN("Tie.")
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
  atexit(restore_terminal);
}

void flush_read(c8* c) {
  int flags = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, flags | O_NONBLOCK);
  while (read(0, c, 1) > 0);
  fcntl(0, F_SETFL, flags);
  read(0, c, 1);
}

void msleep(u16 time) {
  usleep(time * 1e3);
}

// ---

int main() {
  srand(time(0));
  init_terminal();
  do { game(); flush_read(&c); } while (c != 27);
}
