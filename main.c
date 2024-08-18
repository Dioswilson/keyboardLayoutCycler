#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "pthread.h"
#include <sys/inotify.h>

#include "charBuffer.h"
#include "keyboardLayoutCycle.h"

char configFile[81] = {};

char** keyboardDists;
int keyboardDistsSize = INIT_DIST_QUANT;
int keyboardDistsQuant = 0;
int currentDist = 0;

bool runProgram = true;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
	int status = 0;
	pthread_t listenerThread;

	keyboardDists = malloc(sizeof(char*) * INIT_DIST_QUANT);
	for (int i = 0; i < INIT_DIST_QUANT; i++) {
		keyboardDists[i] = malloc(sizeof(char) * KEYBOARD_DIST_MAX_LENGTH);
	}

	status = loadConfig("config.toml");

	if (status == EXIT_FAILURE) {
		freeKeyboardDists();
		printf("Error initializing configuration... exiting program\n");
		return EXIT_FAILURE;
	}

	initConfig();


	pthread_create(&listenerThread, NULL, fileChangeListener, NULL);


	signal(34, changeLayoutHandler); //TODO: Maybe configure signal
	signal(SIGINT, closeHandler);
	signal(SIGTERM, closeHandler);


	while (runProgram) {
		pause();
	}

	pthread_cancel(listenerThread);

	freeKeyboardDists();
	return 0;
}


