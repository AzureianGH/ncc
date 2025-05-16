// Array operations test for 8086 compiler
// This file tests various array operations, including:
// - Array declaration and initialization
// - Array element access
// - Array element assignment
// - Multi-dimensional arrays
// - Array parameters

// Global array declaration
int global_array[5] = {1, 2, 3, 4, 5};
char char_array[10] = "Hello";

// Function to sum an array
int sum_array(int arr[], int size) {
    int sum = 0;
    int i;
    
    for (i = 0; i < size; i++) {
        sum = sum + arr[i];
    }
    
    return sum;
}

// Function to modify an array
void modify_array(int arr[], int size, int value) {
    int i;
    
    for (i = 0; i < size; i++) {
        arr[i] = arr[i] + value;
    }
}

// Test multi-dimensional array
void test_2d_array() {
    int matrix[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    int sum = 0;
    int i, j;
    
    // Calculate sum of diagonal elements
    for (i = 0; i < 3; i++) {
        sum = sum + matrix[i][i];
    }
    
    // Modify elements
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            matrix[i][j] = matrix[i][j] * 2;
        }
    }
}

// Test pointers and arrays
void test_pointers_and_arrays() {
    int arr[5] = {10, 20, 30, 40, 50};
    int *ptr = arr;  // Pointer to the first element
    
    // Access via pointer
    *ptr = 15;       // Modify arr[0]
    ptr = ptr + 1;   // Move to next element
    *ptr = 25;       // Modify arr[1]
    
    // Access via array indexing with pointer
    ptr[2] = 35;     // Modify arr[3]
    
    // Pointer arithmetic
    *(ptr + 2) = 45; // Modify arr[3]
}

// Main function
int main() {
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
    
    // Call array functions
    int total = sum_array(numbers, 10);
    modify_array(numbers, 10, 5);
    
    // Use global array
    int global_sum = sum_array(global_array, 5);
    
    // Test character arrays
    char message[20] = "Testing arrays";
    char c = message[0];  // 'T'
    message[7] = 'A';     // Change to "Testing Arrays"
    
    // Test pointer operations with arrays
    test_pointers_and_arrays();
    
    // Test multi-dimensional arrays
    test_2d_array();
    
    return 0;
}
