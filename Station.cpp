#include "Railway.h"
// ===== STAZIONI ===== DIEGO SPINOSA
/*class Station{
public:
    virtual int Station::request(Train* t);
    bool request_exit(Train* t) = 0;		    // (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
    virtual ~Station();
*/

constexpr double DEPARTURE_DELAY = 5;
constexpr double TRANSIT_DELAY = 7;
constexpr double PARKING_STATUS = 3;

Station::Station(std::string name, int distance, Station* prev, Railway* rail) : station_name_{name}, distance_{distance}, prev_stat_{prev}, next_stat_{nullptr}, haltTimer{0}, central_railw_{rail}, platforms(2,nullptr), platforms_reverse(2,nullptr)  {
    //NEL COSTRUTTORE IMPOSTO LA next_stat_ DELLA PRECEDENTE STAZIONE:
    if(prev != nullptr) prev->next_stat_ = this; //si concatena la stazione alla precedente. Possibile, ovviamente, solo se questa non sia la prima della linea.
    //tutte le variabili sono inizializzate
    //controlli (supplementari):
    if(name == "" || distance < 0 || rail == nullptr) throw std::invalid_argument("\n!!! TENTATA COSTRUZIONE STAZIONE CON PARAMETRI INVALIDI !!!\n");
}

Station::~Station(){};
Principal::~Principal(){};
Secondary::~Secondary(){};

std::string Station::station_name() const{ return station_name_; }		// Restituisce il nome della stazione

int Station::distance() const{ return distance_; }						// Restituisce la distanza della stazione dall'origine

int Station::getHaltTimer() const{ return haltTimer; }				// Restituisce il numero di minuti trascorsi dall'ultima partenza della stazione

Station* Station::next_stat() const{ return next_stat_; }				// Restituisce il puntatore alla prossima stazione

Station* Station::prev_stat() const{ return prev_stat_; }				// Restituisce il puntatore alla stazione precedente (usato per i treni reverse)

std::string Station::update(){
    if(haltTimer>0) haltTimer--;                //Avanzata temporale in stazione: decrementa tempo
    std::string tmp = announcements;
    announcements = "";
    return tmp;
}

///METODI "personali", usati solo internamente (liv. prot. comunque protected)

//PER MECCANISMO PRIORITA' SI VEDA priorità.txt
double Station::getPriority(Train* t) const{  //METODO PRIVATO: calcola priorità treni
    //return int(t->max_spd()/100.0) + t->delay()/10000.0;
    if(dynamic_cast<Regional*>(t)) return 1.0 + t->delay()/10000.0;
    if(dynamic_cast<Fast*>(t)) return 2.0 + t->delay()/10000.0;
    if(dynamic_cast<SuperFast*>(t)) return 3.0 + t->delay()/10000.0;
    else throw std::invalid_argument("\ngetPriority: CLASS CAST INVALIDO\n");
}

//Ritorna la massima priorità parcheggiata: NON conto treni in anticipo! (salvo solo priorità positive)
//Torna 0 con parcheggio vuoto O se il parcheggio contiene TUTTI treni in anticipo
double Station::getMaxPP() const{
    double maxp = 0;
    for(std::vector<Train*>::const_iterator pos = park_.cbegin(); pos != park_.cend() ; pos++){
        double p = getPriority(*pos);
        if(p > maxp) maxp = p;
    }
    return maxp;
}
double Station::getMaxPPR() const{
    double maxp = 0;
    for(std::vector<Train*>::const_iterator pos = park_reverse_.cbegin(); pos != park_reverse_.cend() ; pos++){
        double p = getPriority(*pos);
        if(p > maxp) maxp = p;
    }
    return maxp;
}

bool Station::busy() const{     //METODO PRIVATO: Dice se la stazione è piena ==> nessun binario disponibile
    if(platforms[0] != nullptr && platforms[1] != nullptr ) return true;
    else return false;
}
bool Station::busyR() const{
    if(platforms_reverse[0] != nullptr && platforms_reverse[1] != nullptr ) return true;
    else return false;
}

bool Station::request_exit(Train* t){      // (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
    Train* otherTrain;
    int thisTrainIndex;
    if(!t->reverse()){  //se treno va dritto
        //identifico l'altro treno in banchina e l'indice del treno che chiede d'uscire:
        if(platforms[0] == t){
            otherTrain = platforms[1];
            thisTrainIndex = 0;
        }
        else{
            otherTrain = platforms[0];
            thisTrainIndex = 1;
        }
        //se è stazione capolinea: elimina treno
        /// !!! DA IMPLEMENTARE SECONDO METODO RAILWAY !!!
        //non esce se timer>0 o se altro treno ha precedenza
        if(haltTimer>0 || getPriority(t) < getPriority(otherTrain) ) return false;
        else{   //se può uscire: imposta timer, libera binario, dai via libera
            haltTimer = DEPARTURE_DELAY;
            platforms[thisTrainIndex] = nullptr;
            announcements = announcements + "Il treno "+ t->train_num() +" è in partenza dalla stazione "+ station_name() +" dal binario "+ std::to_string(thisTrainIndex+1) +"\n";
            return true;
        }
    }
    else{               //se treno ritorna
        //identifico l'altro treno in banchina e l'indice del treno che chiede d'uscire:
        if(platforms_reverse[0] == t){
            otherTrain = platforms_reverse[1];
            thisTrainIndex = 0;
        }
        else{
            otherTrain = platforms_reverse[0];
            thisTrainIndex = 1;
        }
        //se è stazione capolinea: elimina treno
        /// !!! DA IMPLEMENTARE SECONDO METODO RAILWAY !!!
        //non esce se timer>0 o se altro treno ha precedenza
        if(haltTimerR>0 || getPriority(t) < getPriority(otherTrain) ) return false;
        else{   //se può uscire: imposta timer, libera binario, dai via libera
            haltTimerR = DEPARTURE_DELAY;
            platforms_reverse[thisTrainIndex] = nullptr;
            announcements = announcements + "Il treno "+ t->train_num() +" è in partenza dalla stazione "+ station_name() +" dal binario "+ std::to_string(thisTrainIndex+1) +"\n";
            return true;
        }
    }

}

//rimuove il treno t, se presente, dal parcheggio
void Station::removeParking(Train* t){
    for(std::vector<Train*>::iterator pos = park_.begin(); pos < park_.end(); pos++){
        if(*pos == t){
            park_.erase(pos);
            return;
        }
    }
}

int Station::assignPlatform(Train* t){
    //Se la stazione è piena O c'è un altro treno in attesa con priorità maggiore O treno è in anticipo (priorità negativa) O il treno è rallentatore
    if( busy() || getPriority(t) < getMaxPP() || t->is_slowing() ){
        if(t->status() != PARKING_STATUS){
            park_.push_back(t);           ///Il treno viene messo in parcheggio SOLO se non lo è già!
            announcements = announcements + "Treno "+ t->train_num() +" andrà al parcheggio\n";
        }
        return -1;
    }
    else{   //se c'è spazio e il treno ha diritto ad usarlo
        int i;
        //trova indice binario libero
        if(platforms[0] == nullptr) i=0;
        else i=1;
        platforms[i] = t;
        //!Se il treno era in parcheggio, devo rimuoverlo:
        if(t->status() == PARKING_STATUS) removeParking(t);
        announcements = announcements + "Treno "+ t->train_num() +" in arrivo al binario "+ std::to_string(i) +"\n";
        return i;
    }
}

int Station::assignPlatformR(Train* t){
    //Se la stazione è piena O c'è un altro treno in attesa con priorità maggiore O treno è in anticipo (priorità negativa) O il treno è rallentatore
    if( busyR() || getPriority(t) < getMaxPPR() || t->is_slowing() ){
        if(t->status() != PARKING_STATUS){
            park_reverse_.push_back(t);   ///Il treno viene messo in parcheggio SOLO se non lo è già!
            announcements = announcements + "Treno "+ t->train_num() +" andrà al parcheggio\n";
        }
        return -1;
    }
    else{   //se c'è spazio e il treno ha diritto ad usarlo
        int i;
        //trova indice binario libero
        if(platforms_reverse[0] == nullptr) i=0;
        else i=1;
        platforms_reverse[i] = t;
        //!Se il treno era in parcheggio, devo rimuoverlo:
        if(t->status() == PARKING_STATUS) removeParkingR(t);
        announcements = announcements + "Treno "+ t->train_num() +" in arrivo al binario "+ std::to_string(i) +"\n";
        return i;
    }
}

void Station::removeParkingR(Train* t){
    for(std::vector<Train*>::iterator pos = park_reverse_.begin(); pos < park_reverse_.end(); pos++){
        if(*pos == t){
            park_reverse_.erase(pos);
            return;
        }
    }
}

//METODI CLASSI DERIVATE: Principal

//IN UNA STAZIONE PRINCIPAL nessun treno transita! Tutti i treni devono fermarsi
int Principal::request(Train* t){
    /// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    /// IN UNA STAZIONE PRINCIPALE !!! N O N !!! È PREVISTO TRANISTO
    //  L'assegnazione binario è prevista SE E SOLO SE è istantaneamente previsto un binario libero
    //  In tutte le stazioni son previsti DUE binari per senso di marcia!
    //  SEQUENZA: Richiesta -> Verifica binario libero -> Verifica PRIORITA' (tipo treno poi ritardo) -> Eventuale assegnazione.
    ///FUNZIONE DEVE VERIFICARE SENSO DI PERCORRENZA DEL TRENO: FALSE andata, TRUE return
    if(!t->reverse()) return assignPlatform(t);     //Se il treno è in ANDATA
    else return assignPlatformR(t);                 //Se il treno è in RITORNO (reverse)
}

Principal::Principal(std::string name, int distance, Station* prev, Railway* rail) : Station(name, distance, prev, rail) {}
//la costruzione NON differisce in alcun parametro

//METODI CLASSI DERIVATE: Secondary

Secondary::Secondary(std::string name, int distance, Station* prev, Railway* rail) : Station(name, distance, prev, rail) {}

int Secondary::request(Train* t){
    /// (Interazione con stazione) -2 deve transitare, -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    //  L'assegnazione binario è prevista SE E SOLO SE è istantaneamente previsto un binario libero
    //  In tutte le stazioni son previsti DUE binari per senso di marcia!
    //  SEQUENZA: Richiesta -> Verifica binario libero -> Verifica PRIORITA' (tipo treno poi ritardo) -> Eventuale assegnazione.
    ///FUNZIONE DEVE VERIFICARE SENSO DI PERCORRENZA DEL TRENO: FALSE andata, TRUE return
    if(!t->reverse()){  //Se il treno è in ANDATA
        //Se non è regionale: TRANSITO
        if(!dynamic_cast<Regional*>(t)){
            haltTimer = TRANSIT_DELAY;
            announcements = announcements + "Treno "+ t->train_num() +" in transito\n";
            return -2;
        }
        //Se invece è treno regionale: comportamento normale
        else return assignPlatform(t);
    }
    else{    //! Se il treno è in RITORNO (reverse)
        //Se non è regionale: TRANSITO
        if(!dynamic_cast<Regional*>(t)){
            haltTimerR = TRANSIT_DELAY;
            announcements = announcements + "Treno "+ t->train_num() +" in transito\n";
            return -2;
        }
        //Se invece è treno regionale: comportamento normale
        else return assignPlatformR(t);
    }
}

