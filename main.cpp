#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <queue>
using namespace std;

struct Event;

struct Frame {
    double r; // length of frame
    double transmission_time; // transmission time of frame
    double queue_time;
    bool is_ack;
};

struct Host {
    queue <Event*> buffer;
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
double ARRIVAL_RATE; // lambda

/* Constant variables for processing */
const int NUM_HOSTS = 50;
const int T = 150;
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
    ev->fr->queue_time = 0.0;

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

}

void process_arrival_event(Event* curr_ev) {
    current_time = curr_ev->event_time;

    if (curr_ev->fr->is_ack) {
        channel_idle = true;
        if (!hosts[curr_ev->src]->buffer.empty()) { // if buffer has frames to send, create backoff event
            double backoff_time = current_time + SENSE;
            Event* prev_ev = hosts[curr_ev->src]->buffer.front();
            total_delay += (current_time - prev_ev->fr->queue_time);
            hosts[curr_ev->src]->buffer.pop();
            create_backoff(backoff_time, prev_ev, generate_backoff());
        } else {
            hosts[curr_ev->src]->backoff = -1; // otherwise set backoff to -1
        }
        return;
    }
    
    if (channel_idle) { // Since channel is not busy, go to backoff procedure right away
        if (hosts[curr_ev->src]->backoff >= 0) { // if backoff procedure is already underway for a host, put frame in buffer
            hosts[curr_ev->src]->buffer.push(curr_ev);
            curr_ev->fr->queue_time = current_time + DIFS;
            return;
        }
        double backoff_event_time = current_time + DIFS;
        create_backoff(backoff_event_time, curr_ev, generate_backoff());
        // Create another arrival event for the host
        double next_arrival_time = current_time + neg_exp_time(ARRIVAL_RATE);
        double next_arrival_len = generate_frame_len();
        create_arrival(next_arrival_time, curr_ev->src, generate_dest(curr_ev->src), next_arrival_len, generate_transmission_time(next_arrival_len), false);

    } else { // channel is busy, sense it again
        double next_arrival_time = current_time + SENSE;
        create_arrival(next_arrival_time, curr_ev->src, curr_ev->dest, curr_ev->fr->r, curr_ev->fr->transmission_time, curr_ev->fr->is_ack);
    }
}

void process_backoff_event(Event* curr_ev) {
    current_time = curr_ev->event_time; 

    if (channel_idle) {
        int decrement_backoff = hosts[curr_ev->src]->backoff - 1;
        if (decrement_backoff == 0) {
            double dep_event_time = current_time + curr_ev->fr->transmission_time + SIFS;
            total_delay += curr_ev->fr->transmission_time + SIFS;
            create_departure(dep_event_time, curr_ev);
            channel_idle = false; // begin transmission
        } else { // Decrement counter and sense channel again
            double next_backoff_time = current_time + SENSE;
            create_backoff(next_backoff_time, curr_ev, decrement_backoff);
        }
    } else { // channel is busy, freeze counter and sense again
        double next_backoff_time = current_time + SENSE; 
        create_backoff(next_backoff_time, curr_ev, hosts[curr_ev->src]->backoff);
    }
}

void process_departure_event(Event* curr_ev) {
    current_time = curr_ev->event_time;
    transmitted_bytes += curr_ev->fr->r;
    double ack_trans_time = generate_transmission_time(ACK_FRAME);
    total_delay += ack_trans_time;
    double ack_arrival_time = current_time + ack_trans_time;
    create_arrival(ack_arrival_time, curr_ev->dest, curr_ev->src, ACK_FRAME, ack_trans_time, true);
}

void compute_statistics() {
    total_time = current_time;

    throughput = transmitted_bytes / total_time;
    cout << "Throughput: " << throughput << endl;    

    avg_network_delay = total_delay / throughput;
    cout << "Average network delay: " << avg_network_delay << endl;

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

    cout << "Enter the arrival rate: ";
    cin >> ARRIVAL_RATE;

    initialize();

    for (int i = 0; i < 100000; ++i) {
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

    compute_statistics();
    
}