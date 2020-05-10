#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <aio.h>
#include <fcntl.h>
#include <signal.h>
#include <dlfcn.h>

sem_t sWriteSem, sReadSem, sMainSem;

char* buffer;
int length;
void *handle;

void *read_thread(void* thr_par){
	char fileName[3][50] = { "/home/alexei-coolhacker228/testfiles/1.txt", "/home/alexei-coolhacker228/testfiles/2.txt", "/home/alexei-coolhacker228/testfiles/3.txt" };
	mode_t mode = S_IRUSR;
	int flags = O_RDONLY | O_CREAT;
	int res = sem_init(&sReadSem, 0, 1);
	if (res != 0) {
 		perror("Semaphore initialization failed");
 		exit(EXIT_FAILURE);
 	}
	char* (*pFunc)(int, int);
	for(int i = 0; i < 3; i++) {
		sem_wait(&sReadSem);
		int fd = open(fileName[i], flags, mode);
		if (fd < 0){
			fprintf (stderr, "openclose: Cannot open file '%s'\n", fileName[i]);
			exit (1);
		}
		struct stat fi;
		stat(fileName[i], &fi);
		buffer = (char*)calloc(fi.st_size,sizeof(char));
		length = fi.st_size - 1;
		pFunc = dlsym(handle, "read_func");
		buffer = (*pFunc)(fd, fi.st_size);
		sem_post(&sWriteSem);
	} 
	pthread_exit(0);
}

void *write_thread(void* thr_par){
	char fileName[50] = "/home/alexei-coolhacker228/testfiles/result.txt";
	mode_t mode = S_IWUSR;
	int flags = O_WRONLY | O_CREAT, OFFSET = 0;
	int res = sem_init(&sWriteSem, 0, 0);
	if (res != 0) {
 		perror("Semaphore initialization failed");
 		exit(EXIT_FAILURE);
 	}
	void (*pFunc)(int, int, int, char*);
	for(int i = 0; i < 3; i++) {
		sem_wait(&sWriteSem);
		int fd = open(fileName, flags, mode);
		if (fd < 0){
			fprintf (stderr, "openclose: Cannot open file '%s'\n", fileName);
			exit (1);
		}
		pFunc = dlsym(handle, "write_func");
		(*pFunc)(fd, length, OFFSET, buffer);
		OFFSET += length;
		sem_post(&sReadSem);
	} 
	sem_post(&sMainSem);
	pthread_exit(0);
}

int main(void) {
	initscr();
	clear();
        noecho();
        refresh();
	int thr_par, res;
	pthread_t twrite, tread;
	res = sem_init(&sMainSem, 0, 0);
	if (res != 0) {
 		perror("Semaphore initialization failed");
 		exit(EXIT_FAILURE);
 	}
	handle = dlopen("/home/alexei-coolhacker228/LABORATORNIE/libDLL.so", RTLD_NOW);
	if (!handle) {
       		 fputs (dlerror(), stderr);
       		 exit(1);
    	}
	printf("\n RABOTAT \n");
	if((res = pthread_create(&tread, NULL, read_thread, &thr_par)) < 0)
            perror("pthread_create");                
	if((res = pthread_create(&twrite, NULL, write_thread, &thr_par)) < 0)
            perror("pthread_create");  
	sem_wait(&sMainSem);  
	dlclose(handle);     
        clear();
	refresh();
        endwin();
        return 0;
}
