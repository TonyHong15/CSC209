#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }
    int num, i = 0;
    int num_rows, num_cols, states;
    num_rows = strtol(argv[1], NULL, 10);
    num_cols = strtol(argv[2], NULL, 10);
    int board[(num_rows*num_cols)];
    states = strtol(argv[3], NULL, 10);
    while (scanf("%d", &num) != EOF){
      board[i] = num;
      i++;
    }
    for (int j = 0; j < states; j++){
      print_state(board, num_rows, num_cols);
      update_state(board, num_rows, num_cols);
    }
    return 0;
}
