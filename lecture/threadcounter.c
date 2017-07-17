#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

const size_t countval = 50000;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main(){
	pthread_t thread1, thread2;
	int crth1,crth2;

	void func1();
	void func2();

	crth1 = pthread_create(&thread1,NULL,(void *)func1,NULL);
	crth2 = pthread_create(&thread2,NULL,(void *)func2,NULL);

	//スレッド1作成失敗
	if (crth1 != 0){
		err(EXIT_FAILURE, "can not create thread 1 %s", strerror(crth1));
	}
	//スレッド2作成失敗
	if (crth2 != 0){
		err(EXIT_FAILURE, "can not create thread 2 %s", strerror(crth2));
	}

	crth1 = pthread_join(thread1,NULL);
	if (crth1 != 0){
		errc(EXIT_FAILURE, crth1, "can not join thread 1");
	}
	crth2 = pthread_join(thread2,NULL);
	if (crth2 != 0){
		errc(EXIT_FAILURE, crth2, "can not join thread 2");
	}

	printf("%d\n",counter);

	pthread_mutex_destroy(&m);
	return 0;

}

void func1(){
	size_t i;

	for(i=0;i<countval; i++){
		int mutex;
		mutex = pthread_mutex_lock(&m);
		//ロック失敗
		if (mutex != 0){
			errc(EXIT_FAILURE,mutex,"can not lock");
		}
		counter++;

		mutex = pthread_mutex_unlock(&m);
		//アンロック失敗
		if (mutex != 0){
			errc(EXIT_FAILURE,mutex,"can not unlock");
		}
	}
}

void func2(){
	size_t i;

	for(i=0;i<countval; i++){
		int mutex;
		mutex = pthread_mutex_lock(&m);
		//ロック失敗
		if (mutex != 0){
			errc(EXIT_FAILURE,mutex,"can not lock");
		}
		counter++;

		mutex = pthread_mutex_unlock(&m);
		//アンロック失敗
		if (mutex != 0){
			errc(EXIT_FAILURE,mutex,"can not unlock");
		}
	}
}