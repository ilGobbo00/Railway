// Header

/*
- Il sistema centrale si accorge delle collisioni perchè è lui che ha accesso a tutti i treni e va a modificare la velocità del treno da rallentare a meno che uno dei due non is trovi in prossimità
   della stazione (zona lenta). Il treno che causa il ritardo verrà impostato nello stato "Rallentamento"  con cui andrà obbligatoriamente a sostare 5 min in parcheggio per far avvenire sorpasso.
- Se il treno (che deve caricare/scaricare passeggeri) durante l'andatura di crocera supera la piazzola di sosta nel minuto il treno verrà riportato alla piazzola per poi riprendere a 80 km/h nel minuto successivo
- Le partenze dei treni vengono svolte prima del passaggio dei treni dal parcheggio alla stazione
- Prima di far andare un treno in arrivo (prima dei 5km) in un binario libero devo assicurarmi che non ci sia un treno della piazzola che stia uscendo per andare su un binario libero: gestito dall'assegnazione dei binari, che viene effettuata al -20° km e successivamente al -5°
- I treni vanno sempre a cannone, l'anticipo è compensato aspettando in piazzola
- Il criterio per far partire i treni in piazzola è tipo di treno e poi il ritardo
- Ai 20km si effettua l'assegnazione del binario valutando l'occupazione istantanea della stazione. In caso non fosse disponibile alcun binario O ci fosse in parcheggio un treno con priorità maggiore O il treno fosse in anticipo
   il treno verrà fatto andare in parcheggio. (casistica gestista dal metodo request() della stazione chiamato dal treno)
- Ai 20km i treni transito verificano che la stazione a cui si avvicinano abbia il binario transito: in caso questo fosse presente, la stazione deve impedire la partenza di treni passeggeri per 7 minuti. Se bin transito non è presente, non è necessario.
- Dalle stazioni i treni partono differenziati di almeno 5 min, per garantire la distanza fra i treni.
- Per calcolare il ritardo: ogni treno quando arriva al parcheggio o alla banchina fa la sottrazione tra il tempo in cui doveva arrivare e quello effettivo del sistema
- Ai cambi di velocità (ai 5km) si taglia il minuto (si approssima al km intero dove viene cambiata la velocità)
- Nessun controllo sulla distanza tra treni che sono compresi nel parcheggio
*/

#ifndef Railway_h
#define Railway_h


#include <iostream>
#include <vector>
#include <string>

class Railway;
class Station;
class Train;

// ===== SISTEMA CENTRALE ===== GIULIO REBECCHI
class Railway{
public:
    Railway(const std::string line_description, const std::string timetables);	// Crea i vettori stations_ e trains_
    Railway(const Railway& r) = delete;					                        //
    Railway& operator=(const Railway& r) = delete;		                        // Construttori e assegnamenti
    Railway(Railway&& r) = delete;										        // disattivati
    Railway& operator=(Railway&& r) = delete;					                //

    std::string advance_time();					// Fa avanzare il tempo e restituisce una concatenazione di stringhe che indica ciò che è successo nel minuto precedente
    bool is_completed();					 	// Controllo se tutti i treni sono arrivati a destinazione finale (se si termina il programma) (tutti status 4, e tutti con next_stat_ = nullptr)
    std::string check_interaction();		    // Controllo distanza (collisioni e sorpasso)
    int curr_time();							// Ritorna il tempo corrente
    void append(std::string msg);               // Funzione per aggiungere contenuto in coda alla stringa di output della Railway
    std::string printout();                     // Funzione di stampa della stringa di output messages_ (svuota la stringa dopo averla restituita)

    ~Railway();
private:
    int curr_time_;							    // Tempo corrente
    std::vector<Station> stations_;		    // Vettore che contiene tutte le stazioni in sequenza line_description.txt
    std::vector<Train> trains_;			    // Vettore che contiene tutti i teni presenti in timetables.txt
    std::string messages_;                      // Stringa per l'output a console
};

