#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static const int BUFFER_SIZE = 1024;

static const char HANDLER_ERROR[] = "error assigning section.key = value";
static const char MISSING_KEY[] = "a key is required";
static const char UNEXPECTED_EOL[] = "unexpected end of line reading config file";
static const char UNEXPECTED_CHARACTER[] = "unexpected character reading config file";


static const char *parseSection(char *section, char *buffer);

static const char *parseKeyValue(char *sectionKey, char *value, char *buffer, CONFIG_HANDLER_FUNC *handler);

int parseConfig(char *fileName, CONFIG_HANDLER_FUNC *handler) {
	FILE *fp = NULL;
	const char *errorString = NULL;
	int line = 1;
	int rc = 1;
	char *buffer = malloc(BUFFER_SIZE * 5);
	char *section = buffer + BUFFER_SIZE;
	char *sectionKey = section + BUFFER_SIZE;
	char *value = sectionKey + (BUFFER_SIZE * 2);
	if (buffer == NULL) {
		errorString = "unable to allocate memory to read configuration file";
		goto cleanup;
	}
	if ((fp = fopen(fileName, "r")) == NULL) {
		sprintf(buffer, "unable to open configuration file [%s]", fileName);
		errorString = buffer;
		goto cleanup;
	}
	while (fgets(buffer, BUFFER_SIZE, fp)) {
		char *s = buffer;
		while (isspace(*s)) {
			s++;
		}
		if (*s == '[') {
			if ((errorString = parseSection(section, s)) != NULL) {
				goto cleanup;
			}
		} else if (*s != ';' && *s != 0) {
			strncpy(sectionKey, section, BUFFER_SIZE);
			if ((errorString = parseKeyValue(sectionKey, value, s, handler)) != NULL) {
				goto cleanup;
			}
		}
		line++;
	}
	if (ferror(fp) != 0) {
		sprintf(buffer, "error reading configuration file: %i", errno);
		errorString = buffer;
		goto cleanup;
	}
	rc = 0;
cleanup:
	if (errorString != NULL) {
		fprintf(stderr, "%s at line %i\n", errorString, line);
	}
	if (buffer != NULL) {
		free(buffer);
	}
	if (fp != NULL) {
		fclose(fp);
	}
	return(rc);
}

static const char *parseSection(char *section, char *input) {
	int len = 0;
	char haveSection = 0;
	input++;
	while (*input != 0  && len < BUFFER_SIZE - 1) {
		if (*input == ']') {
			*section = 0;
			haveSection = 1;
		} else if (*input == ';') {
			if (haveSection == 0) {
				return(UNEXPECTED_EOL);
			} else {
				return(NULL);
			}
		} else if (!isspace(*input)) {
			if (haveSection == 0) {
				*section++ = tolower(*input);
				len++;
			} else {
				return(UNEXPECTED_CHARACTER);
			}
		}
		input++;
	}
	if (haveSection == 0) {
		return(UNEXPECTED_EOL);
	}
	return(NULL);
}

static const char *parseKeyValue(char *sectionKey, char *value, char *buffer, CONFIG_HANDLER_FUNC *handler) {
	const char ctlChars[] = "\a\b  \e\f       \n   \r \t \v    ";
	char *env = NULL;
	int keyLen = strlen(sectionKey);
	int valueLen = 0;
	char haveKey = 0;
	char esc = 0;
	if (keyLen > 0) {
		sectionKey[keyLen++] = '.';
	}
	while (*buffer != 0 && keyLen < (BUFFER_SIZE * 2) - 1) {
		if (!isspace(*buffer)) {
			if (*buffer == '=') {
				sectionKey[keyLen] = 0;
				buffer++;
				break;
			} else if (*buffer == ';') {
				return(UNEXPECTED_CHARACTER);
			} else {
				sectionKey[keyLen++] = tolower(*buffer);
				haveKey = 1;
			}
		}
		buffer++;
	}
	if (haveKey != 1) {
		return(MISSING_KEY);
	}
	while (*buffer != 0 && valueLen < BUFFER_SIZE - 1) {
		if (isspace(*buffer)) {
			if (valueLen > 0 && env == NULL && *buffer != '\n' && *buffer != '\r') {
				value[valueLen++] = *buffer;
			}
		} else {
			if (*buffer == '\\') {
				buffer++;
				esc = 1;
			} else if (*buffer == '$' && *(buffer + 1) == '{') {
				buffer += 2;
				env = buffer;
			} else if (*buffer == '}' && env != NULL) {
				*buffer = 0;
				char *es = getenv(env);
				if (es != NULL) {	
					strcpy((value + valueLen), es);
					valueLen = strlen(value);
					env = NULL;
				} else {
					*(value + valueLen) = 0;
				}
			} else if (*buffer == ';') {
				break;
			}
			if (env == NULL) {
				char ch = tolower(*buffer);
				if (esc == 1 && (ch == 'a' || ch  == 'b' || ch == 'e' || ch == 'f' || ch == 'n' || ch == 'r' || ch == 't' || ch == 'v')) {
					value[valueLen++] = ctlChars[tolower(*buffer) - 'a'];
				} else {
					value[valueLen++] = *buffer;
				}
			}
		}
		esc = 0;
		buffer++;
	}
	value[valueLen] = 0;
	if (handler(sectionKey, value)) {
		return(HANDLER_ERROR);
	}
	return(NULL);
}

