#ifndef CONFIG_H
#define CONFIG_H

typedef int (CONFIG_HANDLER_FUNC)(char *key, char *value);

extern int parseConfig(char *fileName, CONFIG_HANDLER_FUNC *handler);


#endif
