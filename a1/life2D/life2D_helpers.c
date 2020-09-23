#include <stdio.h>
#include <stdlib.h>


void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
    int original_board[num_rows*num_cols];
    for (int k = 0; k < num_rows*num_cols; k++){
      original_board[k] = board[k];
    }
    for (int i = 1; i < num_cols - 1; i++){
      for (int j = 1; j < num_rows - 1; j += 1){
        int x = 0;
        if (original_board[i + (j * num_cols) + 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) - 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) - num_cols] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) - num_cols + 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) - num_cols - 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) + num_cols] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) + num_cols + 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols) + num_cols - 1] == 1){
          x++;
        }
        if (original_board[i + (j * num_cols)] == 0){
          if(x == 2 || x == 3){
            board[i + (j * num_cols)] = 1;
          }
        }
        else{
          if (x < 2 || x > 3){
            board[i + (j * num_cols)] = 0;
          }
        }
      }
    }
}
