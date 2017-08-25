#ifndef _FUNC_H_
#define _FUNC_H_

#include <ctype.h>

int htoi(const char s[],int start,int len)
{
    int i,j;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) //判断是否有前导0x或者0X
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    i+=start;
    j=0;
    for (; (s[i] >= '0' && s[i] <= '9')
         || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F'); ++i)
    {
        if(j>=len)
        {
            break;
        }
        if (_tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
        j++;
    }
    return n;
}

char *trim(char *str)
{
    char *p = str;
    char *p1;
    if(p)
    {
        p1 = p + strlen(str) - 1;
        while(*p && isspace(*p)) p++;
        while(p1 > p && isspace(*p1)) *p1-- = '/0';
    }
    return p;
}

char* join(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    if (result == NULL) exit (1);
    
    strcpy(result, s1);
    strcat(result, s2);
    
    return result;
}

#endif
