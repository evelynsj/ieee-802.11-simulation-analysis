#include <iostream>
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

int main() {
}