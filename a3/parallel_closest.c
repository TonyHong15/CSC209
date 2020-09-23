#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    // Base Case (solve directly)
    if (n < 4 || pdmax == 0) {
      return closest_serial(p, n);
    }
    //Recursive case (divide task to worker processes)
    else{
      struct Point *split_p[2];
      struct Point p1[n / 2];
      struct Point p2[n - n / 2];
      //Store first half of p in one list and the second half in the other
      for (int i = 0; i < n; i++) {
        if (i < n / 2) {
          p1[i] = p[i];
        }
        else {
          p2[i - (n / 2)] = p[i];
        }
      }
      split_p[0] = p1;
      split_p[1] = p2;

      //creating space for 2 pipes for when we fork both child processes
      int fd[2][2];
      int pid;
      for (int i = 0; i < 2; i++) {
        if (pipe(fd[i]) == -1) {
          perror("pipe");
          exit(1);
        }
        if ((pid = fork()) == -1) {
          perror("fork");
          exit(1);
        }
        else if (pid == 0) {
          if (close(fd[i][0]) == -1) {
            perror("close read end from child");
            exit(1);
          }
          //Recursively solve shortest distance and write result for parent to read
          double result;
          result = closest_parallel(split_p[i], n / 2, pdmax - 1, pcount);
          if (write(fd[i][1], &result, sizeof(double)) != sizeof(double)) {
            perror("write from child");
            exit(1);
          }
          if (close(fd[i][1]) == -1) {
            perror("close read end from child");
            exit(1);
          }
          exit(*pcount);
        }
      }

      int status[2];
      double result[2];
      for (int i = 0; i < 2; i++){
        if (close(fd[i][1]) == -1) {
          perror("close write end from parent");
        exit(1);
        }
      }

      //Determine total number of worker processes using child exit codes
      for (int i = 0; i < 2; i++) {
        if (wait(&(status[i])) == -1){
          perror("wait");
          exit(1);
        }
        if(WIFEXITED(status[i])) {
          *pcount += WEXITSTATUS(status[i]);
        }
      }
      *pcount += 2;

      for (int i = 0; i < 2; i++){
        if(read(fd[i][0], &(result[i]), sizeof(double)) != sizeof(double)) {
          perror("read");
          exit(1);
        }
        if (close(fd[i][0]) == -1) {
          perror("close read end from parent");
          exit(1);
        }
      }

      double d = min(result[0], result[1]);
      // Build an array strip[] that contains points close (closer than d)
      // to the line passing through the middle point.
      struct Point *strip = malloc(sizeof(struct Point) * n);
      if (strip == NULL) {
        perror("malloc");
        exit(1);
      }
      int j = 0;
      int mid = n / 2;
      struct Point mid_point = p[mid];
      for (int i = 0; i < n; i++) {
        if (abs(p[i].x - mid_point.x) < d) {
          strip[j] = p[i], j++;
        }
      }
      // Find the closest points in strip.  Return the minimum of d and closest distance in strip[].
      double new_min = min(d, strip_closest(strip, j, d));
      free(strip);
      return new_min;
    }
    return 0.0;
}
