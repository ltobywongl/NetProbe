#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

char *generateMessage(int length, int sequence)
{
    if (length <= 0)
    {
        cout << "Error: generate message length <= 0" << endl;
    }
    char *message = (char *)malloc(length * sizeof(char));
    if (sequence == -1)
    {
        int index = 0;
        for (int i = 0; i < length; i++)
        {
            message[i] = '0' + index;
            index = index + 1;
            if (index == 10)
            {
                index = 0;
            }
        }
        message[length - 1] = '\0';
        return message;
    }
    else
    {
        sprintf(message, "%d", sequence);
        int flag = 0;
        for (int i = 0; i < length; i++)
        {
            if (message[i] == '\0')
            {
                flag = 1;
                message[i] = '#';
                continue;
            }
            if (flag == 1)
            {
                message[i] = '0';
            }
        }
        message[length - 1] = '\0';
        return message;
    }
}

long getSequence(char *message)
{
    char *p;
    char sequence[16];
    strncpy(sequence, message, 15);
    sequence[15] = '\0';
    for (int i = 0; i < 15; i++)
    {
        if (sequence[i] == '#')
        {
            sequence[i] = '\0';
            break;
        }
    }
    return strtol(sequence, &p, 10);
}