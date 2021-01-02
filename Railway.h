// Header

/*
- Il sistema centrale si accorge delle collisioni perchè è lui che ha accesso a tutti i treni e va a modificare la velocità del treno da rallentare a meno che uno dei due non is trovi in prossimità
 della stazione (zona lenta). 
- Se il treno (che deve caricare/scaricare passeggeri) durante l'andatura di crocera supera la piazzola di sosta nel minuto il treno verrà riportato alla piazzola per poi riprendere a 80 km/h nel minuto successivo
- Le partenze dei treni vengono svolte prima del passaggio dei treni dal parcheggio alla stazione
- Prima di far andare un treno in arrivo (prima dei 5km) in un binario libero devo assicurarmi che non ci sia un treno della piazzola che stia uscendo per andare su un binario libero 
- I treni vanno a cannone fino al massimo aspettano in piazzola
- Il criterio per far partire i treni in piazzola è tipo di treno e poi il ritardo
- Ai 20km si effettua la prima assegnazione valutando l'occupazione istantanea della stazione. Si assegna anche a treni in anticipo
- Ai 20km i treni transito verificano che la stazione a cui si avvicinano abbia il binario transito: in caso questo fosse presente, la stazione deve impedire la partenza di treni passeggeri fino a quando il 
 treno transito non sia ad almeno 10 km dalla stazione. Se bin transito non è presente, non è necessario.
- Dalle stazioni i treni partono differenziati di almeno 7 min, per garantire la distanza fra i treni. (7 min = 4 min del tratto a 80Kmh + 2 min dei successivi 5km fatti, nel caso peggiore, a 160kmh + 1 min tolleranza)
*/

#include <iostream>

class Railway{
	public:
  	Railway(const std::string line_description, const std::string timetables);
  	Railway(const Railway& r) = delete;
  	Railway& operator=(const Railway& r) = delete;
  	Railway(Railway&& r) = delete;
  	Railway& operator=(Railway&& r) = delete;
  	
  	std::string advance_time();
  	bool is_completed();					 // Controllo se tutti i treni sono arrivati a destinazione finale (se si termina il programma) (tutti status 4)
  	std::string check_collision();
  
  	~Railway();
  private:
  	int curr_time_;
  	std::vector<Station*> stations_;
  	std::vector<Train*> trains_;			
};





class Train{
public:
		std::string train_num() const;
  	bool reverse() const;
  	const double max_spd() const;
  	double curr_spd() const;
  	void set_curr_spd(double val);
  	double current_km();
  	std::string advance_train();			// Calcolo del km al prossimo minuto. Aggiorna stato 
    Station* curr_stat();
    Station* next_stat();
  	double distance_next_stat(const Station* next_stat_);
  	
  																// Per controllo collisioni, controllo per le stazioni, ...
  	virtual ~Train();
protected:
    Train();
    //int train_id_;
    std::string train_num_;
    bool reverse_;								// true per i treni in ritorno
    const double max_spd_;				// km/min
    double curr_spd_;							// km/min
    double curr_km_;							// da partenza
    Station* curr_stat_;					// nullptr quando parte dalla stazione
    Station* next_stat_;					//
    std::vector<int> arrivals_; 	// minuti
    int delay_;										// anticipo = ritardo negativo
    int status_; 									// 0 Mov Normale, 1 Mov Staz, 2 Binario, 3 Park, 4 Fine corsa
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

class Station{
public:

protected:
    std::vector<Train*> people_plt_;					// Binari passeggeri e transito andata
    std::vector<Train*> people_ptl_reverse_;	// Binari passeggeri e transito ritorno
    std::vector<Train*> park_;								// (Binari) Parcheggio andata
    std::vector<Train*> park_reverse_;				// (Binari) Parcheggio ritorno
    int distance;															// Distanza dalla stazione primaria
};

class Principal : public Station{
public:

private:
};

class Secondary : public Station{
public:

private:
};




















