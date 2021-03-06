#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int SECONDS_TO_WAIT = 60; //сколько секунд процесс должен ждать снятия блокировки. По истечении этого времени программа завершится.
int bytesToRead = 1024;
char *lockFileExtension = ".lck";
char *lockFileFolder = "/tmp/";
char localFileName[255];
char *fullLockFileName;
char *operation;

void* safeMalloc(int bytes) {
	void* address = malloc(bytes);
	if(address != NULL) {
		return address;
	} else {
		printf("Error with allocating memory");
		exit(1);
	}
}

char* getAbsoluteFileName(char* path, char* file) {

	char* absoluteFileName = safeMalloc(strlen(path) + strlen(file));
	strcpy(absoluteFileName, path);
	strcpy(absoluteFileName+strlen(path), file);
	return absoluteFileName;
}

char* getLocalFileName(char* absoluteFileName) {
	//получаем имя файла без пути к нему. Например: /some/path/to/fileName -> fileName

	int localFileNameLength = 0;


	for(int i = strlen(absoluteFileName) - 1; i >= 0; i--) {
		if(absoluteFileName[i] == '/') {
			//strncpy(localFileName, absoluteFileName + strlen(absoluteFileName) - localFileNameLength + 1, localFileNameLength);
			strncpy(localFileName, absoluteFileName + strlen(absoluteFileName) - localFileNameLength, localFileNameLength);
			localFileName[localFileNameLength] = '\0'; //функция strncpy НЕ копирует нулевой символ, его надо добавлять вручную
			return localFileName;
		}
		localFileNameLength++;
	}
	return absoluteFileName;
}

void createFile(char* file) {
	FILE *fp = fopen(file, "wa");
    fclose(fp);
}

void removeFile(char* fileName) {
	remove(fileName);
}

void acquireLock(char *lockFile) {
	createFile(fullLockFileName);
	FILE *fp = fopen(fullLockFileName, "wa");
	if(fp == NULL) {
		printf ("Cannot open file %s.\n", fullLockFileName);
		exit(1);
	}
	//пишем в файл блокировки PID процесса и название операции (read/write)
    fprintf(fp, "%d %s\n", getpid(), operation);
    fclose(fp);
}

int isFileExists(char *absoluteFileName) {
	// R_OK, W_OK, X_OK - проверить права доступа
	if( access( absoluteFileName, F_OK ) != -1 ) {
	    return 1;
	} else {
	    return 0;
	}
}

char* getLockFileName() {
	char* newFile = safeMalloc(strlen(localFileName) + strlen(lockFileExtension) + 1);
	strcpy(newFile, localFileName);
	strcpy(newFile+strlen(localFileName), lockFileExtension);
	return newFile;
}

void waitForLock() {
	int seconds;
	printf("Waiting for file %s\n", fullLockFileName);
	while(1) {
		sleep(1); //чтобы не каждое мгновение запрашивать ресурс
		seconds += 1;
		if(seconds > SECONDS_TO_WAIT) {
			printf("File lock exists too long\n");
			exit(1);
		}
		if(!isFileExists(fullLockFileName)) {
			break;
		}
	}
}

void createLockFile() {
	if(isFileExists(fullLockFileName)) {
		waitForLock();
	}
	acquireLock(fullLockFileName);
}

void makeOperation(char* fileName, int argumentsToWrite, char* data[]) {
	// метод производит операцию над указанным файлом (записать или прочитать). argumentsToWrite - количество переданных на запись аргументов

	sleep(2); //чтобы проверить работоспособность блокировки
	if(operation[0] == 'w') {
		if(argumentsToWrite > 0) {
			printf("Writing to file\n");
			char* dataToWrite;
			FILE *fp = fopen(fileName, "a");
			if(fp == NULL) {
				printf ("Cannot open file %s\n", fileName);
				exit(1);
			} 
			for(int i = 0; i < argumentsToWrite; i++) {
				dataToWrite = data[3+i];
				fprintf(fp, "%s\n", dataToWrite);
			}
			fclose(fp);
		}
	} else if(operation[0] == 'r') {
		printf("Reading from file\n");
		//для простоты читаем первую строку с ограничением в bytesToRead байт(или пока не встретим EOF).
		FILE *fp = fopen(fileName, "r");
		if(fp == NULL) {
			printf ("Cannot open file %s.\n", fileName);
			exit(1);
		}
		char buffer[bytesToRead];
		fgets(buffer, bytesToRead, fp);
		printf("File has been read. First line:\n%s\n", buffer);
	} else {
		printf("Cannot recognize operation. Possible operations: read, write\n");
	}
	printf("removing file %s\n", fullLockFileName);
	removeFile(fullLockFileName);
}

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("[-] Please specify data in following format: programm_name absoluteFileName operationName(read or write) [data to write]\n");
	} else {
		char* absoluteFileName = argv[1];
		operation = argv[2];
		getLocalFileName(absoluteFileName);
		char* lockFile = getLockFileName();
		fullLockFileName = getAbsoluteFileName(lockFileFolder, lockFile);
		//free(lockFile);
		createLockFile();
		makeOperation(absoluteFileName, argc-3, argv);
	}
}