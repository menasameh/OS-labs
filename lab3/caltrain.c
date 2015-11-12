#include <pthread.h>
#include "caltrain.h"



void
station_init(struct station *station)
{
    station->trainSeats=0;
    station->onBoard=0;
    station->readyForTrain=0;
    station->waitForTrain=0;

    pthread_mutex_init(&station->lock, NULL);

    pthread_cond_init(&station->passArrive , NULL);
    pthread_cond_init(&station->passBoarded, NULL);
    pthread_cond_init(&station->trainArrive, NULL);
}

void
station_load_train(struct station *station, int count)
{
    pthread_mutex_lock(&station->lock);
    station->trainSeats=count;
    pthread_cond_broadcast(&station->trainArrive);
    while(station->waitForTrain >0 && station->trainSeats-station->onBoard >0){
        pthread_cond_wait(&station->passBoarded, &station->lock);
        print(station,1);
    }
    station->trainSeats = 0;
    station->readyForTrain=0;
    station->onBoard = 0;
    pthread_mutex_unlock(&station->lock);
}

void
station_wait_for_train(struct station *station)
{
    pthread_mutex_lock(&station->lock);
	station->waitForTrain++;
    print(station,2);
    while(station->trainSeats - station->onBoard - station->readyForTrain== 0 ){
        pthread_cond_wait(&station->trainArrive, &station->lock);
    }
    station->readyForTrain++;
    pthread_mutex_unlock(&station->lock);
}

void
station_on_board(struct station *station)
{
    print(station,3);
    pthread_mutex_lock(&station->lock);
    station->waitForTrain--;
    station->readyForTrain--;
	station->onBoard++;
    pthread_cond_signal(&station->passBoarded);
    pthread_mutex_unlock(&station->lock);
}


void print(struct station *station, int i){
    printf("%s : trainSeats : %d  waiting : %d   ready : %d   boarded : %d\n",i==1?" train Enter       ":i==2?" passenger Enter   ":" passenger on board",
    station->trainSeats, station->waitForTrain, station->readyForTrain, station->onBoard);
}
