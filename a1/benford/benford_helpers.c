#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int digits = 0;
    while (num != 0){
      num = num / BASE;
      digits++;
    }
  //  printf("num of digits %d\n", digits);
    return digits;
}

int get_ith_from_right(int num, int i) {
    for (int j = 0; j <= i; j++){
      num = num / BASE;
    }
    return (num % BASE);

}

int get_ith_from_left(int num, int i) {
    int digits = count_digits(num);
    int digit = get_ith_from_right(num, digits - i - 2);
    //printf("digit from left %d\n", digit);
    return digit;
}

void add_to_tally(int num, int i, int *tally) {
    int digit = get_ith_from_left(num, i);
    tally[digit] += 1;
}
