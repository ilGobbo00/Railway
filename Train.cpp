/*
 * Trains management.
 * Author: Riccardo Gobbo 1224272
 * */

#include "Railway.h"
#include <cmath>
#include <cstdio>

using std::cout;
using std::vector;
using std::string;

enum trainStatus {
    normalMotion = 0,
    stationMotion = 1,
    platformStation = 2,
    park = 3,
    endReached = 4
};

enum stationRequest {
    invalid = -4,
    to_delete = -3,
    transit = -2,
    reject = -1,
    to_platform = 0,
};


// ===== CLASSE TRAIN =====
Train::Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail)
        : train_num_{number}, is_slowing_{ false }, reverse_{ rev }, max_spd_{ max }, curr_spd_{ 0 }, curr_stat_{ curr }, arrivals_{times}, delay_{ 0 },
          old_delay_{ 0 }, wait_count_{ arrivals_[0] }, status_{ platformStation }, time_arrival_next_stat_{ 0 }, last_request_{ invalid }, central_railw_{ rail }
{
    curr_km_ = curr_stat_->distance();                                                                                                // Se il treno fa il percorso opposto allora ha il massimo dei km che verranno decrementati
    !reverse_ ? next_stat_ = curr_stat_->next_stat() : next_stat_ = curr_stat_->prev_stat();                                          // (Ripetizione di platformStation) La prossima stazione dipende se il treno va avanti o nella direzione opposta
}

