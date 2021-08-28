#include <stdio.h>
#include <stdlib.h>

void print(int zs[], int size);
void test(int allo, int times, int arr[]);

/**
 * @brief 测试内存对齐，使用 `gcc -m32` 或者 `gcc -m64` 指定编译的内存寻址宽度。
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    int times = 4;
    int num;
    int arr[times];

    if (argc > 2) {
        num = atoi(argv[2]);
    }
    else 
    {
        num = 1;
        printf("Test memory alainment.\n\nUsage: %s -w {size of allocaiont in byte}\n", argv[0]);
        exit(-1);
    }
    test(num, times, arr);
    print(arr, times);

    return 0;
}

void test(int allo, int times, int arr[])
{
    int *pre = NULL;
    for (int i = 0; i < times; i++)
    {
        int *p = malloc(allo);
        printf("%i. malloc(%i) returns: %p\n", i, allo, p);
        if (pre == 0) {
            arr[i] = 0;
            pre = p;
        }
        else
        {
            arr[i] = (int)p - (int)pre;  // convert pointer to int to compute difference
            pre = p;
        }
    }
}

void print(int sz[], int size)
{
    for (int i = 0; i<size; i++)
    {
        printf("%u allocated size: %u bytes\n", i, sz[i]);
    }
    printf("\n");
}