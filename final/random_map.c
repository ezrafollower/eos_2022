#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// row and col size must be even integers in memory game
#define ROL_SIZE 4
#define COL_SIZE 4
#define CAT_NUM 6

void print_pattern(int pattern[ROL_SIZE][COL_SIZE])
{
    for (int i = 0; i < ROL_SIZE; i++){
        for (int j = 0; j < COL_SIZE; j++){
            printf("%d ", pattern[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void Generator(int array[ROL_SIZE][COL_SIZE], int row, int col, int cat_num)
{
    int cat = 1 + rand() / (RAND_MAX / cat_num);
    if (col == COL_SIZE){
        col = 0;
        row++;
    }
    if (row >= ROL_SIZE / 2){
        return;
    }
    array[row][col] = cat;
    Generator(array, row, col+1, cat_num);
    array[ROL_SIZE - 1 - row][COL_SIZE - 1 - col] = cat;
    return;
}

void swap(int *a, int *b)
{
    int *temp = a;
    a = b;
    b = temp;
}

void Change_order(int pattern[ROL_SIZE][COL_SIZE], int times)
{
    int a, b;
    for (int i = 0; i < times; i++){
        a = i % ROL_SIZE;
        b = (a + rand() / (RAND_MAX / 4)) % 4;
        // printf("a b = %d %d\n", a, b);
        for (int j = 0; j < ROL_SIZE; j++){
            // swap(&pattern[j][a], &pattern[j][b]);
            int temp = pattern[j][a];
            pattern[j][a] = pattern[j][b];
            pattern[j][b] = temp;
        }
        b = (a + rand() / (RAND_MAX / 4)) % 4;
        for (int j = 0; j < COL_SIZE; j++){
            // swap(&pattern[j][a], &pattern[j][b]);
            int temp = pattern[a][j];
            pattern[a][j] = pattern[b][j];
            pattern[b][j] = temp;
        }
    }
}

void Get_random_pattern(int pattern[ROL_SIZE][COL_SIZE], int rol_size, int col_size, int cat_num)
{
    // Set random seed
    srand(time(NULL));
    // Generate the pattern map
    Generator(pattern, 0, 0, cat_num);
    Change_order(pattern, 8);
}

void Get_uniform_pattern(int pattern[ROL_SIZE][COL_SIZE], int rol_size, int col_size, int cat_num)
{
    // Set random seed
    srand(time(NULL));
    // Generate the pattern map
    int tol_num = rol_size * col_size;
    int cat_count = 1;
    for (int i = 0; i < rol_size/2; i++){
        for (int j = 0; j < col_size; j++){
            pattern[i][j] = cat_count;
            pattern[rol_size-i-1][col_size-j-1] = cat_count;
            cat_count++;
            if (cat_count > cat_num){
                cat_count -= cat_num;
            }
        }
    }
    Change_order(pattern, 8);
}

int main()
{
    clock_t start, finish;
    // 0 means no pattern
    // category number from 1~N
    int pattern1[ROL_SIZE][COL_SIZE] = {0};
    int pattern2[ROL_SIZE][COL_SIZE] = {0};

    start = clock();
    Get_uniform_pattern(pattern1, ROL_SIZE, COL_SIZE, CAT_NUM);
    Get_random_pattern(pattern2, ROL_SIZE, COL_SIZE, CAT_NUM);
    finish = clock();
    printf("Generate time: %f\n", (double)(finish - start) / CLOCKS_PER_SEC);

    printf("Uniform pattern:\n");
    print_pattern(pattern1);
    printf("Random  pattern:\n");
    print_pattern(pattern2);
    
    return 0;
}