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

// ===== SISTEMA CENTRALE =====
class Railway{
	public:
  	Railway(const std::string line_description, const std::string timetables);
  	Railway(const Railway& r) = delete;
  	Railway& operator=(const Railway& r) = delete;
  	Railway(Railway&& r) = delete;
  	Railway& operator=(Railway&& r) = delete;
  	
  	std::string advance_time();
  	bool is_completed();					 		// Controllo se tutti i treni sono arrivati a destinazione finale (se si termina il programma) (tutti status 4)
  	std::string check_interaction();	// Controllo distanza (collisioni e sorpasso)
  
  	~Railway();
  private:
  	int curr_time_;
  	std::vector<Station*> stations_;
  	std::vector<Train*> trains_;			
};

// ===== STAZIONI =====
class Station{
public:
		virtual int answer(Train* t) = 0; 				// (Interazione con stazione) -1: binario non disponibile (vai in park, chiedi binario di nuovo dopo), >=0 n. binario (ogni ciclo: partenze, richiesta e risposta) 
  	virtual bool answer_exit(Train* t) = 0;		// (Con treno sui binari) TRUE: partenza consentita, FALSE: stazionamento
    
protected:
    std::vector<Train*> platforms;						// Binari passeggeri e transito andata
    std::vector<Train*> platforms_reverse;		// Binari passeggeri e transito ritorno
    std::vector<Train*> park_;								// (Binari) Parcheggio andata
    std::vector<Train*> park_reverse_;				// (Binari) Parcheggio ritorno
    int distance;															// Distanza dalla stazione primaria
  	void announce(Train* t);									// Il metodo comunica l'arrivo. Nel metodo: si restituisce al flusso generale il messaggio di treno arrivato in stazione.
};

class Principal : public Station{
public:

private:
};

class Secondary : public Station{
public:
	//	-2: OK transito,
private:
};




// ===== TRENI =====
class Train{
public:
  	Train(const Train& t) = delete;
  	Train& operator=(const Train& t) = delete;
  	Train(Train&& t) = delete;
  	Train& operator=(Train&& t) = delete;
  
		std::string train_num() const;
  	bool reverse() const;
  	const double max_spd() const;
  	double curr_spd() const;
  	void set_curr_spd(double val);
  	double current_km() const;
  	std::string advance_train();					// Calcolo del km al prossimo minuto. Aggiorna stato 
    Station* curr_stat() const;
    Station* next_stat() const;
  	std::vector<int>& arrivals() const;
  	int delay() const;
  	int status() const;
  	void set_status();
  
  	
  
  	virtual ~Train();
protected:
    Train();
    //int train_id_;
    std::string train_num_;
    bool reverse_;												// true per i treni in ritorno
    const double max_spd_;								// km/min
    double curr_spd_;											// km/min
    double curr_km_;											// da partenza
    Station* curr_stat_;									// nullptr quando parte dalla stazione (e quindi sta viaggiando) 
    Station* next_stat_;									//
    std::vector<int> arrivals_; 					// minuti
    int delay_;														// anticipo = ritardo negativo
    int status_; 													// 0 Mov Normale, 1 Mov Staz, 2 Binario, 3 Park, 4 Fine corsa
  
  	virtual void request() = 0;						// void perchè possono modificare le variabili membro
  	virtual void request_exit() = 0;
  	void arrived();												//funzione interna invocata dal treno stesso per annunciare il suo arrivo in una stazione
};

class Regional : public Train{
public:

private:
};

class Fast : public Train{
public:

private:
};

class SuperFast : public Train{
public:

private:
};




















