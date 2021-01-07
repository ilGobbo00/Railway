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
#include <math.h>

using std::cout;
using std::vector;

enum trainState{
    normalMotion = 0,
    stationMotion = 1,
    platformStation = 2,
    park = 3,
    endReached = 4
};

enum stationRequest{
    invalid = -3,
    transit = -2,
    reject = -1,
    to_platform = 0,
};

enum delayStatus{
    aumentato = 1,
    uguale = 0,
    diminuito = -1
};

// ===== CLASSE TRAIN =====
Train::Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail)
        : train_num_{number}, is_slowing_{false}, reverse_{rev}, max_spd_{max/60}, curr_spd_{0}, curr_stat_{curr}, arrivals_{times}, delay_{0},
        old_delay_{0}, wait_count_{arrivals_[0]}, status_{platformStation}, time_arrival_next_stat_{0}, last_request_{invalid}, central_railw_{rail}
{
    curr_km_ = curr_stat_ -> distance();                                                                                                // Se il treno fa il percorso opposto allora ha il massimo dei km che verranno decrementati
    !reverse_ ? next_stat_ = curr_stat_ -> next_stat() : next_stat_ = curr_stat_ -> prev_stat();                                        // (Ripetizione di platformStation) La prossima stazione dipende se il treno va avanti o nella direzione opposta
}

