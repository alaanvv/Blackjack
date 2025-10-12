#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MIN(x, y) (x < y ? x : y)

void init_game();

typedef enum {
  ACE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  TEN,
  JACK,
  QUEEN,
  KING
} Rank;

typedef enum { SPADES, HEARTS, DIAMONDS, CLUBS } Suit;

typedef struct {
  Rank rank;
  Suit suit;
} Card;
int is_soft(Card *hand, int size);

const char *ranks[] = {"Ace", "2", "3",  "4",    "5",     "6",   "7",
                       "8",   "9", "10", "Jack", "Queen", "King"};

const char *suits[] = {"spades", "hearts", "diamonds", "clubs"};

// ---

Card shoe[52];
int shoe_size;

Card phand[16];
int phand_size;

Card dhand[16];
int dhand_size;

void shuffle() {
  for (int i = 0; i < 13; i++)
    for (int j = 0; j < 4; j++)
      shoe[(i * 4) + j] = (Card){i, j};

  for (int i = 51; i > 0; i--) {
    int j = rand() % (i + 1);
    Card tmp = shoe[i];
    shoe[i] = shoe[j];
    shoe[j] = tmp;
  }

  shoe_size = 52;
}

void take_card(Card *hand) { *hand = shoe[--shoe_size]; }

int has_ace(Card *hand, int size) {
  for (int i = 0; i < size; i++)
    if (hand[i].rank == ACE)
      return 1;

  return 0;
}

int hand_points(Card *hand, int size, int should_soft) {
  int acc = 0;

  for (int i = 0; i < size; i++)
    acc += MIN(hand[i].rank + 1, 10);

  if (should_soft && is_soft(hand, size))
    acc += 10;

  return acc;
}

int is_soft(Card *hand, int size) {
  if (has_ace(hand, size) && hand_points(hand, size, 0) <= 11)
    return 1;
  return 0;
}

void display(int show_hole) {
  printf("\033[H\033[2J");
  if (dhand_size == 2 && !show_hole)
    printf("Dealer's hand (?):\n");
  else
    printf("Dealer's hand (%s%d):\n", is_soft(dhand, dhand_size) ? "soft " : "",
           hand_points(dhand, dhand_size, 1));
  for (int i = 0; i < dhand_size; i++) {
    if (dhand_size == 2 && i == 1 && !show_hole)
      printf("- ???\n");
    else
      printf("- %s of %s\n", ranks[dhand[i].rank], suits[dhand[i].suit]);
  }

  printf("Player's hand (%s%d):\n", is_soft(phand, phand_size) ? "soft " : "",
         hand_points(phand, phand_size, 1));
  for (int i = 0; i < phand_size; i++)
    printf("- %s of %s\n", ranks[phand[i].rank], suits[phand[i].suit]);
}

void players_turn() {
  char c;
  while (1) {
    display(0);
    if (hand_points(phand, phand_size, 1) > 21) {
      printf("You busted!\n");
      printf("Press anything to play again\n");
      read(0, &c, 1);
      return init_game();
    }

    printf("h - hit; s - stand;\n");
    read(0, &c, 1);
    if (c == 'h')
      take_card(&phand[phand_size++]);
    if (c == 's') {
      if (hand_points(phand, phand_size, 1) == 11 &&
          has_ace(phand, phand_size)) {
        printf("Soft win!\n");
        printf("Press anything to play again\n");
        read(0, &c, 1);
        return init_game();
      }

      return;
    }
  }
}

void dealers_turn() {
  display(1);
  while (hand_points(dhand, dhand_size, 1) < 17) {
    sleep(1);
    take_card(&dhand[dhand_size++]);
    display(1);
  }

  char c;
  if (hand_points(dhand, dhand_size, 1) > 21) {
    printf("Dealer busted!\n");
    printf("Press anything to play again\n");
    read(0, &c, 1);
    return init_game();
  }

  if (hand_points(dhand, dhand_size, 1) == hand_points(phand, phand_size, 1)) {
    printf("Tie!\n");
    printf("Press anything to play again\n");
    read(0, &c, 1);
    return init_game();
  }
  if (hand_points(dhand, dhand_size, 1) > hand_points(phand, phand_size, 1)) {
    printf("Dealer wins!\n");
    printf("Press anything to play again\n");
    read(0, &c, 1);
    return init_game();
  }

  else {
    printf("Player wins!\n");
    printf("Press anything to play again\n");
    read(0, &c, 1);
    return init_game();
  }
}

void init_game() {
  shuffle();
  phand_size = 0;
  dhand_size = 0;

  take_card(&phand[phand_size++]);
  take_card(&phand[phand_size++]);
  take_card(&dhand[dhand_size++]);
  take_card(&dhand[dhand_size++]);

  players_turn();
  dealers_turn();
}
// ---
struct termios t_old;
void restore_terminal() { tcsetattr(0, TCSANOW, &t_old); }
int main() {
  srand(time(0));

  tcgetattr(0, &t_old);
  atexit(restore_terminal);

  struct termios t = t_old;
  t.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, 0, &t);

  init_game();
}
