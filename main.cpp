#include <iostream>
#include <queue>
#include <cmath>
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

// TODO: might not need
Event* GELhead;
Event* GELtail;
int GELsize;

/* TODO: global variables for processing */
int NUM_HOSTS = 1; // variable
int T = 400; // variable. TODO: figure out how much this is?
double ARRIVAL_RATE = 0.01; // variable lambda
double SIFS = 0.00005;
double DIFS = 0.0001;
double SENSE = 0.00001;
double current_time;
double service_rate; // mu. TODO: is this needed?

/* global variables for statistics */
double transmitted_bytes;
double total_time;
double total_delay; // TODO: figure out how to do this

/* TODO: output statistics */
double throughput;
double avg_network_delay;

double neg_exp_time(double rate) {
    double u;
    u = drand48();
    return ((-1/rate)*log(1-u));
}

void create_arrival(double ev_time, Host* host) {
    cout << "Create arrival" << endl;
    Event *ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::arrival;
    ev->host = host;
    // TODO: NEED SERVICE TIME??
    // Event* next; // do this at insert
    // Event* prev; // do this at insert
}

void initialize() {

    /* Initialize processing variables */
    current_time = 0.0;

    /* Initialize statistical variables */
    transmitted_bytes = 0.0;
    total_time = 0.0;
    total_delay = 0.0;
    throughput = 0.0;
    avg_network_delay = 0.0;

    /* Initialize data structures */
    // TODO: Might not need GEL
    GELhead = nullptr;
    GELtail = nullptr;
    GELsize = 0;
    Host *hosts[NUM_HOSTS];

    for (int i = 0; i < NUM_HOSTS; ++i) {
        hosts[i] = new Host;
        hosts[i]->backoff = -1;
        // TODO: INIT ACK
        double first_arrival_time = neg_exp_time(ARRIVAL_RATE) + current_time;
        create_arrival(first_arrival_time, hosts[i]);
    }


    
}

int main() {
    
    initialize();

}