void Train::advance_train() {                                                                                                           // Calcolo del km al prossimo minuto. Aggiorna stato
    switch (status_) {
        case normalMotion:                                                                                                              // Movimento Normale
            !reverse_ ? curr_km_ += curr_spd_ : curr_km_ -= curr_spd_;                                                                  // Avanzamento treno
            if ((!reverse_ && curr_km_ >= next_stat_->distance() - 20) || (reverse_ && curr_km_ <= next_stat_->distance() + 20)) {      // Appena supera i 20km
                if (last_request_ == transit)
                    if (curr_spd_ > 190 / 60.0)                                                                                         // Per evitare collisioni usciti dalla stazione appena prima del blocco
                        curr_spd_ = 190 / 60.0;                                                                                         //  delle partenze e' necessario limitare la velocit� dei treni il transdito a 190km/h
                if ((!reverse_ && curr_km_ < next_stat_->distance() - 5) || (reverse_ && curr_km_ > next_stat_->distance() + 5)) {      // Treno tra i 20km e i 5km prima
                    if (last_request_ == invalid) {                                                                                     // Il treno continua a chiedere cosa deve fare finche' non gli viene data una risposta esaustiva
                            delay_calc();                                                                                               // Calcolo del ritardo prima di fare la richiesta
                            last_request_ = next_stat_->request(this);
                            communications();                                                                                           // Segnalazione di un treno alla stazione (avviene una sola volta)
                        if (last_request_ >= 0) {                                                                                       // Se la richiesta e' un binario
                                plat_num_ = last_request_;                                                                              // Mi salvo su che binario posso andare per la stampa
                                last_request_ = to_platform;                                                                            // Se mi restituisce un binario pongo 0 = to_platform
                            }
                    }
                } else {                                                                                                                  // Treno dentro i 5km prima (non pu� essere last_request = invalid)
                    if (is_slowing_ || last_request_ == reject) {                                                                       // Se e' stato rifiutato o sta rallentando un treno ca nel parcheggio
                        !reverse_ ? curr_km_ = next_stat_->distance() - 5 : curr_km_ = next_stat_->distance() + 5;                      // Appena sfora i +/-5km lo riporta indietro/avanti
                        curr_spd_ = 0;
                        is_slowing_ ? wait_count_ = 5 : wait_count_ = 0;                                                                // Se sta rallentando un treno attendo 5 minuti nel parcheggio
                        status_ = park;
                    } else {                                                                                                            // Se non sta rallentando nessuno
                        if (last_request_ == to_platform) {                                                                             //  e se deve andare al binario
                            curr_spd_ = 80 / 60.0;                                                                                      // Rallenta la velocit�
                            communications();                                                                                           //  e comunica l'arrivo
                        }                                                                                                               // Altrimenti e' entrato nei pressi della stazione ma deve transitare
                        status_ = stationMotion;
                    }
                }
            }
            break;

        case stationMotion:                                                                                                             // Movimento nei pressi della stazone
            !reverse_ ? curr_km_ += curr_spd_ : curr_km_ -= curr_spd_;                                                                  // Avanzo con la velocit� corrente (190 in caso di transito, 80 in caso di fermata)
            if (curr_stat_ != nullptr) {                                                                                                // Se sono appena partito dalla stazione precedente
                if ((!reverse_ && curr_km_ > curr_stat_->distance() + 5) || (reverse_ && curr_km_ < curr_stat_->distance() - 5)) {      //  e se sono dopo i 5km della stazione precedente posso andare al massimo
                    curr_stat_ = nullptr;
                    curr_spd_ = max_spd_;                                                                                               //  Posso andare alla massima velocit�
                    last_request_ = invalid;                                                                                            //  Metto l'ultima richiesta come invalida
                    status_ = normalMotion;                                                                                             //    per permettere la richiesta all'arrivo successivo
                }                                                                                                                       //  Altrimenti avanza ancora il giro successivo
            }
            else {                                                                                                                      // Altrimenti curr_stat_ == nullptr vuol dire che sono in arrivo (e non sto partendo)
                if (last_request_ == to_delete) {                                                                                       // Se sto arrivando alla fine della linea (capolinea o non)
                    if (curr_km_ >= next_stat_->distance()) {
                        curr_km_ = next_stat_->distance();
                        curr_spd_ = 0;
                        status_ = endReached;
                    }
                }
                else {
                    if (last_request_ == to_platform) {                                                                                     // Se posso andare ai binari
                        if ((!reverse_ && curr_km_ >= next_stat_->distance() || (reverse_ && curr_km_ <= next_stat_->distance()))) {        //  Se sono effettivamente arrivato ai binare
                            curr_stat_ = next_stat_;                                                                                        //  La stazione corrente e' esattamente quella a cui dovevo arrivare
                            curr_spd_ = 0;                                                                                                  //  Fermo il treno
                            curr_km_ = next_stat_->distance();                                                                              //  Allineo il treno alla stazione
                            wait_count_ = 5;                                                                                                //  Imposto il timer a 5 minuti per la salita o la discesa delle persone
                            status_ = platformStation;                                                                                      //  Imposto lo stato in binario
                            communications();                                                                                               //  Communicazione arrivo al binario
                        }
                    }                                                                                                                                   // Se deve transitare e ha superato la stazione la prossima stazione
                    if ((last_request_ == transit) && (!reverse_ && curr_km_ >= next_stat_->distance()) || (reverse_ && next_stat_->distance())) {      //  sar� la successiva di quella che ha appena superato
                        curr_stat_ = next_stat_;                                                                                                        // Rendo cur_stat_ diverso da nullptr perche' cos� il caso
                        !reverse_ ? next_stat_ = next_stat_->next_stat() : next_stat_ = next_stat_->prev_stat();                                        //  dopo viene gestito correttamente
                    }
                }
            }
            break;

        case platformStation:                                                                                                       // Binario nella stazione, sicuramente avr� una stazione successiva altrimenti sarebbe nello strato endReached
            if ((!reverse_ && curr_stat_->next_stat() != nullptr) || (reverse_ && curr_stat_->prev_stat() != nullptr)) {            // Se non sono al capolinea
                if (wait_count_ <= 0) {
                    delay_calc();
                    if (curr_stat_->request_exit(this)) {                                                                         // Se la richiesta per poter uscire dal binario viene accettata
                        curr_spd_ = 80 / 60.0;                                                                                      //  Imposto la velocita' limitata a 80km/h
                        !reverse_ ? next_stat_ = curr_stat_->next_stat() : next_stat_ = curr_stat_->prev_stat();                    //  Faccio avanzare il treno nella giusta direzione
                        status_ = stationMotion;                                                                                    //  Movimento nei pressi della staizone
                        if (time_arrival_next_stat_ < static_cast<int>(arrivals_.size()) - 1)
                            time_arrival_next_stat_++;                                                                              //  Aumento l'indice del riferimento dell'orario
                    }
                }
                else
                    wait_count_--;
            }
            else {
                status_ = endReached;                                                                                           //  vuol dire che e' al capolinea
                curr_spd_ = 0;
                delay_calc();
                communications();
            }
            break;

        case park:                                                                  // Parcheggio
            if (is_slowing_) {
                wait_count_ > 0 ? wait_count_-- : is_slowing_ = false;              // Se sto rallentando
            }
            else {
                delay_calc();
                last_request_ = next_stat_->request(this);
                if (last_request_ >= 0) last_request_ = to_platform;
                if (last_request_ != reject && last_request_ != invalid) {           // Nel caso di errori, con != invalid posso effettuare un'altra richiesta senza compromettere il sistema
                    if (last_request_ == to_platform) curr_spd_ = 80 / 60.0;
                    if (last_request_ == transit) curr_spd_ = 190 / 60.0;
                    status_ = stationMotion;
                    communications();
                }
            }
            break;

        case endReached:                                    // Fine corsa
            curr_stat_->delete_train(this);            // Dice alla stazione di eliminare il treno che ha finito la corsa
            break;

        default:
            throw std::logic_error("Invalid status in class Train\n");
    }
}

