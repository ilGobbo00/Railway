/* Rebecchi Giulio 1216393 */

#include "Railway.h"
#include <fstream>
#include <sstream>
#include <math.h>

using std::cout;
using std::string;
using std::ifstream;
using std::istringstream;

Railway::Railway(const string line_description, const string timetables) : curr_time_{ 0 }, stations_{}, trains_{}, messages_{""}	// Crea i vettori della classe dai file forniti
{
	ifstream lines_desc(line_description);	//	Apre file stazioni
	if (!lines_desc.is_open())				//	Controlla che sia aperto
	{
		lines_desc.close();
		throw ("Couldn't open" + line_description + " correctly!\n");
	}

	std::vector<Station*> all_stations;	//	Vettore di appoggio per costruire meglio le timetable (conta anche stazioni non valide)
	std::vector<bool> ok_all_stations;	//	Vettore di appoggio per costruire meglio le timetable (false su stazioni non valide)
	string line1 = "";					//	Singola linea del file da leggere
	string s_name = "";					//	Nome stazione
	int s_type = 0;						//	tipo stazione
	int s_dist1 = 0;					//	km stazione
	int s_prev_dist1 = 0;				//	km stazione precedente

	std::getline(lines_desc, line1);
	all_stations.push_back(new Principal(line1, s_dist1, nullptr, this));	//	Costruzione prima stazione su vett di appoggio
	ok_all_stations.push_back(true);
	stations_.push_back(new Principal(line1, s_dist1, nullptr, this));		//	Costruzione prima stazione su vett stazioni

	while (std::getline(lines_desc, line1))	// Per ogni linea del file rimanente
	{
		s_name = line1.substr(0, line1.find_first_of("01") - 1);	//	Preleva i dati della stazione
		s_type = stoi(line1.substr(line1.find_first_of("01"), 1));
		s_dist1 = stoi(line1.substr(line1.find_first_of("01") + 2));
		if (s_type == 0)	//	Segna la stazione nel vettore di appoggio che tiene anche quelle non valide
			all_stations.push_back(new Principal(s_name, s_dist1, stations_.back(), this));	
		else
			all_stations.push_back(new Secondary(s_name, s_dist1, stations_.back(), this));
		if (s_dist1 - s_prev_dist1 >= 20)	//	Se la stazione ha una distanza valida da quella prima (20 km o più)
		{
			s_prev_dist1 = s_dist1;	//	Aggiorna la distanza precedente (la prima volta era 0)
			if (s_type == 0)
				stations_.push_back(new Principal(s_name, s_dist1, stations_.back(), this));	// Costruzione staz principale nel vett stazioni
			else
				stations_.push_back(new Secondary(s_name, s_dist1, stations_.back(), this));	// Costruzione staz secondaria nel vett stazioni
			ok_all_stations.push_back(true);	//	Segna che la stazione sul vett di appoggio con tutte le stazioni è stata accettata
		}
		else
			ok_all_stations.push_back(false);	//	Segna che la stazione sul vett di appoggio con tutte le stazioni NON è stata accettata
	}
	lines_desc.close();	//	Chiusura file

	ifstream trains_desc(timetables);	//	Apre file treni
	if (!trains_desc.is_open())	//	Controlla che sia aperto
	{
		trains_desc.close();
		throw ("Couldn't open" + timetables + " correctly!\n");
	}
	string line2 = "";	//	Singola linea del file da leggere
	istringstream t_stream;	//	Stringa come stream per la singola linea del file da leggere
	string t_number = "";	//	Numero treno
	int t_rev = 0;			//	Treno al contrario?
	int t_type = 1;			// Tipo di treno
	std::vector<int> t_times;	//	Vettore per gli orari
	bool correction = false;	//	Ho dovuto correggere qualche orario?
	while (std::getline(trains_desc, line2))	//	Per ogni riga del file
	{
		t_stream = istringstream(line2);	//	Inizializza stream
		t_number = line2.substr(0, line2.find(' '));	//	Leggi numero treno come stringa
	    t_stream.ignore(8, ' ');
		t_stream >> t_rev;	//	Leggi direzione treno
		t_stream >> t_type;	//	Leggi tipo treno
		int t_time = 0;	//	Orario in minuti corrente
		int t_prev_time = 0; //	Orario in minuti precedente
		int t_dist2 = 0;	//	km staz corrente
		int t_prev_dist2 = 0;	//	km staz precedente
		correction = false;	//	Resetta se avevo corretto qualcosa
		if (t_type == 1)	//	Se il treno è un Regionale
		{
			if (t_rev == 0)	// E va da origine a capolinea
			{
				t_dist2 = 0;	//	Reset t_dist2
				t_prev_dist2 = 0;	// Reset t_prev_dist2
				for (size_t i = 0; i < all_stations.size(); i++)	//	Analizza stazioni (da prima)
				{
					if (ok_all_stations[i])	//	Se la stazione non è stata rifiutata
					{
						t_prev_dist2 = t_dist2;	//	Aggiorna t_prev_dist2
						t_dist2 = all_stations[i]->distance();	//	Aggiorna t_dist2
						t_prev_time = t_time;	//	Aggiorna t_prev_time
						int t_predicted = static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10;	//	Calcola tempo minimo di viaggio tra la staz e la precedente (non usato per la prima)
						if (t_stream >> t_time)	//	Leggi prossimo orario per arrivare alla stazione interessata
						{	//	Se lo abbiamo
							if (t_times.size() > 0 && (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted))	//	(non per prima staz)	Se non è fattibile per il treno
							{
								t_time = t_prev_time + t_predicted + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);	// Correggiamo con il tempo minimo necessario
								if (!correction)	// E se non è già fatto
									correction = true;	// ci segniamo la correzione
							}
						}
						else	//	Se non lo abbiamo usa il tempo minimo +10 min per calcolarlo
							t_time = t_prev_time + t_predicted + 10 + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);
						t_times.push_back(t_time);	// Aggiungiamo il prossimo orario all'array
					}
					else
						t_stream.ignore(8, ' ');	//	Se la stazione non è stata rifiutata saltiamo 1 orario
				}
			}
			else	// Se va da capolinea ad origine (t_rev == 1)
			{
				t_dist2 = stations_.back()->distance();	//	Reset t_dist2 con ultima staz valida
				t_prev_dist2 = stations_.back()->distance();	//	Reset t_prev_dist2 con ultima staz valida
				for (int i = all_stations.size()-1; i >= 0; i--)	//	Analizza stazioni (da ultima)
				{
					if (ok_all_stations[i])	//	Se la stazione non è stata rifiutata
					{
						t_prev_dist2 = t_dist2;	//	Aggiorna t_prev_dist2
						t_dist2 = all_stations[i]->distance();	//	Aggiorna t_dist2
						t_prev_time = t_time;	//	Aggiorna t_prev_time
						int t_predicted = static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10;	//	Calcola tempo minimo di viaggio tra la staz e la precedente (non usato per la prima)
						if (t_stream >> t_time)	//	Leggi prossimo orario per arrivare alla stazione interessata
						{	//	Se lo abbiamo
							if (t_times.size() > 0 && (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted))	//	(non per prima staz)	Se non è fattibile per il treno
							{
								t_time = t_prev_time + t_predicted + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);	// Correggiamo con il tempo minimo necessario
								if (!correction)	// E se non è già fatto
									correction = true;	// ci segniamo la correzione
							}
						}
						else	//	Se non lo abbiamo usa il tempo minimo +10 min per calcolarlo
							t_time = t_prev_time + t_predicted + 10 + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);
						t_times.push_back(t_time);	// Aggiungiamo il prossimo orario all'array
					}
					else
						t_stream.ignore(8, ' ');	//	Se la stazione è stata rifiutata saltiamo 1 orario
				}
			}
			if (t_times[0] < 1440)	//	Se il treno non parte dopo la mezzanotte del giorno
			{
				trains_.push_back(new Regional(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));	//	Viene aggiunto al vettore dei treni
				if (correction)	//	Se ho dovuto correggere lo comunico al sistema
					append(string("Correzione timetable del treno " + t_number));
			}	//	Altrimenti no
		}
		else	//	Se il treno non è un Regionale, ma un Alta Velocità	o AV Super 
		{
			int c_secondaries = 0;	//	Contatore delle stazioni non principali
			if (t_rev == 0)	// Se va da	origine a capolinea
			{
				t_dist2 = 0;	//	Reset t_dist2
				t_prev_dist2 = 0;	// Reset t_prev_dist2
				for (size_t i = 0; i < all_stations.size(); i++)	//	Analizza stazioni (da prima)
				{
					if (Principal* s = dynamic_cast<Principal*>(all_stations[i]))	// Se è una principale
					{ 
						if (ok_all_stations[i])	// Che non è stata ignorata
						{
							t_prev_dist2 = t_dist2;	//	Aggiorna t_prev_dist2
							t_dist2 = all_stations[i]->distance();	//	Aggiorna t_dist2
							t_prev_time = t_time;	//	Aggiorna t_prev_time
							//	Calcola tempo minimo di viaggio tra la staz e la precedente (usa le secondarie incontrate per rallentare il transito, per evitare collisioni, e non usato per la prima)
							int t_predicted = static_cast<int>(ceil((c_secondaries * 25 < t_dist2 - t_prev_dist2 - 10) ? c_secondaries * 25 / (190.0 / 60) + (t_dist2 - t_prev_dist2 - (c_secondaries * 25) - 10) / ((t_type == 2 ? 240.0 : 300.0) / 60) : (t_dist2 - t_prev_dist2 - 10) / (190.0 / 60))) + 10;
							if (t_stream >> t_time)	//	Leggi prossimo orario per arrivare alla stazione interessata
							{	//	Se lo abbiamo
								if (t_times.size() > 0 && (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted))	//	(non per prima staz)	Se non è fattibile per il treno
								{
									t_time = t_prev_time + t_predicted + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);	// Correggiamo con il tempo minimo necessario
									if (!correction)	// E se non è già fatto
										correction = true;	// ci segniamo la correzione
								}
							}
							else	//	Se non lo abbiamo usa il tempo minimo +10 min per calcolarlo
								t_time = t_prev_time + t_predicted + 10;
							t_times.push_back(t_time); ;	// Aggiungiamo il prossimo orario all'array
							c_secondaries = 0;;	// Resettiamo il contatore delle secondarie incontrate
						}
						else
							t_stream.ignore(8, ' ');	//	Se la stazione è stata rifiutata saltiamo 1 orario
					}
					else if (ok_all_stations[i])	// Se non è una principale ma è stat contata
						c_secondaries++;			// Aggiungila al conto delle secondarie
				}
			}
			else	// Se va da capolinea ad origine (t_rev == 1)	
			{
				t_dist2 = stations_.back()->distance();	//	Reset t_dist2 con ultima staz valida
				t_prev_dist2 = stations_.back()->distance();	//	Reset t_prev_dist2 con ultima staz valida
				for (int i = all_stations.size() - 1; i >= 0; i--)	//	Analizza stazioni (da ultima)
				{
					if (Principal* s = dynamic_cast<Principal*>(all_stations[i]))	// Se è una principale
					{
						if (ok_all_stations[i])	// Che non è stata ignorata
						{
							t_prev_dist2 = t_dist2;	//	Aggiorna t_prev_dist2
							t_dist2 = all_stations[i]->distance();	//	Aggiorna t_dist2
							t_prev_time = t_time;	//	Aggiorna t_prev_time
							//	Calcola tempo minimo di viaggio tra la staz e la precedente (usa le secondarie incontrate per rallentare il transito, per evitare collisioni, e non usato per la prima)
							int t_predicted = static_cast<int>(ceil((c_secondaries * 25 < t_dist2 - t_prev_dist2 - 10) ? c_secondaries * 25 / (190.0 / 60) + (t_dist2 - t_prev_dist2 - (c_secondaries * 25) - 10) / ((t_type == 2 ? 240.0 : 300.0) / 60) : (t_dist2 - t_prev_dist2 - 10) / (190.0 / 60))) + 10;
							if (t_stream >> t_time)	//	Leggi prossimo orario per arrivare alla stazione interessata
							{	//	Se lo abbiamo
								if (t_times.size() > 0 && (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted))	//	(non per prima staz)	Se non è fattibile per il treno
								{
									t_time = t_prev_time + t_predicted + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0);	// Correggiamo con il tempo minimo necessario
									if (!correction)	// E se non è già fatto
										correction = true;	// ci segniamo la correzione
								}
							}
							else	//	Se non lo abbiamo usa il tempo minimo +10 min per calcolarlo
								t_time = t_prev_time + t_predicted + 10;
							t_times.push_back(t_time); ;	// Aggiungiamo il prossimo orario all'array
							c_secondaries = 0;;	// Resettiamo il contatore delle secondarie incontrate
						}
						else
							t_stream.ignore(8, ' ');	//	Se la stazione è stata rifiutata saltiamo 1 orario
					}
					else if (ok_all_stations[i])	// Se non è una principale ma è stat contata
						c_secondaries++;			// Aggiungila al conto delle secondarie
				}
			}
			if (t_times[0] < 1440)	//	Se il treno non parte dopo la mezzanotte del giorno
			{
				if (t_type == 2)
					trains_.push_back(new Fast(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));	//	Viene aggiunto al vettore dei treni (AV)
				else
					trains_.push_back(new SuperFast(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));	//	Viene aggiunto al vettore dei treni	(AVS)
				if (correction)	//	Se ho dovuto correggere lo comunico al sistema
					append(string("Correzione timetable del treno " + t_number));
			}	//	Altrimenti no
		}
		t_stream.clear();	//	Pulisci stream riga
		t_times.clear();	//	Pulisci vettore treni
	}
	trains_desc.close();	// Chiudi file
	for (size_t i = 0; i < trains_.size(); i++)	// Si segna i treni separandoli in due vettori di appoggio per la classe dividendoli in treni in andata e ritorno
		if (!trains_[i]->reverse())
			trains_pts_.push_back(trains_[i]);
		else
			trains_rev_pts_.push_back(trains_[i]);
	sort_trains_pts();	//	Ordina i puntatori ai treni in andata (il primo è quello con max km)
	sort_trains_rev_pts();	//	Ordina i puntatori ai treni in andata (il primo è quello con max km)
	printout();	//	Stampa messaggi
}

