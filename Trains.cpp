/*
 * Trains management.
 * Author: Riccardo Gobbo 1224272
 * */

/* TODO:
 *  1. Print degli avvenimenti con central_rlw->append(string)
 * */
#include "Railway.h"
#include <math.h>

using std::cout;
using std::vector;
using std::string;

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

//enum delayStatus{
//    aumentato = 1,
//    uguale = 0,
//    diminuito = -1
//};

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
        case normalMotion:                                                                                                              // Movimento Normale
            !reverse_ ? curr_km_ += curr_spd_ : curr_km_ -= curr_spd_;                                                                  // Avanzamento treno
            if ((!reverse_ && curr_km_ >= next_stat_->distance() - 20) || (reverse_ && curr_km_ <= next_stat_->distance() + 20)) {      // Appena supera i 20km
                if(curr_spd_ > 190/60.0)                                                                                                // Per evitare collisioni usciti dalla stazione appena prima del blocco
                    curr_spd_ = 190/60.0;                                                                                               //  delle partenze è necessario limitare la velocità dei treni il transdito a 190km/h
                if((!reverse_ && curr_km_ < next_stat_->distance() - 5) || (reverse_ && curr_km_ > next_stat_->distance() + 5)) {       // Treno tra i 20km e i 5km prima
                    if (last_request_ == invalid) {                                                                                     // Il treno continua a chiedere cosa deve fare finchè non gli viene data una risposta esaustiva
                        delay_calc();
                        communications();                                                                                               // 1. Segnalazione di un treno alla stazione (avviene una sola volta)
                        last_request_ = next_stat_->request(this);
                        if(last_request_ >= 0) {
                            plat_num_ = last_request_;                                                                                  // Mi salvo su che binario posso andare per la stampa
                            last_request_ = to_platform;                                                                                // Se mi restituisce un binario pongo 0 = to_platform
                        }
                    }
                }else {                                                                                                                 // Treno dentro i 5km prima (non può essere last_request = invalid)
                    if(is_slowing_ || last_request_ == reject){                                                                         // Se è stato rifiutato o sta rallentando un treno ca nel parcheggio
                        !reverse_ ? curr_km_ = next_stat_->distance() - 5 : curr_km_ = next_stat_->distance() + 5;                      // Appena sfora i +/-5km lo riporta indietro/avanti
                        curr_spd_ = 0;
                        is_slowing_ ? wait_count_ = 5 : wait_count_ = 0;
                        status_ = park;
                    }else{
                        // Il treno regionale 1234 delle ore xx:xx è in arrivo alla stazine di treviso con un ritardo/anticipo
                        if(last_request_ == to_platform) {                                                                                // Se deve andare alla stazione rallenta a 80km/h
                            curr_spd_ = 80 / 60.0;
                            communications();
                        }
                        status_ = stationMotion;
                    }
                }
            }
            break;

        case stationMotion:                                                                                                             // Movimento nei pressi della stazone
            !reverse_ ? curr_km_ += curr_spd_ : curr_km_ -= curr_spd_;                                                                  // Avanzo con la velocità corrente (190 in caso di transito, 80 in caso di fermata)
            if (curr_stat_ != nullptr) {                                                                                                // Se sono appena partito dalla stazione precedente
                if ((!reverse_ && curr_km_ > curr_stat_->distance() + 5) || (reverse_ && curr_km_ < curr_stat_->distance() - 5)){       // Se sono dopo i 5km della stazione precedente posso andare al massimo
                    curr_stat_ = nullptr;
                    curr_spd_ = max_spd_;
                    last_request_ = invalid;
                    status_ = normalMotion;
                }                                                                                                                       // Altrimenti avanza ancora il giro successivo
            } else {                                                                                                                    // Se curr_stat_ == nullptr vuol dire che sono in arrivo (e non sto partendo)
                if (last_request_ == to_platform) {                                                                                     // Se posso andare ai binari
                    if ((!reverse_ && curr_km_ >= next_stat_->distance() ||  (reverse_ && curr_km_ <= next_stat_->distance()))) {       // Se sono effettivamente arrivato ai binare
                        curr_stat_ = next_stat_;                                                                                        // La stazione corrente e' esattamente quella a cui dovevo arrivare
                        curr_spd_ = 0;                                                                                                  // Fermo il treno
                        curr_km_ = next_stat_->distance();                                                                              // Allineo il treno alla stazione
                        wait_count_ = 5;                                                                                                // Imposto il timer a 5 minuti per la salita o la discesa delle persone
                        status_ = platformStation;                                                                                      // Imposto lo stato in binario
                        communications();                                                                                               // Communicazione arrivo al binario
                    }
                }                                                                                                                                   // Se deve transitare e ha superato la stazione la prossima stazione
                if ((last_request_ == transit) && (!reverse_ && curr_km_ >= next_stat_->distance()) || (reverse_ && next_stat_->distance())) {      //  sarà la successiva di quella che ha appena superato
                    curr_stat_ = next_stat_;                                                                                                        // Rendo cur_stat_ diverso da nullptr perchè così il caso
                    !reverse_ ? next_stat_ = next_stat_->next_stat() : next_stat_ = next_stat_->prev_stat();                                        //  dopo viene gestito correttamente
                }
            }
            break;

        case platformStation:                                                                                                       // Binario nella stazione, sicuramente avrà una stazione successiva altrimenti sarebbe nello strato endReached
            if(wait_count_ <= 0) {                                                                                                  // Partenza del treno (prima partenza gestita nell'assegnamento di wait_count nel costruttore)
                if((!reverse_ && curr_stat_ -> next_stat() != nullptr) || (reverse_ && curr_stat_ -> prev_stat() != nullptr)) {     // Se non sono a fine corsa
                    delay_calc();
                    if(curr_stat_->request_exit(this)){                                                                          // Se la richiesta per poter uscire viene accettata
                        curr_spd_ = 80/60.0;                                                                                        // Imposto la velocita' limitata a 80km/h
                        !reverse_ ? next_stat_ = curr_stat_->next_stat() : next_stat_ = curr_stat_->prev_stat();                    // Faccio avanzare il treno nella giusta direzione
                        status_ = stationMotion;                                                                                    // Movimento nei pressi della staizone
                        time_arrival_next_stat_++;                                                                  // del riferimento dell'orario
                        // Treno partito dal binario
                    }else
                        delay_++; // Probabilmente non necessario
                }else{                                                                                              // Se la stazione successiva è nullptr vuol dire che è al capolinea
                    status_ = endReached;
                    curr_spd_ = 0;
                    delay_calc();
                    communications();
                }
            }else                                                                                                   // Decremento il tempo d'attesa finchè non ragginge lo 0
                wait_count_--;
            break;

        case park:                                                                  // Parcheggio (caso reverse e transit analizzati)
            if(is_slowing_){
                wait_count_ > 0 ? wait_count_-- : is_slowing_ = false;
            }else{
                delay_calc();
                last_request_ = next_stat_ -> request(this);
                if(last_request_ >= 0) last_request_ = to_platform;
                if(last_request_ != reject && last_request_ != invalid) {           // Nel caso di errori, con != invalid posso effettuare un'altra richiesta senza compromettere il sistema
                    if (last_request_ == to_platform) curr_spd_ = 80 / 60.0;
                    if (last_request_ == transit) curr_spd_ = 190 / 60.0;
                    status_ = stationMotion;
                    communications();
                }
            }
            break;

        case endReached:                            // Fine corsa
            curr_stat_ -> delete_train(this);       // Dice alla stazione di eliminare il treno che ha finito la corsa
            break;

        default:
            throw std::logic_error("Invalid status in class Train\n");
    }
}

