#include "string.h"

int
count_l(const char *path)
{
    int c=0;
    if (strlen(path)==1) return c;
    for(int i=0; path[i]; i++) {
        if (path[i]=='/') c++;
    }
    return c;
}

char* parent_path(const char *path, int l)
{
    char *pp = malloc(PATH_MAX);
    memset(pp, '\0', PATH_MAX);
    const char delimiter[] = "/";
    
    char *token = strtok((char*)path, delimiter);
    
    for (int i=0; i < l; i++)
    {
        snprintf(pp, sizeof(pp), "%s/%s", pp, token);
    }
    
    return pp;
}
