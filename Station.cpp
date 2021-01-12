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

Station::Station(std::string name, int distance, Station* prev, Railway* rail) : station_name_{ name }, distance_{ distance }, prev_stat_{ prev }, next_stat_{ nullptr }, haltTimer{ 0 }, central_railw_{ rail }, platforms(2, nullptr), platforms_reverse(2, nullptr)  {
	//NEL COSTRUTTORE IMPOSTO LA next_stat_ DELLA PRECEDENTE STAZIONE:
	if (prev != nullptr) prev->next_stat_ = this; //si concatena la stazione alla precedente. Possibile, ovviamente, solo se questa non sia la prima della linea.
	//tutte le variabili sono inizializzate
	//controlli (supplementari):
	if (name == "" || distance < 0 || rail == nullptr) throw std::invalid_argument("\n!!! TENTATA COSTRUZIONE STAZIONE CON PARAMETRI INVALIDI !!!\n");
}

//Station::~Station() {};
//Principal::~Principal() {};
//Secondary::~Secondary() {};

std::string Station::station_name() const { return station_name_; }		// Restituisce il nome della stazione

int Station::distance() const { return distance_; }						// Restituisce la distanza della stazione dall'origine

int Station::getHaltTimer() const { return haltTimer; }				// Restituisce il numero di minuti trascorsi dall'ultima partenza della stazione

Station* Station::next_stat() const { return next_stat_; }				// Restituisce il puntatore alla prossima stazione

Station* Station::prev_stat() const { return prev_stat_; }				// Restituisce il puntatore alla stazione precedente (usato per i treni reverse)

void Station::update() {
	if (haltTimer > 0) haltTimer--;                //Avanzata temporale in stazione: decrementa tempo
	std::string tmp = announcements;
	announcements = "";
	central_railw_->append(tmp);
}

///METODI "personali", usati solo internamente (liv. prot. comunque protected)

//PER MECCANISMO PRIORITA' SI VEDA priorità.txt
double Station::getPriority(Train* t) const {  //METODO PRIVATO: calcola priorità treni
	//return int(t->max_spd()/100.0) + t->delay()/10000.0;
	if (t == nullptr) return 0;
	int trainDelay = t->delay();
	if (trainDelay < 0) return -1;
	if (Regional* tr = dynamic_cast<Regional*>(t)) return 1.0 + trainDelay / 10000.0;
	if (Fast* tr = dynamic_cast<Fast*>(t)) return 2.0 + trainDelay / 10000.0;
	if (SuperFast* tr = dynamic_cast<SuperFast*>(t)) return 3.0 + trainDelay / 10000.0;
	else throw std::invalid_argument("\ngetPriority: CLASS CAST INVALIDO\n");
}

//Ritorna la massima priorità parcheggiata: NON conto treni in anticipo! (salvo solo priorità positive)
//Torna 0 con parcheggio vuoto O se il parcheggio contiene TUTTI treni in anticipo
double Station::getMaxPP() const {
	double maxp = 0;
	for (std::vector<Train*>::const_iterator pos = park_.cbegin(); pos != park_.cend(); pos++) {
		double p = getPriority(*pos);
		if (p > maxp) maxp = p;
	}
	return maxp;
}
double Station::getMaxPPR() const {
	double maxp = 0;
	for (std::vector<Train*>::const_iterator pos = park_reverse_.cbegin(); pos != park_reverse_.cend(); pos++) {
		double p = getPriority(*pos);
		if (p > maxp) maxp = p;
	}
	return maxp;
}

bool Station::busy() const {     //METODO PRIVATO: Dice se la stazione e' piena ==> nessun binario disponibile
	if (platforms[0] != nullptr && platforms[1] != nullptr) return true;
	else return false;
}
bool Station::busyR() const {
	if (platforms_reverse[0] != nullptr && platforms_reverse[1] != nullptr) return true;
	else return false;
}

bool Station::request_exit(Train* t) {      // (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
	//Questo metodo request_exit viene invocato dal treno SOLO quando il
	Train* otherTrain;
	int thisTrainIndex;
	if (!t->reverse()) {  //se treno va dritto
		//identifico l'altro treno in banchina e l'indice del treno che chiede d'uscire:
		if (platforms[0] == t) {
			otherTrain = platforms[1];
			thisTrainIndex = 0;
		}
		else {
			otherTrain = platforms[0];
			thisTrainIndex = 1;
		}
		//non esce se timer>0 o se altro treno ha precedenza
		if (haltTimer > 0 || getPriority(t) < getPriority(otherTrain)) return false;
		else {   //se può uscire: imposta timer, libera binario, dai via libera
			haltTimer = DEPARTURE_DELAY;
			platforms[thisTrainIndex] = nullptr;
			announcements = announcements + "Il treno " + t->train_num() + " e' in partenza dalla stazione " + station_name() + " dal binario " + std::to_string(thisTrainIndex + 1) + "\n";
			return true;
		}
	}
	else {               //se treno ritorna
		//identifico l'altro treno in banchina e l'indice del treno che chiede d'uscire:
		if (platforms_reverse[0] == t) {
			otherTrain = platforms_reverse[1];
			thisTrainIndex = 0;
		}
		else {
			otherTrain = platforms_reverse[0];
			thisTrainIndex = 1;
		}
		//non esce se timer>0 o se altro treno ha precedenza
		if (haltTimerR > 0 || getPriority(t) < getPriority(otherTrain)) return false;
		else {   //se può uscire: imposta timer, libera binario, dai via libera
			haltTimerR = DEPARTURE_DELAY;
			platforms_reverse[thisTrainIndex] = nullptr;
			announcements = announcements + "Il treno " + t->train_num() + " e' in partenza dalla stazione " + station_name() + " dal binario " + std::to_string(thisTrainIndex + 1) + "\n";
			return true;
		}
	}

}

//rimuove il treno t, se presente, dal parcheggio
void Station::removeParking(Train* t) {
	for (std::vector<Train*>::iterator pos = park_.begin(); pos < park_.end(); pos++) {
		if (*pos == t) {
			park_.erase(pos);
			return;
		}
	}
}

int Station::assignPlatform(Train* t) {
	//Se la stazione e' piena O c'e' un altro treno in attesa con priorità maggiore O treno e' in anticipo (priorità negativa) O il treno e' rallentatore
	if (busy() || getPriority(t) < getMaxPP() || t->is_slowing()) {
		if (t->status() != PARKING_STATUS) {
			park_.push_back(t);           ///Il treno viene messo in parcheggio SOLO se non lo e' già!
			announcements = announcements + "Treno " + t->train_num() + " andra' al parcheggio\n";
		}
		return -1;
	}
	else {   //se c'e' spazio e il treno ha diritto ad usarlo
		int i;
		//trova indice binario libero
		if (platforms[0] == nullptr) i = 0;
		else i = 1;
		platforms[i] = t;
		//!Se il treno era in parcheggio, devo rimuoverlo:
		if (t->status() == PARKING_STATUS) removeParking(t);
		announcements = announcements + "Treno " + t->train_num() + " in arrivo al binario " + std::to_string(i) + "\n";
		return i;
	}
}

int Station::assignPlatformR(Train* t) {
	//Se la stazione e' piena O c'e' un altro treno in attesa con priorità maggiore O treno e' in anticipo (priorità negativa) O il treno e' rallentatore
	if (busyR() || getPriority(t) < getMaxPPR() || t->is_slowing()) {
		if (t->status() != PARKING_STATUS) {
			park_reverse_.push_back(t);   ///Il treno viene messo in parcheggio SOLO se non lo e' già!
			announcements = announcements + "Treno " + t->train_num() + " andra' al parcheggio\n";
		}
		return -1;
	}
	else {   //se c'e' spazio e il treno ha diritto ad usarlo
		int i;
		//trova indice binario libero
		if (platforms_reverse[0] == nullptr) i = 0;
		else i = 1;
		platforms_reverse[i] = t;
		//!Se il treno era in parcheggio, devo rimuoverlo:
		if (t->status() == PARKING_STATUS) removeParkingR(t);
		announcements = announcements + "Treno " + t->train_num() + " in arrivo al binario " + std::to_string(i) + "\n";
		return i;
	}
}

