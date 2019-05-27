#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>
#include <random>
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
int T = 5; // variable. TODO: figure out how much this is?
double ARRIVAL_RATE = 0.01; // variable lambda
double SIFS = 0.00005;
double DIFS = 0.0001;
double SENSE = 0.00001;
double current_time;
double service_rate; // mu. TODO: is this needed?
bool channel_idle;

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

double generate_backoff() {
    static default_random_engine generator;
    uniform_real_distribution<double> distribution(1,T);

    // generate uniform random variable between (0,1) and multiply by T
    double dist = distribution(generator) * T;

    return round(dist);
}

void iterate() {
    cout << "***********" << endl;
    cout << "start iterating" << endl;
    cout << "GELsize is " << GELsize << endl;
    Event *curr = GELhead;
    while (curr != nullptr) {
        if (curr->type == Event::arrival) {
            cout << "Arrival ";
        } else if (curr->type == Event::departure) {
            cout << "Departure ";
        } else {
            cout << "Backoff ";
        }
        cout << curr->event_time << endl;
        curr = curr->next;
    }
    cout << "done iterating" << endl;
    cout << "************" << endl;
}

void delete_head() {
    GELhead = GELhead->next;
    GELsize--;
}

void insert(Event* event) { // insert to GEL
    // if head is nullptr
    if (GELhead == nullptr) {
        GELhead = event;
        GELhead->next = nullptr;
        GELhead->prev = nullptr; 
    }
    else {
        if (event->event_time < GELhead->event_time) { // insert in front of head
            event->next = GELhead;
            event->prev = nullptr;
            GELhead->prev = event;
            GELhead = event;
        }
        else {
            Event *curr = GELhead;
            Event *prev = nullptr;
            while (curr) {
                if (event->event_time < curr->event_time) { // Insert in the middle
                    prev = curr->prev;
                    prev->next = event;
                    event->prev = prev;
                    event->next = curr;
                    curr->prev = event;
                    break;
                }
                if (curr->next == nullptr && event->event_time > curr->event_time) { // Insert at the end
                    curr->next = event;
                    event->prev = curr;
                    event->next = nullptr;
                    break;
                }
                curr = curr->next;
            }
        }
    }
    GELsize++;
}

void create_arrival(double ev_time, Host* host) {
    // cout << "Create arrival" << endl;
    Event *ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::arrival;
    ev->host = host;
    // TODO: NEED SERVICE TIME??
    insert(ev);
}

void create_backoff(double ev_time, Host* host, int backoff) {
    // cout << "Create backoff" << endl;
    Event *ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::backoff;
    ev->host = host;
    // TODO: NEED SERVICE TIME??
    ev->host->backoff = backoff;
    insert(ev);
}

void process_arrival_event(Event* curr_ev) {
    // cout << "process arrival event" << endl;
    current_time = curr_ev->event_time;
    double next_event_time = current_time + neg_exp_time(ARRIVAL_RATE); // create next arrival for host
    create_arrival(next_event_time, curr_ev->host);

    if (channel_idle) {
        // cout << "Channel is idle" << endl;
        // Since channel is not busy, can go to backoff procedure right away
        double backoff = generate_backoff(); // create a backoff event
        double backoff_event_time = current_time + DIFS;
        create_backoff(backoff_event_time, curr_ev->host, backoff);
    } else {
        // TODO IF CHANNEL IS BUSY
    }
}

void process_backoff_event(Event* curr_ev) {
    cout << "process backoff event" << endl;
    current_time = curr_ev->event_time;
    // cout << current_time << endl;
    // Sense the channel
    if (channel_idle) {
        int decrease_backoff = curr_ev->host->backoff - 1;
        // TODO: case if counter = 0
        if (decrease_backoff == 0) {
            // cout << "backoff is 0" << endl;
            // cout << curr_ev->event_time << endl;
        } else {
            double next_event_time = current_time + SENSE;
            // cout << next_event_time << endl;
            create_backoff(next_event_time, curr_ev->host, decrease_backoff);
        }
    }
    // TODO: IF CHANNEL IS BUSY
}

void initialize() {

    /* Initialize processing variables */
    current_time = 0.0;
    channel_idle = true;

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
    cout << fixed << endl;
    cout << setprecision(5);
    initialize();

    for (int i = 0; i < 10; ++i) {
        if (GELsize == 0) {
            break;
        }
        // 1. get first event from GEL
        Event *ev = GELhead; 
        delete_head(); // delete front

        // 2. if the event is an arrival event then process-arrival-event
        if (ev->type == Event::arrival) {
            process_arrival_event(ev);
        } else if (ev->type == Event::backoff) { // 3. if the event is a backoff event then process-backoff-event
            process_backoff_event(ev);
        }
        
        // TODO: 4. Otherwise, it must be a departure event and hence process-service-completion
    }

}