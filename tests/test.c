#include <stdio.h>
#include <stdlib.h>

typedef double Double;

int main()
{
    Double *arr = NULL;

    arr = realloc(arr, 4);

    arr[0] = 45.2;

    printf("%f \n", arr[0]);
    printf("%f \n", arr[1]);
    printf("%f \n", arr[2]);
    printf("%f \n", arr[3]);
    printf("%f \n", arr[4]);
    free(arr);

    return 0;
}
