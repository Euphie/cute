#ifndef _INT_LIB_H_
#define _INT_LIB_H_
int _tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

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
            n = 16 * n + (10 + _tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (_tolower(s[i]) - '0');
        }
        j++;
    }
    return n;
}

typedef struct {
    char **str; // the PChar of string array
    size_t num; // the number of string
} IString;

int Split(char *src, char *delim, IString *istr) // split buf
{
    int i;
    char *str = NULL, *p = NULL;
    
    (*istr).num = 1;
    str = (char *)calloc(strlen(src) + 1, sizeof(char));
    if (str == NULL)
        return 0;
    (*istr).str = (char **)calloc(1, sizeof(char *));
    if ((*istr).str == NULL)
        return 0;
    strcpy(str, src);
    
    p = strtok(str, delim);
    (*istr).str[0] = (char *)calloc(strlen(p) + 1, sizeof(char));
    if ((*istr).str[0] == NULL)
        return 0;
    strcpy((*istr).str[0], p);
    for (i = 1; p = strtok(NULL, delim); i++) {
        (*istr).num++;
        (*istr).str = (char **)realloc((*istr).str, (i + 1) * sizeof(char *));
        if ((*istr).str == NULL)
            return 0;
        (*istr).str[i] = (char *)calloc(strlen(p) + 1, sizeof(char));
        if ((*istr).str[0] == NULL)
            return 0;
        strcpy((*istr).str[i], p);
    }
    free(str);
    str = p = NULL;
    
    return 1;
}

#endif
