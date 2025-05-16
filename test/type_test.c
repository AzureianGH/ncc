// Test file for type operations
// This file tests various type operations including char and unsigned int

// Global variables of different types
int global_int = 100;
char global_char = 'A';
unsigned global_unsigned = 65000;

// Function to test char operations
void test_char_operations() {
    char c1 = 'A';
    char c2 = 'B';
    char result;
    
    // Char arithmetic
    result = c1 + c2;    // 'A' + 'B' = 65 + 66 = 131 (truncated to 'C')
    result = c2 - c1;    // 'B' - 'A' = 66 - 65 = 1
    result = c1 * 2;     // 'A' * 2 = 65 * 2 = 130 (truncated)
    
    // Char comparisons
    if (c1 < c2) {
        result = 'Y';    // Should execute (A < B)
    }
    
    // Char to int conversion (implicit)
    int i1 = c1;         // i1 = 65
    
    // Char array access
    char str[10] = "Hello";
    char ch = str[0];    // 'H'
    str[1] = 'a';        // "Hallo"
}

// Function to test unsigned operations
void test_unsigned_operations() {
    unsigned u1 = 65000;
    unsigned u2 = 10000;
    unsigned result;
    
    // Unsigned arithmetic
    result = u1 + u2;    // 65000 + 10000 = 75000 (valid in unsigned range)
    result = u1 - u2;    // 65000 - 10000 = 55000
    result = u1 * 2;     // 65000 * 2 = 130000 (will wrap around)
    result = u1 / 1000;  // 65000 / 1000 = 65
    
    // Unsigned comparisons
    if (u1 > u2) {
        result = 1;      // Should execute (65000 > 10000)
    }
    
    // Test interesting unsigned case (wrap around)
    unsigned max_value = 65535;
    unsigned overflow = max_value + 1;  // Should be 0
    
    // Unsigned and signed interaction
    int negative = -1;
    if (negative < u1) {
        // This should NOT execute in unsigned comparison
        // since -1 as unsigned is 65535, which is > 65000
        result = 999;
    }
}

// Function to test mixed type operations
void test_mixed_types() {
    int i = 1000;
    char c = 'A';
    unsigned u = 50000;
    
    // Mixed arithmetic (with implicit conversions)
    int result1 = i + c;        // 1000 + 65 = 1065
    unsigned result2 = u + i;   // 50000 + 1000 = 51000
    int result3 = c * i;        // 65 * 1000 = 65000
    
    // Mixed comparisons
    if (c < i) {
        result1 = 42;           // Should execute (65 < 1000)
    }
    
    // Explicit conversions
    char narrow = (char)i;      // 1000 truncated to a char
    unsigned wide = (unsigned)c; // 65 as unsigned
}

int main() {
    test_char_operations();
    test_unsigned_operations();
    test_mixed_types();
    
    return 0;
}
