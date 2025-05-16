// Simple array test for 8086 compiler

// Function to work with arrays
void test_array() {
    // Local array declaration
    int numbers[10];
    int i;
    
    // Initialize array
    for (i = 0; i < 10; i++) {
        numbers[i] = i * 10;
    }
    
    // Array access
    int first = numbers[0];
    int last = numbers[9];
    
    // Array element assignment
    numbers[5] = 500;
}

// Main function
int main() {
    test_array();
    return 0;
}
