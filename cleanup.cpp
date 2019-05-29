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

int generate_frame_len() { // negative exponentially distributed random variable in range 0 < r <= 1544
    double len = neg_exp_time(1) * MAX_FRAME; // multiplied by MAX_FRAME to scale 
    if (len > MAX_FRAME) {
        return MAX_FRAME;
    }
    return round(len); // in bytes
}

double generate_transmission_time(double len) {
    return (len * 8) / CHANNEL_CAP;
}

int main() {
    cout << T << ARRIVAL_RATE << MAX_FRAME << ACK_FRAME << CHANNEL_CAP << SIFS << DIFS << SENSE << endl;
    double fr_len = generate_frame_len();
    cout << generate_transmission_time(fr_len) << endl;
}