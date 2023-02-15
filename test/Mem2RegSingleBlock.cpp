int singleBlock()
{
    int sum = 0;
    int i = 10;
    sum += i;
    i = 20;
    sum += i;
    i = 30;
    sum += i;

    return sum;
}

void singleBlock2()
{
    int i;
    int sum = 0;
    while (true)
    {
        sum += i;
        i = 1;
        if (sum > 10)
        {
            break;
        }
    }
    return;
}