#include <iostream>
#include <queue>
using namespace std;

struct Host;

struct Event {
    double event_time;
    double service_time; // TODO: might not need
    enum event_type { arrival, departure, backoff };
    event_type type;
    Event* next;
    Event* prev;
    Host* host; // TODO: needed? or just need the number?
};

struct Host {
    queue <Event*> buffer; // TODO: when do we use this?
    int ack; // to indicate if acknowledgment has been received
    int backoff;
};

// TODO: Do we need a global event list?

/* TODO: global variables for processing */
int NUM_HOSTS = 1; // variable
int T = 400; // variable. TODO: figure out how much this is?
double ARRIVAL_RATE = 0.01; // variable lambda
double SIFS = 0.00005;
double DIFS = 0.0001;
double SENSE = 0.00001;
double curr_time;
double service_rate; // mu. TODO: is this needed?

/* global variables for statistics */
double transmitted_bytes;
double total_time;
double total_delay; // TODO: figure out how to do this

/* TODO: output statistics */
double throughput;
double avg_network_delay;

void initialize() {

    /* Initialize processing variables */
    curr_time = 0.0;

    /* Initialize statistical variables */
    transmitted_bytes = 0.0;
    total_time = 0.0;
    total_delay = 0.0;
    throughput = 0.0;
    avg_network_delay = 0.0;

    /* Initialize data structures */
    // TODO: Initialize GEL if needed
    Host *hosts[NUM_HOSTS];

    for (int i = 0; i < NUM_HOSTS; ++i) {
        hosts[i] = new Host();
        hosts[i]->backoff = -1;
        // TODO: INIT ACK
    }


    
}

int main() {
    
    initialize();

}