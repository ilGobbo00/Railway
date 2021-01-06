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

/* TODO:
 *  1. Print degli avvenimenti con central_rlw->append(string)
 * */
#include "Railway.h"
using std::cout;
using std::vector;

enum trainState{
    normalMove = 0,
    stationMove = 1,
    platformStation = 2,
    park = 3,
    endReached = 4
};

enum stationRequest{
    invalid = -3,
    transit = -2,
    reject = -1,
    // binari = 0, 1
};

// ===== CLASSE TRAIN =====
Train::Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail)
        : train_num_{number}, reverse_{rev}, max_spd_{max},  arrivals_{times}, delay_{0}, wait_count_{arrivals_[0]},
          status_{platformStation}, time_arrival_next_stat_{1}, last_request_{invalid}, central_railw_{rail}
{
    curr_km_ = curr -> distance();
    next_stat_ = curr -> next_stat();
}

void Train::advance_train() {                                                                                                           // Calcolo del km al prossimo minuto. Aggiorna stato
    switch (status_) {
        case normalMove:                                                                                                                // Movimento Normale
            !reverse_ ? curr_km_ += curr_spd_ / 60 : curr_km_ -= curr_spd_ /60;                                                         // Avanzamento treno
            if ((!reverse_ && curr_km_ >= next_stat_->distance() - 20) || (reverse_ && curr_km_ <= next_stat_->distance() + 20)) {      // Appena ha superato il limite di 20km (Non dovrebbe essere necessario currkm < next_station.distance)
                if(max_spd_ >= 160)                                                                                                     // Per evitare collisioni usciti dalla stazione appena prima del blocco delle partenze è necessario
                    curr_spd_ = 190;                                                                                                    //  limitare la velocità dei treni il transdito a 190km/h

                if ((!reverse_ && curr_km_ < next_stat_->distance() - 5) || (reverse_ && curr_km_ > next_stat_->distance() + 5))        // Treno tra i 20km e i 5km prima
                    if (last_request_ == invalid)
                        last_request_ = next_stat_->answer(this);                                                                    // Il treno continua a chiedere cosa deve fare finchè non gli viene data una risposta esaustiva
                else{                                                                                                                   // Treno dentro i 5km prima (non può essere last_request = invalid
                    !reverse_ ? curr_km_ = next_stat_->distance() - 5 : curr_km_ = next_stat_->distance() + 5;                          // Appena sfora i 5km lo riporta indietro
                    if (last_request_ == reject) {                                                                                      // Richiesta di binario rifiutata = vai in parcheggio
                        curr_spd_ = 0;
                        status_ = park;
                    } else {
                        if(last_request_ != transit) {                                                                                  // Se il treno non è nè nel parcheggio nè di transito vuoldire che deve andare al binario
                            status_ = stationMove;
                            curr_spd_ = 80;
                        }
                    }
                }
            }
            break;
        case stationMove:  // TODO: Aggiungere il controllo che sia il capolinea                                                                                         // Movimento stazione
            if(!reverse_) {
                curr_km_ += curr_spd_ / 60;
                if (curr_km_ >= next_stat_->distance()) { // conto del ritardo, annuncio, wait_count
                    curr_km_ = next_stat_->distance();
                    next_stat_->announce(this);
                    status_ = platformStation;
                    delay_ = central_railw_->curr_time() - arrivals_[time_arrival_next_stat_];
                    wait_count_ = 5;
                    arrived();
                }

            }
        // Imposto wait_count_ = 5 se arrivo in stazione
            break;

        case platformStation:                                                                                       // Binario nella stazione, sicuramente avrà una stazione successiva altrimenti sarebbe nello strato endReached
            if (wait_count_ <= 0) {                                                                                 // Partenza del treno (prima partenza gestita nell'assegnamento di wait_count nel costruttore)
                if (curr_stat_->answer_exit(this)) {
                    curr_spd_ = 190;
                    !reverse_ ? next_stat_ = curr_stat_->next_stat() : next_stat_ = curr_stat_->prev_stat();
                    curr_stat_ = nullptr;
                    status_ = stationMove;
                    time_arrival_next_stat_++;
                }
            }

            break;
        case park:                                 // Parcheggio

            break;
        case endReached:                                 // Fine corsa
            break;
        default:
            throw std::logic_error("Invalid status in class Train\n");
    }
}
}


std::string Train::train_num() const {              // Numero del treno
    return train_num_;
}
bool Train::reverse() const{                        // Andata (false) /ritorno (true)
    return reverse_;
}
double Train::max_spd() const{                      // Massima velocità del treno, possibile controllo del tipo di treno
    return max_spd_;
}
double Train::curr_spd() const{                     // Velocità corrente: solitamente massima, altrimenti velocità del treno davanti, altrimenti gli 80km/h
    return curr_spd_;
}
void Train::set_curr_spd(double val) {              // Imposta velocità corrente
    curr_spd_ = val;
}
double Train::current_km() const{                   // Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    return curr_km_;
}
Station* Train::curr_stat() const{                  // Ritorna il puntatore alla stazione dove risiede
    return curr_stat_;
}
Station* Train::next_stat() const{                  // Ritorna il puntatore alla prossima stazione
    return next_stat_;
}
const std::vector<int>& Train::arrivals() const{    // Ritorna il riferimento al vettore arrivals_ (non è detto che serva)
    return arrivals_;
}
int Train::delay() const{                           // Ritorna l'eventuale anticipo/ritardo
    return delay_;
}
int Train::wait_count() const{                      // Ritorna il countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    return wait_count_;
}
void Train::set_wait_count(int min){                // Imposta il countdown d'attesa (in qualsiasi situazione)
    wait_count_ =  min;
}
int Train::status() const{                          // Ritorna lo stato del treno (0 movimento normale, 1 movimento nei pressi della stazione, 2 nel binario, 3 nel parcheggio, 4 capolinea)
    return status_;
}
void Train::set_status(int status){
    status_ = status;                               // Imposta lo stato del treno
}



// ===== CLASSE REGIONAL =====
Regional::Regional(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
//void Regional::arrived(){}								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione


// ===== CLASSE FAST =====
Fast::Fast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
//void Fast::arrived(){}								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



// ===== CLASSE SUPERFAST =====
SuperFast::SuperFast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
//void SuperFast::arrived(){}								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



