#include <stdio.h>

int add(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    printf("Start\n");
    
    int x = 10;
    int y = 20;
    
    printf("x = %d, y = %d\n", x, y);
    
    int z = add(x, y);
    
    printf("z = %d\n", z);
    printf("Done\n");
    
    return 0;
}