// ===== Calcolo del ritardo dei treni =====
void Train::delay_calc() {                                                                                              // Essendo nella classe Train questa funzione verr� fatta dai treni che possono saltare le fermate, Regional ha un caso a parte
    old_delay_ = delay_;                                                                                                // Ritardo di prima
    switch (status_) {
        case normalMotion:
            calc_specific_delay();
            break;

        case stationMotion:
            delay_ = central_railw_->curr_time() + (int)ceil(abs((int)ceil(next_stat()->distance() - curr_km_)) / (80 / 60.0)) - arrivals_[time_arrival_next_stat_];
            break;

        case platformStation:                                                                                           // Quanto e' in ritardo dal partire
            delay_ = central_railw_->curr_time() - (arrivals_[time_arrival_next_stat_] + 5);                            // Tempo corrente - tempo in cui dovevo arrivare
            if ((!reverse_ && curr_stat_->prev_stat() == nullptr) || (reverse_ && curr_stat_->next_stat() == nullptr))  // Se sono all'inizio della mia corsa (inversa o non)
                delay_ > 0 ? delay_ -= 5 : delay_ += 5;
            break;

        case park:
            calc_specific_delay();
            break;

        case endReached:
            delay_ = central_railw_->curr_time() - arrivals_[arrivals_.size() - 1];                                     // Entra qui solo se prima era al binario
            break;

        default:
            throw std::logic_error("Invalid status in calculation of delay (class Train)\n");
    }
}
void Train::calc_specific_delay() {
    int station_to_skip = 0;            // Numero di stazioni prima di trovare quella in cui il treno si fermer�
    int normal_motion_distance = 0;     // Distanza che devo percorrere a max_spd_ per andare da una stazione alla successiva (non considerando i 5 km prima e dopo)
    int delay_due_to_station = 0;
    Station* station_stop = next_stat_; // Riferimento alla prossima stazione (salvata nel treno! dopo station_stop punta alla stazione e quindi il reverse cambia il prev_stat() o il next_stat()!
    bool principal = false;             // Variabile per il ciclo while

    while (!principal) {                                                                                                     // Controllo finche' non trovo la stazione in cui dovr� fermarmi per calcolare il ritardo
        if (station_stop == nullptr) break;
        if (nullptr == dynamic_cast<Principal*>(station_stop)) {
            if (!reverse_) {
                if(station_stop ->next_stat() == nullptr) break;                                                            // Se l'attuale station_stop � il capolinea ma non � principale perch� � entrata nell'if riga 200 non ci sono capolinea
                normal_motion_distance += station_stop->next_stat()->distance() - station_stop->distance() - 25;            // Tutta la distanza tra una stazione e l'altra in cui posso andare a max_spd_
                station_stop = station_stop->next_stat();
            }
            else {
                normal_motion_distance += abs(station_stop->prev_stat()->distance() - station_stop->distance()) - 25;   // Tutta la distanza tra una stazione e l'altra in cui posso andare a max_spd_
                station_stop = station_stop->prev_stat();
            }
            station_to_skip++;
        }
        else
            principal = true;
    }
    /* Tempo mancante stimato per arrivare (in ordine):
     * Prima di arrivare ai -5km dalla stazione: tempo = spazio fino ai -5km dalla stazione / velocit� corrente(190km/h)
     * Per ogni stazione (-5km -> +5km) 10km percorsi a 190km/h (3.17km/min). Il tempo per transitare su ogni stazione = 10km / 3.17km/min = 3.16min a stazione
     * Da una stazione alla succesiva avr� la distanza delle due stazioni - 5km dopo la prima stazione e -20km prima della seconda stazione dove posso andare a max_spd_
     *
     * Invoco ceil al posto di mettere i valori interi arrotondati per eccesso per poter capire meglio in fase di revisione del codice
     */
    if (principal) {
        station_to_skip != 0 ? delay_due_to_station = station_to_skip * (int)ceil(3.16) : delay_due_to_station = (int)ceil(3.75);       // Trovo il tempo che mi serve per saltare tutte le stazioni dove transito
        delay_ = (central_railw_->curr_time() + (int)ceil((abs((int)ceil(next_stat_->distance() - curr_km_)) - 5) / curr_spd_) +
                  delay_due_to_station + (int)ceil(normal_motion_distance / max_spd_)) - arrivals_[time_arrival_next_stat_];
    }
    else delay_ = 0;                                                                                                                            // (Per i non regionali) Se le prossime stazioni sono tutte secondarie non posso calcolare nessun ritardo
}
void Regional::calc_specific_delay() {
    double time_five_km = 3.75;
    int distance_remaining = (int)(next_stat()->distance() - curr_km_) - 5;
    double time_distance_remaining;
    curr_spd_ != 0 ? time_distance_remaining = distance_remaining / curr_spd_ : time_distance_remaining = 0;
    int time_next  = arrivals_[time_arrival_next_stat_];
    delay_ = round(central_railw_->curr_time() + time_five_km + time_distance_remaining) - time_next;                   // Tempo in cui arriver� dopo errer giunto in stazione - tempo d'arrivo stimato
}
std::string Train::changed_delay() {                                                                                                // Calcolo se ci sono state variazioni nel ritardo
    std::string to_return = "Il treno numero " + train_num_ + " diretto verso la stazione " + next_stat_->station_name();
    // Se prima era in orario
    if (old_delay_ == 0) {
        if (delay_ > 0)
            return to_return += " non e' pi� in orario, il suo ritardo e' aumentato di " + print_delay(false) + " min\n";
        if (delay_ < 0)
            return to_return += " non e' pi� in orario, il suo anticipo e' aumentato di " + print_delay(false) + " min\n";
        return "";
        // in orario prima e dopo
    }

    // Se prima era in anticipo
    if (old_delay_ < 0) {
        if (delay_ < 0) {
            if (old_delay_ > delay_)
                return to_return += " ha aumentato il suo anticipo di " + print_delay(true) + " min\n";
            if (old_delay_ < delay_)
                return to_return += " ha diminuito il suo anticipo di " + print_delay(true) + " min\n";
            return "";
        }
        if (delay_ > 0)
            return to_return += " non e' pi� in anticipo, ma in ritardo di " + print_delay(false) + " min\n";
        return to_return += " non e' piu' in anticipo. Il treno e' in orario\n";
    }

    // Se prima era in ritardo
    if (old_delay_ > 0) {
        if (delay_ > 0) {
            if (old_delay_ < delay_)
                return to_return += " ha aumentato il suo ritardo di " + print_delay(true) + " min\n";
            if (old_delay_ > delay_)
                return to_return += " ha diminuito il suo ritardo di " + print_delay(true) + " min\n";
            return "";
        }
        if (delay_ < 0)
            return to_return += " non e' piu' in ritardo, ma in anticipo di " + print_delay(false) + " min\n";
        return to_return += " non e' piu' in anticipo. Il treno e' in orario\n";
    }
    throw std::logic_error("Error in changed_delay function, non return is called\n");
}

