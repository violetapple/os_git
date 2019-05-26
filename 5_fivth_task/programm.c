#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h> 

#define MAXPROC 50 //сколько можем породить процессов
#define MAX_ARGUMENTS_COUNT 50 //сколько можем максимально передать аргументов в процессе

//В программе делаем условную нумерацию процессов в соответствии с их порядком в конфиг файле:
/*
/bin/sleep 14 respawn -> index = 0
/bin/sleep 15 wait -> index = 1
...
*/

//для создания файла процесса в папке tmp: /tmp/PID_FILENAME.pid_number 
char const *PID_FILENAME_EXTENSION = ".pid";
//например, для /bin/ls (если это первая программа в конфиге) будет создан файл /tmp/1.pid, в содержимом которого будет соответствующий PID
int FILENAME_LENGTH = MAXPROC + 5; // +5 из-за расширения.

//массив статусов дочерних процессов
//чтобы не обнулять массив объявляем его внешним
pid_t pid_list[MAXPROC]; //каждый элемент - число - PID запущенного процесса. Если по индексу лежит 0, то соответствующий индексу процесс не запущен.
//на случай непредвиденных обстоятельств храним число процессов
int pid_count=0; //число запущенных процессов


char *programmNames[MAXPROC]; //каждый элемент - строка - название исполняемого файла для соответствующего индексу процесса
char *programmData[MAXPROC][MAX_ARGUMENTS_COUNT + 1]; //каждый элемент по первому индексу - массив аргументов для процесса вида <аргумент 1><аргумент 2>; Конец массива должен быть NULL, поэтому +1
char *programmStatus[MAXPROC]; //каждый элемент - строка wait или respawn - действие для соответствующего индексу массива процесса
int processCount = 0; //сколько было считано процессов из файла

void createFile(char* file) {
	//file - это имя файла
	FILE *fp = fopen(file, "wa");
    fclose(fp);
}

void removeFile(char* fileName) {
	remove(fileName);
}

void createProcessFile(int pid) {

}

