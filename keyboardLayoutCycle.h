#include "charBuffer.h"
#include <sys/inotify.h>

#define KEYBOARD_DIST_MAX_LENGTH 30
#define INIT_DIST_QUANT 10

#define DEFAULT_CONFIG "layout_list = [ \"%s\" ]"

#define INOTIFY_BUFF_SIZE 2* (sizeof(struct inotify_event) + 1)

typedef enum {
		TOML_SUCCESS,
		TOML_END,
		TOML_ERROR,
} TomlStatus_t;

int loadConfig(const char* configFileName);

int getConfigFileFd(const char* configFileName);

TomlStatus_t readNextTomlElement(int fd, CharBuffer_t* readBuffer);

void initConfig();

void removeAllOfChar(char* src, char* dest, char c);

void freeKeyboardDists();

void closeHandler(int sig);

void changeLayoutHandler(int sig);

void setKeyboardLayout(char* layout);

void getActualLayout(char* layout);

void* fileChangeListener(void*);