// ===== STAZIONI ===== DIEGO SPINOSA
class Station{
public:
    Station(const Station& s) = delete;				//
    Station& operator=(const Station& s) = delete;	// Construttori e assegnamenti
    Station(Station&& s) = delete;					   // disattivati
    Station& operator=(Station&& s) = delete;		//

    std::string station_name() const;				   // Restituisce il nome della stazione
    std::string announcements;                     // Stringa per comunicazioni
    int distance() const;							      // Restituisce la distanza della stazione dall'origine
    int getHaltTimer() const;						      // Restituisce il numero di minuti trascorsi dall'ultima partenza della stazione
    Station* next_stat() const;						   // Restituisce il puntatore alla prossima stazione
    Station* prev_stat() const;						   // Restituisce il puntatore alla stazione precedente (usato per i treni reverse)

    virtual int request(Train* t) = 0; 			   // (Interazione con stazione) -2: transito, -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    bool request_exit(Train* t);		               // (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
    std::string update();                          // Metodo con cui "far passare il tempo" in stazione. Verrà invocato da Railway nella parte inziale d'ogni minuto, PRIMA di verifiche varie/avanzamento treni. Riporta aggiornamenti.
    void deleteTrain(Train* t);                    // Chiamato da un treno al capolinea per essere rimosso dai binari

    virtual ~Station();

protected:
    Station(std::string name, int distance, Station* prev, Railway* rail);

    std::string station_name_;						// Nome della stazione
    std::vector<Train*> platforms;					// Binari passeggeri e transito andata LUNGHEZZA FISSA DEFINITA IN COSTRUTTORE
    std::vector<Train*> platforms_reverse;		// Binari passeggeri e transito ritorno LUNGHEZZA FISSA DEFINITA IN COSTRUTTORE
    std::vector<Train*> park_;						// (Binari) Parcheggio andata
    std::vector<Train*> park_reverse_;				// (Binari) Parcheggio ritorno
    int distance_;									   // Distanza dalla stazione primaria
    Station* next_stat_;							   // Punta alla stazione sucessiva per poterlo comunicare ai treni che dovranno partire
    Station* prev_stat_;							   // Punta alla precedente per il reverse
    int haltTimer;								      // Minuti di fermo stazione (caricato da partenze o transiti, quando >0 ferma partenze)
    int haltTimerR;								      // Minuti di fermo stazione (caricato da partenze o transiti, quando >0 ferma partenze) ritorno
    Railway* central_railw_; 						   // Per avere il tempo corrente mi serve il riferimento al rail
   
    double getPriority(Train* t) const;         // Restituisce priorità treno
  	 bool busy() const;                          // Se stazione è piena in senso andata
  	 bool busyR() const;                         // Se stazione è piena in senso ritorno
    double getMaxPP() const;                    // Ricava la priorità massima fra i treni in parcheggio
    double getMaxPPR() const;                   // Ricava la priorità massima fra i treni in parcheggio (ritorno)
    void removeParking(Train* t);               // Rimuove treno dal parcheggio (se presente)
    void removeParkingR(Train* t);              // Rimuove treno dal parcheggio (se presente) ritorno
    int assignPlatform(Train* t);               // Funzione ausiliaria per assegnazione binari
    int assignPlatformR(Train* t);              // Funzione ausiliaria per assegnazione binari (ritorno)
};

class Principal : public Station{
public:
    Principal(std::string name, int distance, Station* prev, Railway* rail);
    int request(Train* t); 							// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    ~Principal();
};

class Secondary : public Station{					// Il binario di transito è dato dal diverso comportamento delle stazioni di tipo Principal e Secondary
public:                                         // (perchè nel secondo caso lasciamo passare il treno transito facendo aspettare gli altri)
    Secondary(std::string name, int distance, Station* prev, Railway* rail);
    int request(Train* t); 							// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    ~Secondary();
};