// ===== Calcolo del ritardo dei treni =====
void Train::delay_calc(){                                                                                               // Essendo nella classe Train questa funzione verrà fatta dai treni che possono saltare le fermate, Regional ha un caso a parte
    old_delay_ = delay_;                                                                                                // Ritardo di prima
    switch (status_) {
        case normalMotion:
            calc_specific_delay();
            break;

        case stationMotion:
            delay_ = central_railw_ -> curr_time() + ceil(abs(next_stat() -> distance() - curr_km_) / (80/60.0)) - arrivals_[time_arrival_next_stat_];
            break;

        case platformStation:   // Quanto è in ritardo dal partire
            delay_ = central_railw_->curr_time() - arrivals_[time_arrival_next_stat_] + 5;                              // Tempo corrente - tempo in cui dovevo arrivare
            if((!reverse_ && curr_stat_ -> prev_stat() == nullptr)||(reverse_ && curr_stat_->next_stat() == nullptr))   // Se sono all'inizio della mia corsa (inversa o non)
                delay_ - 5;
            break;

        case park:
            calc_specific_delay();
            break;

        default:
            throw std::logic_error("Invalid status in calculation of delay (class Train)\n");
    }
}
void Train::calc_specific_delay(){
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
    delay_ = (central_railw_->curr_time() + ceil((abs(next_stat_->distance() - curr_km_) - 5) / curr_spd_) + delay_due_to_station + normal_motion_distance / max_spd_) - arrivals_[time_arrival_next_stat_];
}
void Regional::calc_specific_delay(){
    delay_ = central_railw_->curr_time() + ceil(3.75) + ceil((abs(next_stat()->distance() - curr_km_) - 5) /curr_spd_) - arrivals_[time_arrival_next_stat_];     // Tempo in cui arriverò dopo errer giunto in stazione - tempo d'arrivo stimato
}
std::string Train::changed_delay(){
        std::string to_return = "Il treno numero " + train_num_ + "diretto verso la stazione " + next_stat_->station_name();
        // Se prima era in orario
        if (old_delay_ == 0) {
            if (delay_ > 0)
                return to_return += " non e' più in orario, il suo ritardo e' aumentato di " + std::to_string(delay_) + " min\n";
            if (delay_ < 0)
                return to_return += " non e' più in orario, il suo anticipo e' aumentato di " + std::to_string(delay_) + " min\n";
            return "";
            // in orario prima e dopo
        }

        // Se prima era in anticipo
        if (old_delay_ < 0) {
            if (delay_ < 0) {
                if (old_delay_ > delay_)
                    return to_return += " ha aumentato il suo anticipo di " + std::to_string(abs(delay_ - old_delay_)) + " min\n";
                if (old_delay_ < delay_)
                    return to_return += " ha diminuito il suo anticipo di " + std::to_string(abs(old_delay_ - delay_)) + " min\n";
                return "";
            }
            if (delay_ > 0)
                return to_return += " non e' più in anticipo, ma in ritardo di " + std::to_string(delay_) + " min\n";
            return to_return += "non e' piu' in anticipo. Il treno è in orario\n";
        }

        // Se prima era in ritardo
        if (old_delay_ > 0) {
            if (delay_ > 0) {
                if (old_delay_ < delay_)
                    return to_return += " ha aumentato il suo ritardo di " + std::to_string(delay_ - old_delay_) + " min\n";
                if (old_delay_ > delay_)
                    return to_return += " ha diminuito il suo ritardo di " + std::to_string(old_delay_ - delay_) + " min\n";
                return "";
            }
            if (delay_ < 0)
                return to_return += " non e' piu' in ritardo, ma in anticipo di " + std::to_string(abs(delay_)) + " min\n";
            return to_return += "non e' piu' in anticipo. Il treno è in orario\n";
        }
        throw std::logic_error("Error in changed_delay function, non return is called\n");
}

