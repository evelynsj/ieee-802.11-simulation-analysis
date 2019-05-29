#include <iostream>
#include <cmath>
using namespace std;

struct Frame {
    double r; // length of frame
    double transmission_time; // transmission time of frame
    bool is_ack;
};

struct Host {
    // TODO: queue<Event*> buffer - figure out what this is for :(
    int backoff;
};

struct Event {
    double event_time;
    enum event_type { arrival, departure, backoff };
    event_type type;
    int src; 
    int dest;
    Event* next;
    Event* prev;
    Frame* fr;
};

/* Global variables for processing */
double current_time;
bool channel_idle;

/* Constant variables for processing */
const int NUM_HOSTS = 2;
const int T = 5;
const double ARRIVAL_RATE = 0.01; // lambda
const double MAX_FRAME = 1544;
const double ACK_FRAME = 64;
const double CHANNEL_CAP = 10000000; // 1 Mbps = 10^6 bits/sec 
const double SIFS = 0.00005;
const double DIFS = 0.0001;
const double SENSE = 0.00001;

/* Global variables for statistics */
double transmitted_bytes;
double total_time;
double total_delay;

/* Output statistics */
double throughput;
double avg_network_delay;

/* Data structures */
Host* hosts[NUM_HOSTS];
Event* GELhead;
Event* GELtail;
int GELsize;

/* Functions */
double neg_exp_time(double rate) {
    double u;
    u = drand48();
    return ((-1/rate)*log(1-u));
}

int main() {
}