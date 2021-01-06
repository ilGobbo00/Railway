/*
 * Trains management.
 * Author: Riccardo Gobbo 1224272
 * */

/* VARIABILI MEMBRO
    std::string train_num_;						// Numero del treno
    bool reverse_;								// True per i treni in ritorno
    const double max_spd_;						// km/min
    double curr_spd_;							// km/min
    double curr_km_;							// Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    Station* curr_stat_;						// nullptr quando parte dalla stazione (e quindi sta viaggiando) oppure è nel parcheggio
    Station* next_stat_;						// Puntatore alla prossima stazione di arrivo, è nullptr nel caso in cui sia al capolinea
    std::vector<int> arrivals_; 				// Orari in cui io arrivo alle stazioni
    int delay_;									// anticipo = ritardo negativo
    int wait_count_;							// countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    int status_; 								// 0 Mov Normale, 1 Mov Staz, 2 Binario, 3 Park, 4 Fine corsa
    int time_arrival_next_stat_;                // Orario in cui il treno dovrebbe arrivare alla stazione (indice del vettore arrivals
    int last_request_;                          // Risposta dall'utlima richiesta (-2 transito, -1 rifiutata, -3 default, >=0 num binario)
    Railway* central_railw_; 					// Per avere il tempo corrente mi serve il riferimento al rail
*/

#include "Railway.h"

// ===== CLASSE TRAIN =====
Train::Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail)
        : train_num_{number}, reverse_{rev}, max_spd_{max},  arrivals_{times}, delay_{0}, wait_count_{0},
          status_{2}, time_arrival_next_stat_{1}, last_request_{-3}, central_railw_{rail},
{
    curr_km_ = curr -> distance();
    next_stat_ = curr -> next_stat();
}

std::string Train::advance_train() {                // Calcolo del km al prossimo minuto. Aggiorna stato
}

std::string Train::train_num() const {          // Numero del treno
    return train_num_;
}

bool Train::reverse() const{                    // Andata (false) /ritorno (true)
    return reverse_;
}

double Train::max_spd() const{                  // Massima velocità del treno, possibile controllo del tipo di treno
    return max_spd_;
}
double Train::curr_spd() const{                 // Velocità corrente: solitamente massima, altrimenti velocità del treno davanti, altrimenti gli 80km/h
    return curr_spd_;
}
void Train::set_curr_spd(double val) {          // Imposta velocità corrente
    curr_spd_ = val;
}
double Train::current_km() const{               // Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    return curr_km_;
}
Station* Train::curr_stat() const{              // Ritorna il puntatore alla stazione dove risiede
    return curr_stat_;
}
Station* Train::next_stat() const{              // Ritorna il puntatore alla prossima stazione
    return next_stat_;
}
const std::vector<int>& Train::arrivals() const{      // Ritorna il riferimento al vettore arrivals_ (non è detto che serva)
    return arrivals_;
}
int Train::delay() const{                       // Ritorna l'eventuale anticipo/ritardo
    return delay_;
}
int Train::wait_count() const{                   // Ritorna il countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    return wait_count_;
}
void Train::set_wait_count(int min){              // Imposta il countdown d'attesa (in qualsiasi situazione)
    wait_count_ =  min;
}
int Train::status() const{                        // Ritorna lo stato del treno (0 movimento normale, 1 movimento nei pressi della stazione, 2 nel binario, 3 nel parcheggio, 4 capolinea)
    return status_;
}
void Train::set_status(int status){
    status_ = status;                             // Imposta lo stato del treno
}

virtual void request() = 0;					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
virtual void request_exit() = 0;			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
void arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione


// ===== CLASSE REGIONAL =====
Regional::Regional(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
void Regional::request();		    			// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
void Regional::request_exit();	    		// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
void Regional::arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione


// ===== CLASSE FAST =====
Fast::Fast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
void Fast::request(); 					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
void Fast::request_exit();    			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
void Fast::arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



// ===== CLASSE SUPERFAST =====
SuperFast::SuperFast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
void SuperFast::request();					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
void SuperFast::request_exit();			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
void SuperFast::arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



