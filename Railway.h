// Header

/*
- Il sistema centrale si accorge delle collisioni perchè è lui che ha accesso a tutti i treni e va a modificare la velocità del treno da rallentare a meno che uno dei due non is trovi in prossimità
   della stazione (zona lenta). Il treno che causa il ritardo verrà impostato nello stato "Rallentamento" che lo obbliga a NON prenotare binario perchè andrà obbligatoriamente a sostare 5 min in parcheggio per far avvenire sorpasso.
- Se il treno (che deve caricare/scaricare passeggeri) durante l'andatura di crocera supera la piazzola di sosta nel minuto il treno verrà riportato alla piazzola per poi riprendere a 80 km/h nel minuto successivo
- Le partenze dei treni vengono svolte prima del passaggio dei treni dal parcheggio alla stazione
- Prima di far andare un treno in arrivo (prima dei 5km) in un binario libero devo assicurarmi che non ci sia un treno della piazzola che stia uscendo per andare su un binario libero 
- I treni vanno sempre a cannone, l'anticipo è compensato aspettando in piazzola
- Il criterio per far partire i treni in piazzola è tipo di treno e poi il ritardo
- Ai 20km si effettua l'assegnazione del binario valutando l'occupazione istantanea della stazione. Si assegna anche a treni in anticipo. In caso non fosse disponibile alcun binario (poichè assegnato a treno con priorità maggiore) 
   il treno verrà fatto andare in parcheggio.
- Ai 20km i treni transito verificano che la stazione a cui si avvicinano abbia il binario transito: in caso questo fosse presente, la stazione deve impedire la partenza di treni passeggeri per 7 minuti. Se bin transito non è presente, non è necessario.
- Dalle stazioni i treni partono differenziati di almeno 5 min, per garantire la distanza fra i treni.
- Per calcolare il ritardo: ogni treno quando arriva al parcheggio o alla banchina fa la sottrazione tra il tempo in cui doveva arrivare e quello effettivo del sistema
- Ai cambi di velocità (ai 5km) si taglia il minuto (si approssima al km intero dove viene cambiata la velocità)
- Nessun controllo sulla distanza tra treni che sono compresi nel parcheggio
*/


#include <iostream>

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
    std::vector<Station*> stations_;		    // Vettore che contiene tutte le stazioni in sequenza line_description.txt
    std::vector<Train*> trains_;			    // Vettore che contiene tutti i teni presenti in timetables.txt
    std::string messages_;                      // Stringa per l'output a console
};

// ===== STAZIONI ===== DIEGO SPINOSA
class Station{
public:
    Station(const Station& s) = delete;				//
    Station& operator=(const Station& s) = delete;	// Construttori e assegnamenti
    Station(Station&& s) = delete;					// disattivati
    Station& operator=(Station&& s) = delete;		//

    std::string station_name() const;				// Restituisce il nome della stazione
    int distance() const;							// Restituisce la distanza della stazione dall'origine
    int since_train_() const;						// Restituisce il numero di minuti trascorsi dall'ultima partenza della stazione
    Station* next_stat() const;						// Restituisce il puntatore alla prossima stazione
    Station* prev_stat() const;						// Restituisce il puntatore alla stazione precedente (usato per i treni reverse)

    virtual int answer(Train* t) = 0; 				// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    virtual bool answer_exit(Train* t) = 0;		    // (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
    void announce(Train* t) const;					// Il metodo comunica l'arrivo. Nel metodo: si restituisce al flusso generale il messaggio di treno arrivato in stazione.

    virtual ~Station();

protected:
    Station(std::string name, int distance, Station* prev, Railway* rail);

    std::string station_name_;						// Nome della stazione
    std::vector<Train*> platforms;					// Binari passeggeri e transito andata
    std::vector<Train*> platforms_reverse;		    // Binari passeggeri e transito ritorno
    std::vector<Train*> park_;						// (Binari) Parcheggio andata
    std::vector<Train*> park_reverse_;				// (Binari) Parcheggio ritorno
    int distance_;									// Distanza dalla stazione primaria
    Station* next_stat_;							// Punta alla stazione sucessiva per poterlo comunicare ai treni che dovranno partire
    Station* prev_stat_;							// Punta alla precedente per il reverse
    int since_train_;								// Minuti passati dall'ultima partenza
    Railway* central_railw_; 						// Per avere il tempo corrente mi serve il riferimento al rail
};

class Principal : public Station{
public:
    Principal(std::string name, int distance, Station* prev, Railway* rail);

    int answer(Train* t); 							// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta)
    bool answer_exit(Train* t);						// (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento

    virtual ~Principal();
};

class Secondary : public Station{					// Il binario di transito è dato dal diverso comportamento delle stazioni di tipo Principal e Secondary
    //      (perchè nel secondo caso lasciamo passare il treno transito facendo aspettare gli altri)
public:
    Secondary(std::string name, int distance, Station* prev, Railway* rail);

    int answer(Train* t); 							// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario
    //      (ogni ciclo: partenze, richiesta e risposta)
    bool answer_exit(Train* t);						// (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento

    virtual ~Secondary();
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
    const double max_spd() const;				// Massima velocità del treno, possibile controllo del tipo di treno
    double curr_spd() const;					// Velocità corrente: solitamente massima, altrimenti velocità del treno davanti, altrimenti gli 80km/h
    void set_curr_spd(double val);				// Imposta velocità corrente
    double current_km() const;					// Distanza percosa dalla stazione iniziale (o finale nel caso di reverse)
    std::string advance_train();				// Calcolo del km al prossimo minuto. Aggiorna stato
    Station* curr_stat() const;					// Ritorna il puntatore alla stazione dove risiede
    Station* next_stat() const;					// Ritorna il puntatore alla prossima stazione
    std::vector<int>& arrivals() const;		    // Ritorna il riferimento al vettore arrivals_ (non è detto che serva)
    int delay() const;							// Ritorna l'eventuale anticipo/ritardo
    int wait_count() const;						// Ritorna il countdown d'attesa del treno prima che parta, viene assegnato dalla stazione
    void set_wait_count(int min);				// Imposta il countdown d'attesa (in qualsiasi situazione)
    int status() const;							// Ritorna lo stato del treno (0 movimento normale, 1 movimento nei pressi della stazione, 2 nel binario, 3 nel parcheggio, 4 capolinea)
    void set_status();							// Imposta lo stato del treno

    virtual ~Train();

protected:
    Train(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);

    virtual void request() = 0;					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
    virtual void request_exit() = 0;			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
    void arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione

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
    Railway* central_railw_; 					// Per avere il tempo corrente mi serve il riferimento al rail
};

class Regional : public Train{
public:
    Regional(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
    virtual ~Regional();
private:
    virtual void request() = 0;					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
    virtual void request_exit() = 0;			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
    void arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione
};

class Fast : public Train{
public:
    Fast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
    virtual ~Fast();
private:
    virtual void request() = 0;					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
    virtual void request_exit() = 0;			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
    void arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione
};

class SuperFast : public Train{
public:
    SuperFast(std::string number, bool rev, double max, Station* curr, std::vector<int> times, Railway* rail);
    virtual ~SuperFast();
private:
    virtual void request() = 0;					// (richiedere binario sia banchina che transito) void perchè possono modificare le variabili membro
    virtual void request_exit() = 0;			// Richiede alla stazione il permesso di uscire dalla stessa (non è detto che serva, la priorità è data dalla stazione che fa uscire i treni dal parcheggio)
    void arrived();								// Funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione
};