void Train::advance_train() {                                                                                                           // Calcolo del km al prossimo minuto. Aggiorna stato
    switch (status_) {
        case normalMotion:                                                                                                                // Movimento Normale
            !reverse_ ? curr_km_ += curr_spd_ : curr_km_ -= curr_spd_;                                                        // Avanzamento treno
            if ((!reverse_ && curr_km_ >= next_stat_->distance() - 20) || (reverse_ && curr_km_ <= next_stat_->distance() + 20)) {      // Appena supera i 20km
                if(curr_spd_ > 190/60)                                                                                                  // Per evitare collisioni usciti dalla stazione appena prima del blocco
                    curr_spd_ = 190/60;                                                                                                 //  delle partenze è necessario limitare la velocità dei treni il transdito a 190km/h
                if((!reverse_ && curr_km_ < next_stat_->distance() - 5) || (reverse_ && curr_km_ > next_stat_->distance() + 5)) {       // Treno tra i 20km e i 5km prima
                    if (last_request_ == invalid) {                                                                                     // Il treno continua a chiedere cosa deve fare finchè non gli viene data una risposta esaustiva
                        delay_calc();
                        last_request_ = next_stat_->request(this);
                        if(last_request_ >= 0) last_request_ = to_platform;                                                             // Se mi restituisce un binario pongo 0 = to_platform
                    }
                }else {                                                                                                                 // Treno dentro i 5km prima (non può essere last_request = invalid)
                    if(is_slowing_ || last_request_ == reject){                                                                         // Se è stato rifiutato o sta rallentando un treno ca nel parcheggio
                        !reverse_ ? curr_km_ = next_stat_->distance() - 5 : curr_km_ = next_stat_->distance() + 5;                      // Appena sfora i +/-5km lo riporta indietro/avanti
                        curr_spd_ = 0;
                        status_ = park;
                    }else{
                        if(last_request_ == to_platform)                                                                                // Se deve andare alla stazione rallenta a 80km/h
                            curr_spd_ = 80/60;
                        status_ = stationMotion;
                    }
                }
            }
            break;

        case stationMotion:                                                                                             // Movimento stazione
            !reverse_ ? curr_km_ += curr_spd_
                      : curr_km_ -= curr_spd_;;                                                                     // Avanzo con la velocità corrente (190 in caso di transito, 80 in caso di fermata)
            if ((!reverse_ && curr_stat_ != nullptr || (reverse_ && curr_km_ < curr_stat_->distance() - 5))) {      // Se sono appena partito dalla stazione precedente
                if (curr_km_ > curr_stat_->distance() + 5) {                                                        // Se sono dopo i 5km della stazione precedente posso andare al massimo
                    curr_stat_ = nullptr;
                    curr_spd_ = max_spd_;
                    last_request_ = invalid;
                    status_ = normalMotion;
                }                                                                                                   // Altrimenti avanza ancora il giro successivo
            } else {                                                                                                // Se curr_stat_ == nullptr vuol dire che sono in arrivo (e non sto partendo)
                if (last_request_ == to_platform) {
                    if ((!reverse_ && curr_km_ >= next_stat_->distance() ||
                         (reverse_ && curr_km_ <= next_stat_->distance()))) {
                        curr_spd_ = 0;
                        curr_km_ = next_stat_->distance();
                        wait_count_ = 5;
                        status_ = platformStation;
                        arrived();
                    }
                }                                                                                                                                   // Se deve transitare e ha superato la stazione la prossima stazione
                if ((last_request_ == transit) && (!reverse_ && curr_km_ >= next_stat_->distance()) || (reverse_ && next_stat_->distance())) {      //  sarà la successiva di quella che ha appena superato
                    curr_stat_ = next_stat_;                                                                                                        // Rendo cur_stat_ diverso da nullptr perchè così il caso
                    !reverse_ ? next_stat_ = next_stat_->next_stat() : next_stat_ = next_stat_->prev_stat();                                        //  dopo viene gestito correttamente
                }
            }
            break;

        case platformStation:                                                                                       // Binario nella stazione, sicuramente avrà una stazione successiva altrimenti sarebbe nello strato endReached
            if(wait_count_ <= 0) {                                                                                 // Partenza del treno (prima partenza gestita nell'assegnamento di wait_count nel costruttore)
                if((!reverse_ && curr_stat_ -> next_stat() != nullptr) || (reverse_ && curr_stat_ -> prev_stat() != nullptr)) {         // Se non sono a fine corsa
                    delay_calc();
                    if(curr_stat_->request_exit(this)) {
                        curr_spd_ = 80/60;
                        !reverse_ ? next_stat_ = curr_stat_->next_stat() : next_stat_ = curr_stat_->prev_stat();
                        status_ = stationMotion;
                        time_arrival_next_stat_++;                                                                            //   del riferimento dell'orario
                    }else
                        delay_++; // Probabilmente non necessario
                }else{                                                                                             // Se la stazione successiva è nullptr vuol dire che è al capolinea
                    status_ = endReached;
                    arrived();
                }
            }else                                                                                                   // Decremento il tempo d'attesa finchè non ragginge lo 0
                wait_count_--;
            break;

        case park:                                 // Parcheggio
            // last req = invalid
            // Anche i treni di transito possono andare in parcheggio
            // TODO: Fare il conto se ritardo o anticipo, se sono un rallentatore devo aspettare 5 min prima di poter fare una nuova request alla stazione. Se sono un rallentatore tiro giù la flag dopo 5 min
            break;

        case endReached:                                 // Fine corsa

            break;

        default:
            throw std::logic_error("Invalid status in class Train\n");
    }
}

void Train::delay_calc(){                                                                                               // Essendo nella classe Train questa funzione verrà fatta dai treni che possono saltare le fermate, Regional ha un caso a parte
    old_delay_ = delay_;                                                                                                // Ritardo di prima
    switch (status_) {
        case normalMotion:
            calculate_normalMotion_status_delay();
            break;

        case stationMotion:
            //delay_ = central_railw_->curr_time() - arrivals_[time_arrival_next_stat_];            // TODO: sitemare la gestione del ritardo in questo caso
            break;

        case platformStation:   // Quanto è in ritardo dal partire
            delay_ = central_railw_->curr_time() - arrivals_[time_arrival_next_stat_] + 5;                              // Tempo corrente - tempo in cui dovevo arrivare
            if((!reverse_ && curr_stat_ -> prev_stat() == nullptr)||(reverse_ && curr_stat_->next_stat() == nullptr))   // Se sono all'inizio della mia corsa (inversa o non)
                delay_ - 5;
            break;

        case park:
            delay_ = central_railw_->curr_time() + 3.75 - arrivals_[time_arrival_next_stat_];                           // Tempo corrente + il tempo che ci mette ad arrivare in stazione - il tempo a cui dovrebbe arrivare
            break;
    }
}