Railway::~Railway()	//	Distruttore per la memoria allocata alle stazioni e treni
{
	for (size_t i = 0; i < stations_.size(); i++)
		delete stations_[i];
	for (size_t i = 0; i < trains_.size(); i++)
		delete trains_[i];
}

void Railway::advance_time()					// Fa avanzare il tempo e restituisce una concatenazione di stringhe che indica ciò che è successo nel minuto precedente
{
	for (size_t i = 0; i < stations_.size(); i++)	//	aggiorna stazioni
		stations_[i]->update();
	for (size_t i = 0; i < trains_.size(); i++)	// aggiorna treni
		if (trains_[i]->status() != 4)			// che non hanno finito la corsa
			trains_[i]->advance_train();
	check_interaction();						// controlla treni vicini e gestiscili
	printout();									// stampa messaggi (generati da aggiornamenti)
	curr_time_++;								// avanza il tempo del sistema di un minuto
}

bool Railway::is_completed() const					// Controllo se tutti i treni sono arrivati a destinazione finale (se sì termina il programma) (tutti status 4, e tutti con next_stat_ = nullptr)
{
	bool done = true;
	for (size_t i = 0; i < trains_.size(); i++)
		if (trains_[i]->status() != 4)
		{
			done = false;
			break;
		}
	return done;
}

