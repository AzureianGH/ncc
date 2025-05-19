#include "test/bootloader.h"

// Test for inline assembly with operands
void testInlineAssemblyOperands() {
    // Test a simple variable access
    int x = 42;
    __asm("mov ax, %0" : : "r"(x));  // Should load x into ax

    // Test with multiple operands
    int a = 10, b = 20;
    __asm("mov ax, %0\nmov bx, %1" : : "r"(a), "r"(b));  // Should load a into ax and b into bx
    
    // Test with more complex expressions
    int array[5] = {1, 2, 3, 4, 5};
    int index = 2;
    __asm("mov ax, %0" : : "r"(array[index]));  // Should load array[2] (value 3) into ax
}

int main() {
    testInlineAssemblyOperands();
    return 0;
}