int Train::changed_dilay(){
    if(status_ != platformStation && old_delay_ != delay_)
        return old_delay_ < delay_ ? aumentato : diminuito;
    else return uguale;
}

void Train::calculate_normalMotion_status_delay(){
    int station_to_skip = 0;            // Numero di stazioni prima di trovare quella in cui il treno si fermerà
    int normal_motion_distance = 0;     // Distanza che devo percorrere a max_spd_ per andare da una stazione alla successiva (non considerando i 5 km prima e dopo)
    int delay_due_to_station = 0;
    Station* station_stop = next_stat_; // Riferimento alla prossima stazione (salvata nel treno! dopo station_stop punta alla stazione e quindi il reverse cambia il prev_stat() o il next_stat()!
    bool principal = false;             // Variabile per il ciclo while

    while(!principal) {                                                                                                     // Controllo finchè non trovo la stazione in cui dovrò fermarmi per calcolare il ritardo
        if (nullptr == dynamic_cast<Principal*>(station_stop)) {
            if(!reverse_) {
                normal_motion_distance += station_stop->next_stat()->distance() - station_stop->distance() - 25;            // Tutta la distanza tra una stazione e l'altra in cui posso andare a max_spd_
                station_stop = station_stop->next_stat();
            }
            else{
                normal_motion_distance += abs(station_stop->prev_stat()->distance() - station_stop->distance()) - 25;   // Tutta la distanza tra una stazione e l'altra in cui posso andare a max_spd_
                station_stop = station_stop->prev_stat();
            }
            station_to_skip++;
        } else
            principal = true;
    }
    /* Tempo mancante stimato per arrivare (in ordine):
     * Prima di arrivare ai -5km dalla stazione: tempo = spazio fino ai -5km dalla stazione / velocità corrente(190km/h)
     * Per ogni stazione (-5km -> +5km) 10km percorsi a 190km/h (3.17km/min). Il tempo per transitare su ogni stazione = 10km / 3.17km/min = 3.16min a stazione
     * Da una stazione alla succesiva avrò la distanza delle due stazioni - 5km dopo la prima stazione e -20km prima della seconda stazione dove posso andare a max_spd_
     *
     * Invoco ceil al posto di mettere i valori interi arrotondati per eccesso per poter capire meglio in fase di revisione del codice
     */
    station_to_skip != 0 ? delay_due_to_station = station_to_skip * ceil(3.16) : delay_due_to_station = ceil(3.75);                                                // Se è di transito ma la prossima stazione è una principale devo contare il tempo per fare i 5km a 80km/h
    delay_ = (central_railw_->curr_time() + ceil((abs(next_stat()->distance() - curr_km_) - 5) / curr_spd_) + delay_due_to_station + normal_motion_distance / max_spd_) - arrivals_[time_arrival_next_stat_];
}
void Regional::calculate_normalMotion_status_delay(){
    delay_ = central_railw_->curr_time() + ceil(3.75) + ceil((abs(next_stat()->distance() - curr_km_) - 5) /curr_spd_) - arrivals_[time_arrival_next_stat_];     // Tempo in cui arriverò dopo errer giunto in stazione - tempo d'arrivo stimato
}

std::string Train::train_num() const {              // Numero del treno
    return train_num_;
}
bool Train::is_slowing() const{             // Ritorna se il treno sta rallentando qualcuno
    return is_slowing_;
}
void Train::set_slowing(bool is_slowing){   // Impostare il caso in cui il treno stia rallentando qualcuno
    is_slowing_ = is_slowing;
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
//void Regional::arrived(){
// il treno regionale numero x delle ore x
// }								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione


// ===== CLASSE FAST =====
Fast::Fast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
//void Fast::arrived(){
// Il treno veloce
// }								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



// ===== CLASSE SUPERFAST =====
SuperFast::SuperFast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
//void SuperFast::arrived(){
//Il treno alta velocita
// }								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione



