#include <stdlib.h>
#include <iostream>

using namespace std;

#include "bmtest.h"
#include "frame.h"

int MINIBASE_RESTART_FLAG = 0;

int main (int argc, char **argv)
{

	BMTester tester;
	Status status;

	int bufSize = NUMBUF; 
	minibase_globals = new SystemDefs(status, "MINIBASE.DB", 2000, bufSize, "Clock");

	cout << "before tests\n";

	if (status != OK)
	{
		cerr << "Error initializing Minibase.\n";
		exit(2);
	}

	cout << "before tests\n";

	status = tester.RunTests();


	if (status != OK)
	{
		cout << "Error running buffer manager tests\n";
		minibase_errors.show_errors();
		//delete the created database
		remove("MINIBASE.DB");
		return 1;
	}

	delete minibase_globals;
	//delete also the created database
	remove("MINIBASE.DB");
	cout << endl;
	return 0;
}
