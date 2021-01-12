#include "Railway.h"

using std::cout;

int main()
{
    cout << "--- RAILWAY ---\n";
	Railway rail_system("line_description.txt", "timetables.txt");
	while (!rail_system.is_completed())
		rail_system.advance_time();
	return 0;
}
