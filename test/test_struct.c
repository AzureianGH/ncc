struct test_struct {
    int a;
    int b;
    char c;
};

void function()
{
    struct test_struct my_struct;
    my_struct.a = 5;
    my_struct.b = 10;
    my_struct.c = 'z';
}

struct test_struct *getTestStruct()
{
    static struct test_struct my_struct;
    my_struct.a = 5;
    my_struct.b = 10;
    my_struct.c = 'z';
    return &my_struct;
}

void doSomethingWithStruct(struct test_struct *s)
{
    s->a += 1;
    s->b += 2;
    s->c = 'y';
}