// ===== Communicazioni dei treni =====
void Train::communications() {                                                                                                                                  // Le comunicazioni variano in base allo stato
    string comm;
    switch (status_) {
        case normalMotion:                                                                                                                                      // Comunicazioni al 20esimo chilometro prima della prossima stazione in base alla risposta ricevuta dalla stazione
            comm = "Il treno " + get_train_type() + " n." + train_num_;
            switch (last_request_) {
                case reject:
                    comm += " ha ricevuto l'ordine dalla stazione di " + next_stat_->station_name() + " di poter andare al parcheggio\n";
                    break;
                case to_platform:
                    comm += " delle ore " + print_time();
                    if (delay_ != 0)
                        delay_ > 0 ? comm += " con un ritardo di " + print_delay(false) : comm += " con un anticipo di " + print_delay(false);
                    comm += " sta arrivando alla stazione di " + next_stat_->station_name() + " al binario n." + std::to_string(plat_num_) + "\n";
                    break;
                case transit:
                    comm += " ha ricevuto l'ordine dalla stazione di " + next_stat_->station_name() + " di poter transitare\n";
                    break;
                case invalid:                                                                                                                                               // Prima communicazione, quando il treno ha appena superato i 20km
                    comm += " e' in arrivo alla stazione di " + next_stat_->station_name();
                    if (delay_ != 0) {
                        delay_ > 0 ? comm += " con un ritardo di " + print_delay(false) : comm += " con un anticipo di " + print_delay(false);
                        comm += "\n";
                    }
                    break;
                case to_delete:
                    comm += " ha superato l'ultima stazione\n";
            }
            break;

        case stationMotion:                                                                                                                                                 // Movimento nei pressi della stazione
            if (last_request_ == to_platform)
                comm = changed_delay();
            break;

        case platformStation:                                                                                                                                               // Comunicazioni quando sono al binario
            comm = "Il treno " + get_train_type() + " n." + train_num_ + " delle ore " + print_time() + " e' arrivato alla stazione di " + curr_stat_->station_name();
            if (delay_ != 0) {
                if (delay_ < 0)
                    comm += " con un anticipo di " + print_delay(false);
                else
                    comm += " con un ritardo di " + print_delay(false);
            }
            comm += "\n";
            break;

        case park:
            // Non devo communicare niente se sono nel parcheggio
            break;

        case endReached:
            comm = "Il treno " + get_train_type() + " numero " + train_num_ + " e' giunto al capolinea";
            if (delay_ < 0)
                comm += " con un anticipo di " + print_delay(false) + " \n";
            if (delay_ > 0)
                comm += " con un ritardo di " + print_delay(false) + " \n";
            if (delay_ == 0)
                comm += " in orario\n";
            break;

        default:
            throw std::logic_error("Invalid status in communication\n");
    }
    central_railw_->append(comm);
    comm = "";
}


