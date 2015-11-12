#include <pthread.h>

struct station {

    pthread_mutex_t lock;

    pthread_cond_t trainArrive;
    pthread_cond_t passArrive;
    pthread_cond_t passBoarded;

    int waitForTrain;
    int readyForTrain;
    int onBoard;
    int trainSeats;

};

void station_init(struct station *station);

void station_load_train(struct station *station, int count);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);

