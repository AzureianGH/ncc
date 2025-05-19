#include <stdio.h>

/* Test program for pointer handling in NCC compiler */
int main() {
    /* Test 1: Basic pointer dereferencing */
    int x = 42;
    int *p = &x;
    char c = 'A';
    char *cp = &c;
    
    /* Print values */
    printf("x = %d, *p = %d\n", x, *p);
    printf("c = %c, *cp = %c\n", c, *cp);
    
    /* Test 2: Pointer arithmetic */
    int nums[5] = {10, 20, 30, 40, 50};
    int *np = nums;
    
    printf("nums[0] = %d, *np = %d\n", nums[0], *np);
    np = np + 1;  /* Should advance by sizeof(int) */
    printf("nums[1] = %d, *np = %d\n", nums[1], *np);
    np = np + 2;  /* Should advance by 2*sizeof(int) */
    printf("nums[3] = %d, *np = %d\n", nums[3], *np);
    
    /* Test 3: Array indexing */
    printf("nums[2] = %d\n", nums[2]);
    printf("np[-1] = %d\n", np[-1]);  /* Should access nums[2] */
    
    /* Test 4: Char array and pointer */
    char str[] = "Hello";
    char *sp = str;
    
    printf("str[1] = %c, sp[1] = %c\n", str[1], sp[1]);
    sp = sp + 2;
    printf("sp[0] = %c, sp[1] = %c\n", sp[0], sp[1]);
    
    /* Test 5: Pointer difference */
    int *start = nums;
    int *end = &nums[4];
    int count = end - start;
    printf("Pointer difference: %d elements\n", count);
    
    return 0;
}