void Station::removeParkingR(Train* t) {
	for (std::vector<Train*>::iterator pos = park_reverse_.begin(); pos < park_reverse_.end(); pos++) {
		if (*pos == t) {
			park_reverse_.erase(pos);
			return;
		}
	}
}

void Station::delete_train(Train* t) {
	//Se il treno e' in stazione (quindi il suo puntatore curr_stat e' diverso da nullptr): il treno t va cancellato dalla stazione
	if (t->curr_stat() != nullptr) {
		if (!t->reverse()) {      //se treno va dritto
			if (platforms[0] == t) platforms[0] = nullptr;
			else platforms[1] = nullptr;
		}
		else {                   //se treno e' in ritorno
			if (platforms_reverse[0] == t) platforms_reverse[0] = nullptr;
			else platforms_reverse[1] = nullptr;
		}
	}
	//Seguente parte va eseguita in ogni caso, anche se il treno non si trova in stazione
	announcements = announcements + "Il treno " + t->train_num() + " ha terminato la sua corsa.\n";
}
//METODI CLASSI DERIVATE: Principal

Principal::Principal(std::string name, int distance, Station* prev, Railway* rail) : Station(name, distance, prev, rail) {}
//la costruzione NON differisce in alcun parametro

//IN UNA STAZIONE PRINCIPAL nessun treno transita! Tutti i treni devono fermarsi
int Principal::request(Train* t) {
	/// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
	/// IN UNA STAZIONE PRINCIPALE !!! N O N !!! e' PREVISTO TRANISTO
	//  L'assegnazione binario e' prevista SE E SOLO SE e' istantaneamente previsto un binario libero
	//  In tutte le stazioni son previsti DUE binari per senso di marcia!
	//  SEQUENZA: Richiesta -> Verifica binario libero -> Verifica PRIORITA' (tipo treno poi ritardo) -> Eventuale assegnazione.
	///FUNZIONE DEVE VERIFICARE SENSO DI PERCORRENZA DEL TRENO: FALSE andata, TRUE return
	if (!t->reverse()) return assignPlatform(t);     //Se il treno e' in ANDATA
	else return assignPlatformR(t);                 //Se il treno e' in RITORNO (reverse)
}

//METODI CLASSI DERIVATE: Secondary

Secondary::Secondary(std::string name, int distance, Station* prev, Railway* rail) : Station(name, distance, prev, rail) {}

int Secondary::request(Train* t) {
	/// (Interazione con stazione) -3 transito del capolinea, -2 deve transitare, -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
	//  L'assegnazione binario e' prevista SE E SOLO SE e' istantaneamente previsto un binario libero
	//  In tutte le stazioni son previsti DUE binari per senso di marcia!
	//  SEQUENZA: Richiesta -> Verifica binario libero -> Verifica PRIORITA' (tipo treno poi ritardo) -> Eventuale assegnazione.
	///FUNZIONE DEVE VERIFICARE SENSO DI PERCORRENZA DEL TRENO: FALSE andata, TRUE return
	if (!t->reverse()) {  //Se il treno e' in ANDATA
		//Se non e' regionale: TRANSITO
		if (!dynamic_cast<Regional*>(t)) {
			if (next_stat_ == nullptr) return -3;     //CASO LIMITE: TRANSITO DEL CAPOLINEA
			haltTimer = TRANSIT_DELAY;
			announcements = announcements + "Treno " + t->train_num() + " in transito\n";
			return -2;
		}
		//Se invece e' treno regionale: comportamento normale
		else return assignPlatform(t);
	}
	else {    //! Se il treno e' in RITORNO (reverse)
		//Se non e' regionale: TRANSITO
		if (!dynamic_cast<Regional*>(t)) {
			if (prev_stat_ == nullptr) return -3;     //CASO LIMITE: TRANSITO DEL CAPOLINEA
			haltTimerR = TRANSIT_DELAY;
			announcements = announcements + "Treno " + t->train_num() + " in transito\n";
			return -2;
		}
		//Se invece e' treno regionale: comportamento normale
		else return assignPlatformR(t);
	}
}