int loadConfig(const char* configFileName) {
	CharBuffer_t readBuffer = {};
	TomlStatus_t tomlStatus = TOML_SUCCESS;
	char* bufferCopy = NULL;

	int fd = getConfigFileFd(configFileName);

	if (fd == -1) {
		return EXIT_FAILURE;
	}

	initBuffer(&readBuffer);
	for (int i = 0; i < keyboardDistsQuant; i++) {
		memset(keyboardDists[i], 0, KEYBOARD_DIST_MAX_LENGTH);
	}
	keyboardDistsQuant = 0;

	while (tomlStatus == TOML_SUCCESS) {
		tomlStatus = readNextTomlElement(fd, &readBuffer);
		if (tomlStatus != TOML_ERROR) {
			if (strchr(readBuffer.content, '=') == NULL) {
				continue;
			}
			//printf("Contet: %s\n", readBuffer.content);

			bufferCopy = (char*) malloc(strlen(readBuffer.content));
			removeAllOfChar(readBuffer.content, bufferCopy, ' ');

			if (strncmp(bufferCopy, "layout_list=", 12) == 0) {
				// printf("Layout list: %s\n", bufferCopy);
				//Get the [....] array
				char* arrayStr = strtok(bufferCopy, "=");
				arrayStr = strtok(NULL, "=");

				if (*arrayStr == '[' && arrayStr[strlen(arrayStr) - 1] == ']') {

					strtok(arrayStr, "\"");

					while ((arrayStr = strtok(NULL, "\"")) != NULL) {
						if (keyboardDistsQuant >= keyboardDistsSize - 2) {
							keyboardDistsSize *= 2;
							keyboardDists = realloc(keyboardDists, sizeof(char*) * keyboardDistsSize);
							for (int i = keyboardDistsQuant; i < keyboardDistsSize; i++) {
								keyboardDists[i] = malloc(sizeof(char) * KEYBOARD_DIST_MAX_LENGTH);
							}
						}

						//TODO: Maybe check if it is a valid layout
						strcpy(keyboardDists[keyboardDistsQuant], arrayStr);
						keyboardDistsQuant++;
						strtok(NULL, "\"");
					}
				}
			}
			free(bufferCopy);
		}
	}

	close(fd);

	if (tomlStatus == TOML_ERROR || keyboardDistsQuant == 0) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


/**
 * @desc Gets configuration file at $HOME/.config/keyboardLayoutCycle with configFileName name
 * @return Returns -1 if error
 * */
int getConfigFileFd(const char* configFileName) {
	int status = 0;
	int fd = 0;
	off_t fileSize = 0;
	char configDir[70] = {};
	char defaultConf[sizeof(DEFAULT_CONFIG) + KEYBOARD_DIST_MAX_LENGTH] = {};
	char currentLayout[KEYBOARD_DIST_MAX_LENGTH] = {};

	const char* homeDir = getenv("HOME");
	if (homeDir == NULL) {
		fprintf(stderr, "Unable to determine the home directory.\n");
		return -1;
	}

	// Note: Might not work if name is too long
	snprintf(configDir, sizeof(configDir), "%s/.config/keyboardLayoutCycle", homeDir);
	snprintf(configFile, sizeof(configFile), "%s/%s", configDir, configFileName);

	struct stat st = {0};

	if (stat(configDir, &st) == -1) { // If directory does not exist
		status = mkdir(configDir, 0777);

		if (status == -1) {
			perror("Could not create configuration directory, ~/.config might be missing");
			return -1;
		}
	}

	fd = open(configFile, O_CREAT | O_RDWR, 0666);

	if (fd == -1) {
		perror("Could not load configuration");
		return -1;
	}

	fileSize = lseek(fd, 0, SEEK_END); // Igore -1

	if (fileSize == 0) {
		getActualLayout(currentLayout);
		snprintf(defaultConf, sizeof(defaultConf), DEFAULT_CONFIG, currentLayout);

		write(fd, defaultConf, strlen(defaultConf));
	}

	//Back to begining
	lseek(fd, 0, SEEK_SET);

	return fd;
}

TomlStatus_t readNextTomlElement(int fd, CharBuffer_t* readBuffer) {
	TomlStatus_t status = TOML_SUCCESS;
	int buffPos = 0;
	bool doRead = true;
	size_t bytesRead = 0;
	bool foundArray = false;
	char currChar = -1;

	memset(readBuffer->content, 0, readBuffer->size);

	while (doRead) { // Note: Throw error for
		if (buffPos >= readBuffer->size - 2) {
			resizeBuffer(readBuffer);
		}

		bytesRead = read(fd, readBuffer->content + buffPos, 1);
		if (bytesRead == -1) {
			perror("Could not read configuration file");
			return TOML_ERROR;
		}
		currChar = readBuffer->content[buffPos];

		buffPos++;


		if (bytesRead == 0) {
			doRead = false;
			status = TOML_END;
			buffPos--;
			if (foundArray) {
				status = TOML_ERROR;
			}
		}

		if (currChar == '[') {
			foundArray = true;
		}

		if (foundArray) {
			if (currChar == ']') {
				doRead = false;
				foundArray = false;
				status = TOML_SUCCESS;
			}
			else if (currChar == '\n') {
				buffPos--;
			}
		}
		else {
			if (currChar == '\n') {
				doRead = false;
				status = TOML_SUCCESS;
				buffPos--;
			}
		}
	}

	readBuffer->content[buffPos] = '\0';


	return status;
}


void initConfig() {
	char layout[KEYBOARD_DIST_MAX_LENGTH] = {};
	bool layoutFound = false;

	getActualLayout(layout);

	for (int i = 0; i < keyboardDistsQuant; i++) {
		if (strcmp(keyboardDists[i], layout) == 0) {
			currentDist = i;
			layoutFound = true;
			break;
		}
	}

	if (!layoutFound) {
		setKeyboardLayout(keyboardDists[0]);
		currentDist = 0;
	}
}

void removeAllOfChar(char* src, char* dest, char c) {
	strcpy(dest, src);
	//TODO: Maybe not do copy, just reomve from OG array

	while (*src) {
		if (*src != c) {
			*dest = *src;
			dest++;
		}
		src++;
	}
	*dest = '\0';
}

void freeKeyboardDists() {
	for (int i = 0; i < keyboardDistsSize; i++) {
		free(keyboardDists[i]);
	}
	free(keyboardDists);
}

void changeLayoutHandler(int sig) {
	pthread_mutex_lock(&mutex);
	currentDist++;
	currentDist = currentDist % keyboardDistsQuant;

	setKeyboardLayout(keyboardDists[currentDist]);

	pthread_mutex_unlock(&mutex);
}

void closeHandler(int sig) {
	runProgram = false;
}

void setKeyboardLayout(char* layout) {
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork failed");
	}
	else if (pid == 0) {// Child process
		char* cmdArgs[] = {"/bin/setxkbmap", layout, NULL};
		execvp(cmdArgs[0], cmdArgs);
		perror("Layout change failed");//If execvp returns, it failed
	}
}

void getActualLayout(char* layout) {
	char buffer[KEYBOARD_DIST_MAX_LENGTH] = {};
	FILE* fp;

	fp = popen("/bin/setxkbmap -query", "r");
	/*Get the value into a variable*/
	fgets(buffer, KEYBOARD_DIST_MAX_LENGTH, fp);
	fgets(buffer, KEYBOARD_DIST_MAX_LENGTH, fp);
	fgets(buffer, KEYBOARD_DIST_MAX_LENGTH, fp);
	sscanf(buffer, "layout:%s", layout);//Warn: Possible overflow
	pclose(fp);
}


void* fileChangeListener(void*) {
	char eventBuff[INOTIFY_BUFF_SIZE] = {};

	while (runProgram) { //Most likely will be cancelled by main thread
		int fd = inotify_init();
		if (fd == -1) {
			perror("Could not initialize config listener");
			return NULL;
		}

		int wd = inotify_add_watch(fd, configFile, IN_MODIFY);
		read(fd, eventBuff, INOTIFY_BUFF_SIZE);

		pthread_mutex_lock(&mutex);

		loadConfig("config.toml");
		initConfig();
		//printf("Config changed\n");
		fflush(stdout);

		pthread_mutex_unlock(&mutex);

		close(fd);
	}

	return NULL;
}

