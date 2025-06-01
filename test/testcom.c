void main()
{
    clearscreen();  // Clear the screen
    printstring("Input a string: $");
    char* userInput = getuserinput();  // Get user input from the function
    clearscreen();  // Clear the screen
    printstring("You entered: $");
    printstring(userInput);  // Call the function to get user input and print it
}

void printstring(char* str)
{
    __asm("mov dx, [bp+4]");  // Get 'str' argument from stack
    __asm("mov ah, 0x09");    // Set AH = 09h (MS-DOS print string)
    __asm("int 0x21");        // Call MS-DOS interrupt
}

char* getuserinput()
{
    char input[256];  // Static buffer to hold user input
    input[0] = 255;
    input[1] = 0;  // Initialize first two bytes to indicate empty string
    __asm("mov dx, %0" : : "r"(input));  // Load address of input buffer into DX
    __asm("mov ah, 0x0A");  // Set AH = 0Ah (MS-DOS buffered input)
    __asm("int 0x21");  // Call MS-DOS interrupt to read input

    int i = 2 + input[1];  // Calculate length of input string
    input[i] = '$';  // Null-terminate the string

    return input + 2;  // Return the input string
}

void clearscreen()
{
    __asm("mov ah, 0x06");   // Scroll up function
    __asm("mov al, 0");      // Number of lines to scroll (0 = clear entire window)
    __asm("mov bh, 0x07");   // Attribute for blank lines (light gray on black)
    __asm("mov cx, 0");      // Upper left corner (row=0, col=0)
    __asm("mov dx, 0x184F"); // Lower right corner (row=24, col=79)
    __asm("int 0x10");       // Call BIOS video interrupt
}