void Railway::check_interaction()				// Controllo distanza (collisioni e sorpasso) (non modifica trains_, ma solo i due vettori di appoggio)
{
	sort_trains_pts();	//	Ordina i puntatori ai treni in andata (il primo è quello con max km)
	sort_trains_rev_pts();	//	Ordina i puntatori ai treni in andata (il primo è quello con max km)
	for (size_t i = 0; i < static_cast<int>(trains_pts_.size()) - 1; i++)	//	Controlla per ogni coppia di treni in andata consecutivi che non siano in stazione, vicino ad una, od in transito che non siano più vicino di 10 km, se lo sono
	{																		//	Allora se quello davanti sta rallentando quello dietro (confronto tipi), lo comunico; e imposto quello dietro alla velocità di quello davanti
		if ((trains_pts_[i]->status() == 0 && trains_pts_[i + 1]->status() == 0) && (abs(trains_pts_[i]->current_km() - trains_pts_[i + 1]->current_km()) < 10.0))
		{
			if (Regional* t1 = dynamic_cast<Regional*>(trains_pts_[i]))
			{
				if (Fast* t1 = dynamic_cast<Fast*>(trains_pts_[i + 1]))
				{
					trains_pts_[i]->set_slowing(true);
					trains_pts_[i + 1]->set_curr_spd(trains_pts_[i]->curr_spd());
				}
				else if (SuperFast* t1 = dynamic_cast<SuperFast*>(trains_pts_[i + 1]))
				{
					trains_pts_[i]->set_slowing(true);
					trains_pts_[i + 1]->set_curr_spd(trains_pts_[i]->curr_spd());
				}
			}
			else if (Fast* t1 = dynamic_cast<Fast*>(trains_pts_[i]))
			{
				if (SuperFast* t1 = dynamic_cast<SuperFast*>(trains_pts_[i + 1]))
				{
					trains_pts_[i]->set_slowing(true);
					trains_pts_[i + 1]->set_curr_spd(trains_pts_[i]->curr_spd());
				}
			}
		}
	}
	for (int i = 0; i < static_cast<int>(trains_rev_pts_.size()) - 1; i++)	//	Controlla per ogni coppia di treni in ritorno consecutivi che non siano in stazione, vicino ad una, od in transito che non siano più vicino di 10 km, se lo sono
	{																		//	Allora se quello davanti sta rallentando quello dietro (confronto tipi), lo comunico; e imposto quello dietro alla velocità di quello davanti
		if ((trains_rev_pts_[i]->status() == 0 && trains_rev_pts_[i + 1]->status() == 0) && (abs(trains_rev_pts_[i]->current_km() - trains_rev_pts_[i + 1]->current_km()) < 10.0))
		{
			if (Regional* t1 = dynamic_cast<Regional*>(trains_rev_pts_[i]))
			{
				if (Fast* t1 = dynamic_cast<Fast*>(trains_rev_pts_[i + 1]))
				{
					trains_rev_pts_[i]->set_slowing(true);
					trains_rev_pts_[i + 1]->set_curr_spd(trains_rev_pts_[i]->curr_spd());
				}
				else if (SuperFast* t1 = dynamic_cast<SuperFast*>(trains_rev_pts_[i + 1]))
				{
					trains_rev_pts_[i]->set_slowing(true);
					trains_rev_pts_[i + 1]->set_curr_spd(trains_rev_pts_[i]->curr_spd());
				}
			}
			else if (Fast* t1 = dynamic_cast<Fast*>(trains_rev_pts_[i]))
			{
				if (SuperFast* t1 = dynamic_cast<SuperFast*>(trains_rev_pts_[i + 1]))
				{
					trains_rev_pts_[i]->set_slowing(true);
					trains_rev_pts_[i + 1]->set_curr_spd(trains_rev_pts_[i]->curr_spd());
				}
			}
		}
	}
}

