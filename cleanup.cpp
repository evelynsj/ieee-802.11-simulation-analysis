#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
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

double generate_frame_len() { // negative exponentially distributed random variable in range 0 < r <= 1544
    double len = neg_exp_time(1) * MAX_FRAME; // multiplied by MAX_FRAME to scale 
    if (len > MAX_FRAME) {
        return MAX_FRAME;
    }
    return len; // in bytes
}

double generate_transmission_time(double len) {
    return (len * 8) / CHANNEL_CAP;
}

int generate_backoff() {
    static default_random_engine generator;
    uniform_real_distribution<double> distribution(1,T);

    double dist = distribution(generator) * T; // generate uniform random variable between (0,1) and multiply by T
    return round(dist);
}

int generate_dest(int i) {
    int dest;
    do {
        dest = rand() % NUM_HOSTS;
    } while (i == dest);

    return dest;
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
        cout << "Src = " << curr->src << endl;
        cout << "Dest = " << curr->dest << endl;
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

void create_arrival(double ev_time, int src, int dest, int len, double trans_time, bool is_ack) {
    Event* ev = new Event;
    ev->fr = new Frame;

    ev->event_time = ev_time;
    ev->type = Event::arrival;
    ev->src = src;
    ev->dest = dest;

    ev->fr->r = len;
    ev->fr->transmission_time = trans_time;
    ev->fr->is_ack = is_ack;

    insert(ev);
}

void create_backoff(double ev_time, Event* prev_ev, int backoff) {
    Event* ev = new Event;
    ev->fr = new Frame;

    ev->event_time = ev_time;
    ev->type = Event::backoff;
    ev->src = prev_ev->src;
    ev->dest = prev_ev->dest;

    ev->fr = prev_ev->fr;

    hosts[ev->src]->backoff = backoff;

    insert(ev);
}

void create_departure(double ev_time, Event* prev_ev) {
    Event *ev = new Event;
    ev->fr = new Frame;

    ev->event_time = ev_time;
    ev->type = Event::departure;
    ev->src = prev_ev->src;
    ev->dest = prev_ev->dest;

    ev->fr = prev_ev->fr;

    insert(ev);

    // TODO: set backoff to -1? 
}

void process_arrival_event(Event* curr_ev) {
    cout << "Process arrival event" << endl;
    current_time = curr_ev->event_time;

    if (curr_ev->fr->is_ack) {
        cout << "Ack frame" << endl;
        channel_idle = true;
        return;
    }

    // Create another arrival event for the host
    double next_arrival_time = current_time + neg_exp_time(ARRIVAL_RATE);
    double next_arrival_len = generate_frame_len();
    create_arrival(next_arrival_time, curr_ev->src, generate_dest(curr_ev->src), next_arrival_len, generate_transmission_time(next_arrival_len), false);

    if (channel_idle) { // Since channel is not busy, go to backoff procedure right away
        double backoff_event_time = current_time + DIFS;
        create_backoff(backoff_event_time, curr_ev, generate_backoff());
    } else {
        // TODO: if channel is busy, create another arrival event
    }
}

void process_backoff_event(Event* curr_ev) {
    cout << "Process backoff event" << endl;
    current_time = curr_ev->event_time; 

    if (channel_idle) {
        int decrement_backoff = hosts[curr_ev->src]->backoff - 1;
        if (decrement_backoff == 0) {
            double dep_event_time = current_time + curr_ev->fr->transmission_time + SIFS;
            create_departure(dep_event_time, curr_ev);
            channel_idle = false; // begin transmission
        } else { // Decrement counter and sense channel again
            double next_backoff_time = current_time + SENSE;
            create_backoff(next_backoff_time, curr_ev, decrement_backoff);
        }
    } else {
        // TODO: if channel is busy, freeze counter and create another backoff event
    }
}

void process_departure_event(Event* curr_ev) {
    cout << "Process departure event" << endl;
    current_time = curr_ev->event_time;
    double ack_trans_time = generate_transmission_time(MAX_FRAME);
    double ack_arrival_time = current_time + ack_trans_time;
    create_arrival(ack_arrival_time, curr_ev->dest, curr_ev->src, MAX_FRAME, ack_trans_time, true);
}

void initialize() {
    /* Initialize global variables */
    current_time = 0.0;
    channel_idle = true;

    /* Initialize statistical variables */
    transmitted_bytes = 0.0;
    total_time = 0.0;
    total_delay = 0.0;
    throughput = 0.0;
    avg_network_delay = 0.0;

    /* Initialize data structures */
    GELhead = nullptr;
    GELtail = nullptr;
    GELsize = 0;

    for (int i = 0; i < NUM_HOSTS; ++i) {
        hosts[i] = new Host;
        hosts[i]->backoff = -1;
        double init_arrival_time = current_time + neg_exp_time(ARRIVAL_RATE);
        double frame_len = generate_frame_len();
        create_arrival(init_arrival_time, i, generate_dest(i), frame_len, generate_transmission_time(frame_len), false);
    }
}

int main() {
    cout << T << ARRIVAL_RATE << MAX_FRAME << ACK_FRAME << CHANNEL_CAP << SIFS << DIFS << SENSE << endl;
    cout << fixed << endl;
    cout << setprecision(5);

    initialize();

    for (int i = 0; i < 10; ++i) {
        cout << "**i " << i << endl;
        if (GELsize == 0) {
            break;
        }

        Event *ev = GELhead; // Get first event from GEL
        delete_head(); // delete front

        if (ev->type == Event::arrival) {
            process_arrival_event(ev);
        } else if (ev->type == Event::backoff) {
            process_backoff_event(ev);
        } else {
            process_departure_event(ev);
        }
    }
    
}