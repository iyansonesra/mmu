#include "tdmm.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <regex.h>
#include <unistd.h>
#include <errno.h>
#include <byteswap.h>
#include <bits/mman-linux.h>
#include <time.h>

#define size 5

void* ptr[size];

int main() {
    int x = 10;
    void* stack_bot = &x;
    t_init(FIRST_FIT, stack_bot);
    double avgMemoryUtil = 0;

    for(int x = 0; x < 5; x++) {
      ptr[x] = t_malloc(10);
    }

    t_free(ptr[3]);
    t_free(ptr[2]);


  //   for(int x = 1; x <= size; x++) {
  //   ptr[x-1] = t_malloc(10);
  //   //avgMemoryUtil += percentageMemUtil();
  // }

    // clock_t begin = clock();
    // void * checking = t_malloc(10);

    //  clock_t end = clock();
    // double mallocTime = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Malloc Time: %lf\n", mallocTime);

    // clock_t begin1 = clock();
    // t_free(checking);

    //  clock_t end1 = clock();
    // double freeTime = (double)(end1 - begin1) / CLOCKS_PER_SEC;
    // printf("Feee Time: %lf\n", freeTime);

    // Initialize random number generator
    // srand(time(NULL));

   
    // int malloc_count = 0;
    // double total_memory_utilization = 0;
    // for (int x = 1; x <= size; x++) {
    //     int action = rand() % 2; // Randomly choose between malloc (0) and free (1)
    //     if (action == 0) {
    //         // Malloc
    //         ptr[malloc_count] = t_malloc(10);
    //         total_memory_utilization += percentageMemUtil();
    //         malloc_count++;
    //     } else {
    //         // Free
    //         if (malloc_count > 0) {
    //             t_free(ptr[malloc_count - 1]);
    //             malloc_count--;
    //             total_memory_utilization += percentageMemUtil();
    //         }
    //     }

    //     avgMemoryUtil = total_memory_utilization / x;
    //     // Write to CSV
    // }

    // // Open the CSV file for writing
    // FILE *csv_file = fopen("BUDDYmemUtil1000000.csv", "w");
    // if (csv_file == NULL) {
    //     printf("Error opening file for writing.\n");
    //     return 1;
    // }

    // // Write the CSV header
    // fprintf(csv_file, "Action,Memory Utilization\n");

    // for (int x = 1; x <= size; x++) {
    //     int action = rand() % 2; // Randomly choose between malloc (0) and free (1)
    //     if (action == 0) {
    //         // Malloc
    //         ptr[malloc_count] = t_malloc(1000000);
    //         total_memory_utilization += percentageMemUtil();
    //         malloc_count++;
    //     } else {
    //         // Free
    //         if (malloc_count > 0) {
    //             t_free(ptr[malloc_count - 1]);
    //             malloc_count--;
    //             total_memory_utilization += percentageMemUtil();
    //         }
    //     }

    //     avgMemoryUtil = total_memory_utilization / x;
    //     // Write to CSV
    //     fprintf(csv_file, "%d,%lf\n", x, avgMemoryUtil);
    // }

    // // Close the CSV file
    // fclose(csv_file);

    // return 0;

     // int  x = 10;
  // void * stack_bot = &x;
  // t_init(BEST_FIT, stack_bot);
  // double avgMemoryUtil = 0;

  // for(int x = 1; x <= size; x++) {
  //   ptr[x-1] = t_malloc(10);
  //   avgMemoryUtil += percentageMemUtil();
  // }

  // avgMemoryUtil /= size;
  // printf("%lf\n", avgMemoryUtil);
   




































//   int  x = 10;
//    void * stack_bot = &x;

  
// t_init(FIRST_FIT, stack_bot);



//   //  clock_t end = clock();
//   // double mallocTime = (double)(end - begin) / CLOCKS_PER_SEC;
// //printf("Malloc Time: %lf\n", mallocTime);



// // int indices[size];
// //     for (int i = 0; i < size; i++) {
// //         indices[i] = i;
// //     }

//     // Shuffle the array of indices
//     //shuffle(indices, size);
//   // clock_t begin2 = clock();
//  //freeRandomly(indices, size);
//   clock_t begin = clock();
//   void * temp = malloc(10);
//    clock_t end2 = clock();
//  double mallocTime = (double)(end2 - begin) / CLOCKS_PER_SEC;
// printf("Malloc Time: %lf\n", mallocTime);

//  clock_t begin1 = clock();
//   t_free(temp);
//    clock_t end3 = clock();
//  double freeTime = (double)(end3 - begin1) / CLOCKS_PER_SEC;
// printf("Free Time: %lf\n", freeTime);


// printf("Total Time %lf\n", mallocTime + freeTime);

//   return 0;
// }

}

 
