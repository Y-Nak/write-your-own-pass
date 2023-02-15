int test1(bool b)
{
    int i = 1;
    if (b)
        i = 2;
    else
        i = 3;

    return i;
}

int test2(bool b)
{
    int c = 10;
    for (int i = 0; i < 10; ++i)
    {
        c += i;

        if (b)
            c += 100;
        else
        {
            ++i;
            continue;
        }

        c += 1;
    }
    return c;
}