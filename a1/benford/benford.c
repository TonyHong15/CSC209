#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }
    int tally[BASE];
    for (int i = 0; i < BASE; i++){
      tally[i] = 0;
    }
    int position_from_left = strtol(argv[1], NULL, 10);
    int curr_num;
    if (argc == 2){
      while (scanf("%d", &curr_num) != EOF){
        add_to_tally(curr_num, position_from_left, tally);
      }

    }
    else{
      FILE *file = fopen(argv[2], "r");
      if (file == NULL){
        printf("could not enter file");
        return 1;
      }
      while(fscanf(file, "%d", &curr_num) == 1){
        add_to_tally(curr_num, position_from_left, tally);
      }
      if (fclose(file) != 0){
        printf("error closing file" );
        return 1;
      }
    }
    for (int i = 0; i < BASE; i++){
      printf("%ds: %d\n", i, tally[i]);
    }
    return 0;
}