// ===== TRENI ===== RICCARDO GOBBO
class Train{
public:
    Train(const Train& t) = delete;				//
    Train& operator=(const Train& t) = delete;	// Construttori e assegnamenti
    Train(Train&& t) = delete;					// disattivati
    Train& operator=(Train&& t) = delete;		//

    std::string train_num() const;				// Numero del treno
    bool reverse() const;						// Andata (false) /ritorno (true)
    double max_spd() const;				        // Massima velocità del treno, possibile controllo del tipo di treno
    double curr_spd() const;					// Velocità corrente: solitamente massima, altrimenti velocità del treno davanti, altrimenti gli 80km/h
    void set_curr_spd(double val);				// Imposta velocità corrente
    double current_km() const;					// Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    void advance_train();				        // Calcolo del km al prossimo minuto. Aggiorna stato
    Station* curr_stat() const;					// Ritorna il puntatore alla stazione dove risiede
    Station* next_stat() const;					// Ritorna il puntatore alla prossima stazione
    const std::vector<int> & arrivals() const;	// Ritorna il riferimento al vettore arrivals_ (non è detto che serva)
    int delay() const;							// Ritorna l'eventuale anticipo/ritardo
    int wait_count() const;						// Ritorna il countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    void set_wait_count(int min);				// Imposta il countdown d'attesa (in qualsiasi situazione)
    int status() const;							// Ritorna lo stato del treno (0 movimento normale, 1 movimento nei pressi della stazione, 2 nel binario, 3 nel parcheggio, 4 capolinea)
    void set_status(int status);				// Imposta lo stato del treno
    bool is_slowing() const;                    // Ritorna se il treno sta rallentando qualcuno
    void set_slowing(bool is_slowing);          // Impostare il caso in cui il treno stia rallentando qualcuno
    virtual ~Train();

protected:
    Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);

    virtual void delay_calc();
    virtual void communications();				// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione
    virtual void calc_specific_delay();         // Funzione interna invocata quando bisogna calcolare il ritardo in base al tipo di treno
    int changed_delay();
    virtual std::string get_train_type() = 0;

    std::string train_num_;						// Numero del treno
    bool is_slowing_;                           // Controllo se il treno sta rallentando un altro treno
    bool reverse_;								// True per i treni in ritorno
    const double max_spd_;						// km/min
    double curr_spd_;							// km/min
    double curr_km_;							// Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    Station* curr_stat_;						// nullptr quando parte dalla stazione (e quindi sta viaggiando) oppure è nel parcheggio
    Station* next_stat_;						// Puntatore alla prossima stazione di arrivo, è nullptr nel caso in cui sia al capolinea
    const std::vector<int> arrivals_; 			// Orari in cui io arrivo alle stazioni
    int delay_;									// anticipo = ritardo negativo
    int old_delay_;                             // Variabile per il controllo della variazione del ritardo
    int wait_count_;							// countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    int status_; 								// 0 Mov Normale, 1 Mov Staz, 2 Binario, 3 Park, 4 Fine corsa
    int time_arrival_next_stat_;                // Orario in cui il treno dovrebbe arrivare alla stazione (indice del vettore arrivals
    int last_request_;                          // Risposta dall'utlima richiesta (-2 transito, -1 rifiutata, -3 default, >=0 num binario)
    int plat_num_;                              // Per la stampa del binario
    Railway* central_railw_; 					// Per avere il tempo corrente mi serve il riferimento al rail
};

class Regional : public Train{
public:
    Regional(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail);
    ~Regional();
private:
    std::string get_train_type();
    void calc_specific_delay();
};

class Fast : public Train{
public:
    Fast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail);
    ~Fast();
private:
    std::string get_train_type();
};

class SuperFast : public Train{
public:
    SuperFast(std::string number, bool rev, Station* curr, std::vector<int> times, Railway* rail);
    ~SuperFast();
private:
    std::string get_train_type();
};

#endif











