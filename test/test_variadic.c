#define BITS16
#include <stdarg.h>
#include "bootloader.h"

// Function to convert integer to string
void itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;
    
    // Handle 0 explicitly
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    // Handle negative numbers
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
    
    // Process digits
    while (num != 0) {
        int remainder = num % base;
        str[i++] = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
        num = num / base;
    }
    
    // Append negative sign for base 10
    if (isNegative) {
        str[i++] = '-';
    }
    
    // Terminate string
    str[i] = '\0';
    
    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// Simple printf-like function that prints a format string with variables
void mini_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    while (*format) {
        if (*format == '%') {
            format++; // Skip the '%'
            
            // Handle format specifiers
            switch (*format) {
                case 'd': { // Integer
                    int value = va_arg(args, int);
                    char buffer[16];
                    itoa(value, buffer, 10);
                    writeString(buffer);
                    break;
                }
                case 's': { // String
                    char* str = va_arg(args, char*);
                    writeString(str);
                    break;
                }
                case 'c': { // Character
                    // Note: char is promoted to int when passed through ...
                    char c = (char)va_arg(args, int);
                    writeChar(c);
                    break;
                }
                case 'x': { // Hex
                    int value = va_arg(args, int);
                    char buffer[16];
                    itoa(value, buffer, 16);
                    writeString(buffer);
                    break;
                }
                default:
                    // Unknown format specifier, just print it
                    writeChar(*format);
                    break;
            }
        } else {
            // Regular character, just print it
            writeChar(*format);
        }
        format++;
    }
    
    va_end(args);
}

// Function with a variable number of integer arguments
// Returns the sum of all the arguments
int sum_ints(int count, ...) {
    va_list args;
    va_start(args, count);
    
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += va_arg(args, int);
    }
    
    va_end(args);
    return sum;
}

// Test function demonstrating different argument types
void test_different_types(int num, ...) {
    va_list args;
    va_start(args, num);
    
    writeString("Testing different argument types:\r\n");
    
    // First arg - char
    char c = (char)va_arg(args, int); // char is promoted to int in varargs
    mini_printf("  Char argument: %c\r\n", c);
    
    // Second arg - int
    int i = va_arg(args, int);
    mini_printf("  Int argument: %d\r\n", i);
    
    // Third arg - string (char*)
    char* s = va_arg(args, char*);
    mini_printf("  String argument: %s\r\n", s);
    
    // Fourth arg - pointer to int
    int* p = va_arg(args, int*);
    mini_printf("  Pointer argument value: %d\r\n", *p);
    
    va_end(args);
}

// Simple function to gather arguments and pass through
void forward_args(char* prefix, ...) {
    va_list args;
    va_start(args, prefix);
    
    // Just forward all arguments to mini_printf
    writeString(prefix);
    writeString(" ");
    
    // Get the next argument which is our format string
    char* format = va_arg(args, char*);
    
    // Forward the arguments
    mini_printf(format, va_arg(args, int), va_arg(args, char*));
    
    va_end(args);
}

// Test the variadic functions
void test_variadic() {
    writeString("Testing variadic functions:\r\n");
    
    // Test mini_printf
    mini_printf("Hello %s! The answer is %d.\r\n", "world", 42);
    mini_printf("Hex value: 0x%x, char: %c\r\n", 0xABCD, 'Z');
    
    // Test sum_ints
    int result = sum_ints(5, 10, 20, 30, 40, 50);
    mini_printf("Sum of 5 integers: %d\r\n", result);

    // Test with different argument types
    int test_value = 99;
    test_different_types(4, 'A', 42, "test string", &test_value);
    
    // Test argument forwarding
    forward_args("Forwarded:", "Int: %d, String: %s\r\n", 123, "hello");
}

void _after_diskload() {
    clearScreen();
    writeString("NCC Variadic Arguments Test\r\n");
    writeString("===========================\r\n");
    
    // Run the variadic function tests
    test_variadic();
    
    // Halt system after testing
    haltForever();
}
