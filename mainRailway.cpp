#include "Railway.h"

using std::cout;

int main()
{
    cout << "--- RAILWAY ---\n";
	Railway rail_system("C:\\Users\\ricca\\Documents\\GitHub\\Railway\\line_description.txt", "C:\\Users\\ricca\\Documents\\GitHub\\Railway\\timetables.txt");
	while (!rail_system.is_completed())
		rail_system.advance_time();
	return 0;
}
