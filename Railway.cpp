#include "Railway.h"
#include <fstream>
#include <sstream>
#include <math.h>

using std::cout;
using std::string;
using std::ifstream;
using std::istringstream;

Railway::Railway(const string line_description, const string timetables) : curr_time_{ 0 }, stations_{}, trains_{}, messages_{""}	// Crea i vettori stations_ e trains_
{
	ifstream lines_desc(line_description);
	if (!lines_desc.is_open())
	{
		lines_desc.close();
		throw ("Couldn't open" + line_description + " correctly!\n");
	}

	std::vector<Station*> all_stations;
	std::vector<bool> ok_all_stations;
	string line1 = "";
	string s_name = "";
	int s_type = 0;
	int s_dist1 = 0;
	int s_prev_dist1 = 0;

	std::getline(lines_desc, line1);
	all_stations.push_back(new Principal(line1, s_dist1, nullptr, this));
	ok_all_stations.push_back(true);
	stations_.push_back(new Principal(line1, s_dist1, nullptr, this));

	while (std::getline(lines_desc, line1))
	{
		s_name = line1.substr(0, line1.find_first_of("01") - 1);
		s_type = stoi(line1.substr(line1.find_first_of("01"), 1));
		s_dist1 = stoi(line1.substr(line1.find_first_of("01") + 2));
		if (s_type == 0)
			all_stations.push_back(new Principal(s_name, s_dist1, stations_.back(), this));
		else
			all_stations.push_back(new Secondary(s_name, s_dist1, stations_.back(), this));
		if (s_dist1 - s_prev_dist1 >= 20)
		{
			s_prev_dist1 = s_dist1;
			if (s_type == 0)
				stations_.push_back(new Principal(s_name, s_dist1, stations_.back(), this));
			else
				stations_.push_back(new Secondary(s_name, s_dist1, stations_.back(), this));
			ok_all_stations.push_back(true);
		}
		else
			ok_all_stations.push_back(false);
	}
	lines_desc.close();

	ifstream trains_desc(timetables);
	if (!trains_desc.is_open())
	{
		trains_desc.close();
		throw ("Couldn't open" + timetables + " correctly!\n");
	}
	string line2 = "";
	istringstream t_stream;
	string t_number = "";
	int t_rev = 0;
	int t_type = 1;
	std::vector<int> t_times;
	bool correction = false;
	while (std::getline(trains_desc, line2))
	{
		t_stream = istringstream(line2);
		t_number = line2.substr(0, line2.find(' '));
	    t_stream.ignore(8, ' ');
		t_stream >> t_rev;
		t_stream >> t_type;
		int t_time = 0;
		int t_prev_time = 0;
		int t_dist2 = 0;
		int t_prev_dist2 = 0;
		correction = false;
		if (t_type == 1)
		{
			if (t_rev == 0)
			{
				t_dist2 = 0;
				t_prev_dist2 = 0;
				for (size_t i = 0; i < all_stations.size(); i++)
				{
					if (ok_all_stations[i])
					{
						t_prev_dist2 = t_dist2;
						t_dist2 = all_stations[i]->distance();
						t_prev_time = t_time;
						if (t_stream >> t_time)
						{
							if (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10)
							{
								t_time = t_prev_time + static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10;
								if (!correction)
									correction = true;
							}
						}
						else
							t_time = t_prev_time + (static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10) + 10;
						t_times.push_back(t_time);
					}
					else
						t_stream.ignore(8, ' ');
				}
			}
			else
			{
				t_dist2 = stations_.back()->distance();
				t_prev_dist2 = stations_.back()->distance();
				for (int i = all_stations.size()-1; i >= 0; i--)
				{
					if (ok_all_stations[i])
					{
						t_prev_dist2 = t_dist2;
						t_dist2 = all_stations[i]->distance();
						t_prev_time = t_time;
						if (t_stream >> t_time)
						{
							if (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10)
							{
								t_time = t_prev_time + static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10;
								if (!correction)
									correction = true;
							}
						}
						else
							t_time = t_prev_time + (static_cast<int>(ceil((t_dist2 - t_prev_dist2 - 10) / (160.0 / 60))) + 10) + 10;
						t_times.push_back(t_time);
					}
					else
						t_stream.ignore(8, ' ');
				}
			}
			if (t_times[0] < 1440)
			{
				trains_.push_back(new Regional(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));
				if (correction)
					append(string("Correzione timetable del treno " + t_number));
			}
		}
		else
		{
			int c_secondaries = 0;
			if (t_rev == 0)
			{
				t_dist2 = 0;
				t_prev_dist2 = 0;
				for (size_t i = 0; i < all_stations.size(); i++)
				{
					if (Principal* s = dynamic_cast<Principal*>(all_stations[i]))
					{ 
						if (ok_all_stations[i])
						{
							t_prev_dist2 = t_dist2;
							t_dist2 = all_stations[i]->distance();
							t_prev_time = t_time;
							int t_predicted = static_cast<int>(ceil((c_secondaries * 25 < t_dist2 - t_prev_dist2 - 10) ? c_secondaries * 25 / (190.0 / 60) + (t_dist2 - t_prev_dist2 - (c_secondaries * 25) - 10) / ((t_type == 2 ? 240.0 : 300.0) / 60) : (t_dist2 - t_prev_dist2 - 10) / (190.0 / 60))) + 10;
							if (t_stream >> t_time)
								if (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted)
								{
									t_time = t_prev_time + t_predicted;
									if (!correction)
										correction = true;
								}
							else
								t_time = t_prev_time + t_predicted + 10;
							t_times.push_back(t_time);
							c_secondaries = 0;
						}
						else
							t_stream.ignore(8, ' ');
					}
					else if (ok_all_stations[i])
						c_secondaries++;
				}
			}
			else
			{
				t_dist2 = stations_.back()->distance();
				t_prev_dist2 = stations_.back()->distance();
				for (int i = all_stations.size() - 1; i >= 0; i--)
				{
					if (Principal* s = dynamic_cast<Principal*>(all_stations[i]))
						if (ok_all_stations[i])
						{
							t_prev_dist2 = t_dist2;
							t_dist2 = all_stations[i]->distance();
							t_prev_time = t_time;
							int t_predicted = static_cast<int>(ceil((c_secondaries * 25 < t_dist2 - t_prev_dist2 - 10) ? c_secondaries * 25 / (190.0 / 60) + (t_dist2 - t_prev_dist2 - (c_secondaries * 25) - 10) / ((t_type == 2 ? 240.0 : 300.0) / 60) : (t_dist2 - t_prev_dist2 - 10) / (190.0 / 60))) + 10;
							if (t_stream >> t_time)
								if (t_time - (t_prev_time + (static_cast<int>(t_times.size()) >= 2 ? 5 : 0)) < t_predicted)
								{
									t_time = t_prev_time + t_predicted;
									if (!correction)
										correction = true;
								}
							else
								t_time = t_prev_time + t_predicted + 10;
							t_times.push_back(t_time);
							c_secondaries = 0;
						}
						else
							t_stream.ignore(8, ' ');
					else if (ok_all_stations[i])
						c_secondaries++;
				}
			}
			if (t_times[0] < 1440)
			{
				if (t_type == 2)
					trains_.push_back(new Fast(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));
				else
					trains_.push_back(new SuperFast(t_number, t_rev, (t_rev == 0 ? stations_.front() : stations_.back()), t_times, this));
				if (correction)
					append(string("Correzione timetable del treno " + t_number));
			}
		}
		t_stream.clear();
		t_times.clear();
	}
	trains_desc.close();
	for (size_t i = 0; i < trains_.size(); i++)
		if (!trains_[i]->reverse())
			trains_pts_.push_back(trains_[i]);
		else
			trains_rev_pts_.push_back(trains_[i]);
	sort_trains_pts();
	sort_trains_rev_pts();
	printout();
}

