#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXPROC 50 //сколько можем породить процессов
#define MAX_ARGUMENTS_COUNT 50 //сколько можем максимально передать аргументов в процессе

//Делаем условную нумерацию процессов в соответствии с их порядком в конфиг файле
char *programmNames[MAXPROC][MAX_ARGUMENTS_COUNT + 1]; //каждый элемент - массив вида <программа><аргумент 1><аргумент 2>; +1 из-за имени программы
char *programmStatus[MAXPROC]; //каждый элемент - строка wait или respawn - действие для соответствующего индексу массива процесса
int processCount = 0; //сколько было считано процессов из файла

void createFile(char* file) {
	FILE *fp = fopen(file, "wa");
    fclose(fp);
}

void removeFile(char* fileName) {
	remove(fileName);
}

void splitLine(char *line) {
	//разбить прочтенную строку из файла на имя программы и аргументы и записать это все в массив programmNames
	char *splitted;
	char *actualLine;
	int i = 0;
	//убираем из прочтенной линии символ переноса строки и затем саму линию разбиваем по пробелам на имя программы, ее аргументы и команду (exit/respawn)
	while((actualLine = strsep(&line, "\n"))) {
	    while ((splitted = strsep(&actualLine, " "))) {
	    	//splitted хранит имя программы, либо ее аргументы
	    	programmNames[processCount][i] = (char *)malloc(strlen(splitted));
	    	strcpy(programmNames[processCount][i], splitted);
	    	i += 1;
	    }
	    programmStatus[processCount] = (char *)malloc(strlen(programmNames[processCount][i - 1]));	  
	    strcpy(programmStatus[processCount], programmNames[processCount][i - 1]); //programmStatus - только для действия с процессом (wait/respawn)
	    //free(programmNames[processCount][i - 1]); //если использовать эту строку вместо нижней, то освобожденный адрес будет равен адресу programmNames[processCount+1][0]. Хз почему
	    programmNames[processCount][i - 1] = NULL; //programmNames - массив только для имени программы и аргументов. NULL - разделитель
	    processCount += 1;
		break;	 
	}
}

void parseConfig(char *fileName) {
	//Читать построчно файл https://stackoverflow.com/questions/3501338/c-read-file-line-by-line
    FILE *fp;
    char *token = NULL;	
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;
    fp = fopen(fileName, "r");
    if (fp == NULL) {
    	printf("[-]Error with opening file. Exit from program\n");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
    	splitLine(line);
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);
    }
    fclose(fp);
    if (line)
        free(line);
    //exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	parseConfig("/tmp/someinfo");	//прочитать конфиг и спарсить из него название каждой программы и аргументы для нее в массивы
	//После успешного выполнения в programmNames программа(0-й элемент) с аргументами для нее (индекс - порядковый номер программы),
	//а в programmStatus - действие (wait/respawn) для программы соответствующего индекса
/*	int j = 0;
	for(int i = 0; i < processCount; i++) {
		while(programmNames[i][j] != NULL) {
			printf("%s", programmNames[i][j]);
			j += 1;
		}
		printf("[ STATUS: ]%s\n", programmStatus[i]);
		j = 0;
	} */

}