// ===== Funzioni get e set della classe train =====
std::string Train::train_num() const {              // Numero del treno
    return train_num_;
}
std::string Train::print_time()const {              // Stampa dell'orario in formato xx:xx
    char formatted_time[6];
    int hh = (arrivals_[time_arrival_next_stat_] / 60) % 24;
    int mm = arrivals_[time_arrival_next_stat_] % 60;
    int result = sprintf_s(formatted_time, 6, "%.2d:%.2d", hh, mm);
    return formatted_time;
}
std::string Train::print_delay(bool diff) const {               // Stampa del ritardo in formato hh ore mm minuti
    int delay_to_calculate = 0;
    if(diff)
        delay_to_calculate = abs(delay_ - old_delay_);
    else
        delay_to_calculate = abs(delay_);

    char formatted_time[15];
    int hh = (delay_to_calculate / 60) % 24;
    int mm = delay_to_calculate % 60;
    if(hh != 0)
        int result = sprintf_s(formatted_time, 15, "%.2d ore %.2d min", hh, mm);
    else
        int result = sprintf_s(formatted_time, 7, "%.2d min", mm);
    return formatted_time;
}
bool Train::is_slowing() const {             // Ritorna se il treno sta rallentando qualcuno
    return is_slowing_;
}
void Train::set_slowing(bool is_slowing) {   // Impostare il caso in cui il treno stia rallentando qualcuno
    is_slowing_ = is_slowing;
}
bool Train::reverse() const {                        // Andata (false) /ritorno (true)
    return reverse_;
}
double Train::max_spd() const {                      // Massima velocit� del treno, possibile controllo del tipo di treno
    return max_spd_;
}
double Train::curr_spd() const {                     // Velocit� corrente: solitamente massima, altrimenti velocit� del treno davanti, altrimenti gli 80km/h
    return curr_spd_;
}
void Train::set_curr_spd(double val) {              // Imposta velocit� corrente
    curr_spd_ = val;
}
double Train::current_km() const {                   // Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    return curr_km_;
}
Station* Train::curr_stat() const {                  // Ritorna il puntatore alla stazione dove risiede
    return curr_stat_;
}
Station* Train::next_stat() const {                  // Ritorna il puntatore alla prossima stazione
    return next_stat_;
}
const std::vector<int>& Train::arrivals() const {    // Ritorna il riferimento al vettore arrivals_ (non e' detto che serva)
    return arrivals_;
}
int Train::delay() const {                           // Ritorna l'eventuale anticipo/ritardo
    return delay_;
}
int Train::wait_count() const {                      // Ritorna il countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    return wait_count_;
}
void Train::set_wait_count(int min) {                // Imposta il countdown d'attesa (in qualsiasi situazione)
    wait_count_ = min;
}
int Train::status() const {                          // Ritorna lo stato del treno (0 movimento normale, 1 movimento nei pressi della stazione, 2 nel binario, 3 nel parcheggio, 4 capolinea)
    return status_;
}
void Train::set_status(int status) {
    status_ = status;                               // Imposta lo stato del treno
}



// ===== CLASSE REGIONAL =====
Regional::Regional(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail) : Train(number, rev, 160.0 / 60, curr, times, rail) {}
std::string Regional::get_train_type()const {
    return "regionale";
}

// ===== CLASSE FAST =====
Fast::Fast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail) :Train(number, rev, 240.0 / 60, curr, times, rail) {}
std::string Fast::get_train_type()const {
    return "veloce";
}

// ===== CLASSE SUPERFAST =====
SuperFast::SuperFast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail) :Train(number, rev, 300.0 / 60, curr, times, rail) {}
std::string SuperFast::get_train_type()const {
    return "alta velocita'";
}