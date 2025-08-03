int global_variable = 42;


void test_explicit()
{
    char* str = "Hello, World!";

    unsigned int ptr_location = (unsigned int)str;

    unsigned char byte_value = *((unsigned char*)ptr_location);

    unsigned int value = *((unsigned int*)ptr_location);
}