Railway::~Railway()
{
	for (size_t i = 0; i < stations_.size(); i++)
		delete stations_[i];
	for (size_t i = 0; i < trains_.size(); i++)
		delete trains_[i];
}

void Railway::advance_time()					// Fa avanzare il tempo e restituisce una concatenazione di stringhe che indica ciò che è successo nel minuto precedente
{
	for (size_t i = 0; i < stations_.size(); i++)
		stations_[i]->update();
	for (size_t i = 0; i < trains_.size(); i++)
		if (trains_[i]->status() != 4)
			trains_[i]->advance_train();
	check_interaction();
	curr_time_++;
	printout();
}

bool Railway::is_completed() const					// Controllo se tutti i treni sono arrivati a destinazione finale (se si termina il programma) (tutti status 4, e tutti con next_stat_ = nullptr)
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

void Railway::check_interaction()				// Controllo distanza (collisioni e sorpasso)
{
	sort_trains_pts();
	sort_trains_rev_pts();
	for (size_t i = 0; i < static_cast<int>(trains_pts_.size())-1; i++)
	{
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
	for (int i = 0; i < static_cast<int>(trains_rev_pts_.size())-1; i++)
	{
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

void Railway::append(std::string msg) { if(msg != "") messages_ += msg + "\n"; } // Funzione per aggiungere contenuto in coda alla stringa di output della Railway

void Railway::printout()						// Funzione di stampa della stringa di output messages_ (svuota la stringa dopo averla restituita)
{
	if (messages_ != "")
	{
		int hour = (curr_time_ / 60) % 24;
		int mins = curr_time_ % 60;
		string print = "---RAILWAY SYSTEM---\t[" + std::to_string(hour) + ":" + std::to_string(mins) + "]\n" + messages_;
		messages_ = "";
		cout << print;
	}
}

void Railway::sort_trains_pts()
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

void Railway::sort_trains_rev_pts()
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