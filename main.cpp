#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>
#include <random>
using namespace std;

struct Host;

struct Frame {
    double r;
    bool ack; // to indicate if frame is ack or not
    Host* src;
    Host* dest;
};

struct Event {
    double event_time;
    double service_time; // TODO: might not need
    enum event_type { arrival, departure, backoff };
    event_type type;
    Event* next;
    Event* prev;
    Frame* fr;
    Host* host; // TODO: needed? or just need the number? if there's already src, does host matter?
};

struct Host {
    queue <Event*> buffer; // TODO: when do we use this?
    int backoff;
};

// TODO: might not need
Event* GELhead;
Event* GELtail;
int GELsize;

/* TODO: global variables for processing */
int NUM_HOSTS = 50; // variable
int T = 400; // variable. TODO: figure out how much this is?
double ARRIVAL_RATE = 0.01; // variable lambda
double MAX_FRAME = 1544;
double ACK_FRAME = 64;
double CHANNEL_CAP = 10000000; // 1 Mbps = 10^6 bits/sec 
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

void generate_dest(Host* hosts[]) {
    int dest;
    do {
        dest = rand() % NUM_HOSTS;
    } while (GELhead->fr->src == hosts[dest]);
    GELhead->fr->dest = hosts[dest];
}

void generate_dest_init(Host* hosts[]) {
    int dest;
    int i = 0;

    Event *curr = GELhead;
    while (curr != nullptr) {
        do {
            dest = rand() % NUM_HOSTS;
        } while (curr->fr->src == hosts[dest]);
        curr->fr->dest = hosts[dest];
        curr = curr->next;
        i++;
    }
}

int generate_frame_len() {
    // negative exponentially distributed random variable in range 0 < r <= 1544
    double len = neg_exp_time(1) * MAX_FRAME; // multiplied by MAX_FRAME to scale 
    if (len > MAX_FRAME) {
        return MAX_FRAME;
    }

    return round(len);
}

double generate_transmission_time(double len) {
    return (len * 8) / CHANNEL_CAP;
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
        cout << curr->fr->src << endl;
        cout << curr->fr->dest << endl;
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
    ev->fr = new Frame;
    ev->fr->r = generate_frame_len();
    ev->fr->ack = false;
    ev->fr->src = host;
    insert(ev);
}

void create_ack_arrival(double ev_time, Host* src, Host* dest) {
    Event *ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::arrival;
    ev->host = src;
    ev->fr = new Frame;
    ev->fr->r = ACK_FRAME;
    ev->fr->ack = true;
    ev->fr->src = src;
    ev->fr->dest = dest;
    insert(ev);
}

void create_backoff(double ev_time, Host* host, int backoff, Frame* frame) {
    // cout << "Create backoff" << endl;
    Event *ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::backoff;
    ev->host = host;
    // TODO: NEED SERVICE TIME??
    ev->host->backoff = backoff;
    ev->fr = new Frame;
    ev->fr = frame;
    insert(ev);
}

void create_departure(double ev_time, Host* host, Frame* frame) {
    Event* ev = new Event;
    ev->event_time = ev_time;
    ev->type = Event::departure;
    ev->host = host;
    ev->fr = new Frame;
    ev->fr = frame;
    insert(ev);
}

void process_arrival_event(Event* curr_ev, Host* hosts[]) {
    cout << "process arrival event" << endl;
    if (curr_ev->fr->ack) {
        cout << "frame is ack" << endl;
        channel_idle = true;
        return;
    }
    current_time = curr_ev->event_time;
    cout << current_time << endl;
    double next_event_time = current_time + neg_exp_time(ARRIVAL_RATE); // create next arrival for host that is not ack
    cout << "Next event time is " << next_event_time << endl;
    create_arrival(next_event_time, curr_ev->host);
    generate_dest(hosts);

    if (channel_idle) {
        // cout << "Channel is idle" << endl;
        // Since channel is not busy, can go to backoff procedure right away
        double backoff = generate_backoff(); // create a backoff event
        double backoff_event_time = current_time + DIFS;
        create_backoff(backoff_event_time, curr_ev->host, backoff, curr_ev->fr);
        // TODO MIGHT NEED TO INSERT TO BUFFER HERE
    } else {
        // TODO IF CHANNEL IS BUSY
        cout << "CHANNEL IS BUSY" << endl;
    }
}

void process_backoff_event(Event* curr_ev) {
    cout << "process backoff event" << endl;
    current_time = curr_ev->event_time;
    cout << current_time << endl;
    // Sense the channel
    if (channel_idle) {
        int decrease_backoff = curr_ev->host->backoff - 1;
        // TODO: case if counter = 0
        if (decrease_backoff == 0) {
            cout << "backoff is 0" << endl;
            // cout << curr_ev->event_time << endl;
            double transmission_time = generate_transmission_time(curr_ev->fr->r);
            double dep_event_time = current_time + transmission_time + SIFS;
            cout << "Departure event time " << dep_event_time << endl;
            create_departure(dep_event_time, curr_ev->host, curr_ev->fr);
            channel_idle = false;
            // TODO: src and dest is probably wrong ;(
            // TODO do we need to reset backoff to -1?           
            // TODO: for ack frame, create an arrival event and discard?  
            // TODO: ACK ARRIVAL IS PROB DIFFERENT LENGTH
        } else {
            double next_event_time = current_time + SENSE;
            // cout << next_event_time << endl;
            create_backoff(next_event_time, curr_ev->host, decrease_backoff, curr_ev->fr);
        }
    } else {
        // TODO: IF CHANNEL IS BUSY
        cout << "CHANNEL IS BUSY" << endl;
        
    }
    
}

void process_departure_event(Event* curr_ev) {
    cout << "process departure event" << endl;
    current_time = curr_ev->event_time;
    double ack_transmission_time = current_time + ((ACK_FRAME * 8) / CHANNEL_CAP);
    cout << current_time << endl;
    cout << ack_transmission_time << endl;
    create_ack_arrival(ack_transmission_time, curr_ev->fr->dest, curr_ev->fr->src);
}

void initialize(Host* hosts[]) {

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

    for (int i = 0; i < NUM_HOSTS; ++i) {
        hosts[i] = new Host;
        hosts[i]->backoff = -1;
        double first_arrival_time = neg_exp_time(ARRIVAL_RATE) + current_time;
        create_arrival(first_arrival_time, hosts[i]); // not ack frame
    }

    generate_dest_init(hosts);
    
}

int main() {
    Host *hosts[NUM_HOSTS];
    cout << fixed << endl;
    cout << setprecision(5);
    initialize(hosts);
    // iterate();

    for (int i = 0; i < 100000; ++i) {
        cout << "** i " << i << endl;
        if (GELsize == 0) {
            break;
        }
        // 1. get first event from GEL
        Event *ev = GELhead; 
        delete_head(); // delete front

        // 2. if the event is an arrival event then process-arrival-event
        if (ev->type == Event::arrival) {
            process_arrival_event(ev, hosts);
        } else if (ev->type == Event::backoff) { // 3. if the event is a backoff event then process-backoff-event
            process_backoff_event(ev);
        } else {
            process_departure_event(ev);
        }

        // iterate();
        
        // TODO: 4. Otherwise, it must be a departure event and hence process-service-completion
    }

}