void splitLine(char *line) {
	//разбить прочтенную строку из файла на имя программы и аргументы и записать это все в массив programmData
	char *splitted;
	char *actualLine;
	int i = 0;
	//убираем из прочтенной линии символ переноса строки и затем саму линию разбиваем по пробелам на имя программы, ее аргументы и команду (exit/respawn)
	while((actualLine = strsep(&line, "\n"))) {
	    while ((splitted = strsep(&actualLine, " "))) {
	    	//splitted хранит имя программы, либо ее аргументы
	    	if(i == 0) {
	    		//для имен исполняемых файлов отдельный массив
	    		programmNames[processCount] = (char *)malloc(strlen(splitted));
	    		strcpy(programmNames[processCount], splitted);
	    		i += 1;
	    		continue;
	    	}
	    	programmData[processCount][i - 1] = (char *)malloc(strlen(splitted));
	    	strcpy(programmData[processCount][i - 1], splitted);
	    	i += 1;
	    }
	    programmStatus[processCount] = (char *)malloc(strlen(programmData[processCount][i - 2]));	  
	    strcpy(programmStatus[processCount], programmData[processCount][i - 2]); //programmStatus - только для действия с процессом (wait/respawn)
	    //free(programmData[processCount][i - 1]); //если использовать эту строку вместо нижней, то освобожденный адрес будет равен адресу programmData[processCount+1][0]. Хз почему
	    programmData[processCount][i - 2] = NULL; //programmData - массив только для имени программы и аргументов. NULL - обозначение конца массива
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

void createPidFile(int pid, int p) {
	//в pid находится номер стартовавшего процесса, в p - его порядковый номер
	//создаем файл вида /tmp/somefile.pid_number и пишем в него pid
	char *filename = (char *)malloc(FILENAME_LENGTH);
	sprintf(filename, "/tmp/%i%s", p, PID_FILENAME_EXTENSION);
	//TODO: создать файл и записать в него получивуюся инфу
}


void forkProcesses() {
	int i, p; 
	pid_t cpid;

	for (p = 0; p < processCount; p++)
	{
		if(programmNames[p] == NULL) continue; //в завершенных процессах ставим NULL вместо имени 
		//в p хранится номер процесса в порядке, соответстующем порядку исполняемых файлов в конфиге
		//TODO: MAXPROC сделать равным кол-ву считанных исполняемых файлов?
		cpid = fork(); 
		switch (cpid) 
		  	{ 
		  	case -1:
	    		syslog(LOG_WARNING, "%s\n", "fork failed, cpid = -1\n");
	    		break; 
		  	case 0:
		  		//ветка потомка
		  		//TODO: учесть, что может не запуститься
	    		cpid = getpid();         //global PID - PID потомка(значение, которое возвращается родителю после fork)
	            execvp(programmNames[p], programmData[p]); //передаем имя программы и начало массива аргументов для соответстующей программы
	            if (errno != 0) {
	                syslog(LOG_WARNING, "%s %s", "Error with execvp program: ", programmNames[p]);
	                exit(1);
	            }
	            syslog(LOG_NOTICE, "Child %d has been spawned with pid: %d\n", p, cpid);
	            exit(0);
		  	default:
		  		//ветка родителя
		  		//TODO: в /tmp/filename.pid записать PID стартовавшего потомка
		  		pid_list[p]=cpid;
		  		createPidFile(cpid, p);
		        pid_count++;
	    }
	}

	while (pid_count)
	{
		//TODO: смотреть на статус: wait/respawn и делать соответствующее действие
	    	cpid=waitpid(-1, NULL, 0);   //ждем любого завершенного потомка
	    	for (p=0; p<MAXPROC; p++)
		{
	        	if(pid_list[p]==cpid)
				{
				//делаем что-то по завершении дочернего процесса
	            	printf("Child number %d pid %d finished\n",p,cpid);
	            	pid_list[p]=0;
	            	pid_count--;
	        	}
		}
	}
}

void signalHandlerHUP() {
	//Поскольку после получения сигнала перезапускаем процессы, то НЕ надо выставлять в соответствующих массивах имен NULL, потому что процессы надо считать живыми
    for (int i = 0; i < processCount; i++) {
        if (pid_list[i] > 0 && programmNames[i] != NULL) {
            kill(pid_list[i], SIGKILL);
        }
    }
    forkProcesses();
}

int main(int argc, char **argv) {
	signal(SIGHUP, (void (*)(int))signalHandlerHUP);
	parseConfig("/tmp/someinfo");	//прочитать конфиг и спарсить из него название каждой программы и аргументы для нее в массивы
	//После успешного выполнения в programmData  аргументы для программы (индекс - порядковый номер программы),
	//в programmStatus - действие (wait/respawn) для процесса соответствующего индекса (индекс - порядковый номер программы)
	//в programmNames - имя для исполняемого файла для соответствующего индексу процесса (индекс - порядковый номер программы)

//debug
/*	int j = 0;
	for(int i = 0; i < processCount; i++) {
		printf("Name: %s  Arguments: ", programmNames[i]);
		while(programmData[i][j] != NULL) {
			printf("%s", programmData[i][j]);
			j += 1;
		}
		printf("[ STATUS: ]%s\n", programmStatus[i]);
		j = 0;
	}  */

	//создаем демона: //http://mainloop.ru/c-language/kak-sozdat-demona-v-linux.html
    int pid = fork();    
    switch (pid) {
        case 0:
            setsid();
            chdir("/");
            close((int) stdin);
            close((int) stdout);
            close((int) stderr);
            forkProcesses();
            break;
        case -1:
            printf("Fail. Unable to fork from main programm, cant start daemon. Exit\n");
            exit(1);
        default:
            break;
    }
    return 0;
}