int Railway::curr_time() const { return curr_time_; } // Ritorna il tempo corrente

void Railway::append(std::string msg) { if (msg != "") messages_ += msg + "\n"; } // Funzione per aggiungere contenuto in coda alla stringa di output della Railway

void Railway::printout()						// Funzione di stampa della stringa di output messages_ (svuota la stringa dopo averla restituita)
{
	if (messages_ != "")
	{
		char formatted_time[6];
		int hour = (curr_time_ / 60) % 24;
		int mins = curr_time_ % 60;
		int result = sprintf_s(formatted_time, 6, "%.2d:%.2d", hour, mins);
		string print = "---RAILWAY SYSTEM---\t[" + (string)formatted_time + "]\n" + messages_;
		messages_ = "";
		cout << print;
	}
}

void Railway::sort_trains_pts()						//	Selection sort per i puntatori dei treni in andata (km più alto prima)
{
	if (static_cast<int>(trains_pts_.size()) >= 2)
	{
		for (size_t i = 0; i < trains_pts_.size() - 1; i++) {
			size_t max = i;
			for (size_t j = i + 1; j < trains_pts_.size(); j++) {
				if (trains_pts_[max]->current_km() < trains_pts_[j]->current_km())
					max = j;
			}
			if (max != i) {
				Train* temp = trains_pts_[i];
				trains_pts_[i] = trains_pts_[max];
				trains_pts_[max] = temp;
			}
		}
	}
}

void Railway::sort_trains_rev_pts()					//	Selection sort per i puntatori dei treni in ritorno (km più basso prima)
{
	if (static_cast<int>(trains_rev_pts_.size()) >= 2)
	{
		for (size_t i = 0; i < trains_rev_pts_.size() - 1; i++) {
			size_t min = i;
			for (size_t j = i + 1; j < trains_rev_pts_.size(); j++) {
				if (trains_rev_pts_[min]->current_km() > trains_rev_pts_[j]->current_km())
					min = j;
			}
			if (min != i) {
				Train* temp = trains_rev_pts_[i];
				trains_rev_pts_[i] = trains_rev_pts_[min];
				trains_rev_pts_[min] = temp;
			}
		}
	}
}