// ===== Communicazioni dei treni =====
void Train::communications(){
    string comm;
    switch (status_) {
        case normalMotion:
            comm = "Il treno " + get_train_type() + " numero " + train_num_; // + " ha ricevuto l'ordine dalla stazione di " + next_stat_ -> station_name() + " di poter ";
            switch (last_request_) {
                case reject:
                    comm += " ha ricevuto l'ordine dalla stazione di " + next_stat_ -> station_name() + " di poter andare al parcheggio\n";
                    break;
                case to_platform:
                    comm +=  " delle ore " + print_time();
                    if(delay_ != 0)
                        delay_ > 0 ? comm += " con un ritardo di " + std::to_string(old_delay_)  : comm += " con un anticipo di " + std::to_string(old_delay_);
                    comm += " sta arrivando alla stazione di " + next_stat_ -> station_name() + " al binario n. " + std::to_string(plat_num_) + "\n";
                    break;
                case transit:
                    comm += " ha ricevuto l'ordine dalla stazione di " + next_stat_ -> station_name() + " di poter transitare\n";
                    break;
                case invalid:                               // Prima communicazione, quando il treno ha appena superato i 20km
                    comm += " è in arrivo alla stazione di " + next_stat_ -> station_name();
                    if(delay_ != 0)
                        delay_ > 0 ? comm += " con un ritardo di " + std::to_string(old_delay_)  : comm += " con un anticipo di " + std::to_string(old_delay_);
                    comm += "\n";
                    break;
            }
            break;

        case stationMotion:
            if(last_request_ == to_platform)
                comm = changed_delay();
            break;

        case platformStation:
            comm = "Il treno " + get_train_type() + " delle ore " + print_time() + " e' arrivato alla stazione di " + curr_stat_ -> station_name();
            if(delay_ != 0)
                if(delay_ < 0)
                    comm += " con un anticipo di " + std::to_string(abs(delay_)) + " min\n";
                else
                    comm += " con un ritardo di " + std::to_string(delay_) + " min\n";
            break;

        case park:
            // Non devo communicare niente se sono nel parcheggio
            break;

        case endReached:
            comm = "Il treno " + get_train_type() + " numero " + train_num_ + " e' giunto al capolinea";
            if(delay_ < 0)
                comm += " con un anticipo di " + std::to_string(abs(delay_)) + " min\n";
            if(delay_ > 0)
                comm += " con un ritardo di " + std::to_string(delay_) + " min\n";
            if(delay_ == 0)
                comm += " in orario\n";
            break;

        default:
            throw std::logic_error("Invalid status in communication\n");
    }
    central_railw_ -> append(comm);
    comm = "";
}


// ===== Funzioni get e set della classe train =====
std::string Train::train_num() const {              // Numero del treno
    return train_num_;
}
std::string Train::print_time()const{
    return std::to_string((arrivals_[time_arrival_next_stat_] /60) %24) + ":" + std::to_string(arrivals_[time_arrival_next_stat_] %60);
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
Regional::Regional(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail) : Train(number, rev, 160/60, curr, times, rail){}
std::string Regional::get_train_type()const{
    return "regionale";
}

// ===== CLASSE FAST =====
Fast::Fast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail):Train(number, rev, 240/60, curr, times, rail){}
std::string Fast::get_train_type()const{
    return "veloce";
}

// ===== CLASSE SUPERFAST =====
SuperFast::SuperFast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail):Train(number, rev, 300/60, curr, times, rail){}
std::string SuperFast::get_train_type()const{
    return "alta velocita'";
}



