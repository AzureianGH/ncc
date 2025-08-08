int main() {
    int fact = factorial(5); // Should return 120
    __asm("mov eax, 333");
    return fact;
}

int factorial(int n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

[[naked]] void naked()
{

}