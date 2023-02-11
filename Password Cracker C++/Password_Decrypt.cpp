#include <map>
#include <mpi.h>
#include <omp.h>
#include <crypt.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "Password_Crack.cpp"

using namespace std;


/* Responsible for distributing the alphabet among the processes, to search for the password and
	 prints the password once it is cracked, and then terminate all processes and end program */
int main(int argc, char** argv)
{
	int myrank, nprocs;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	// time count starts
	double start = MPI_Wtime();
	double end;

	// Master code.
	if (myrank == 0) {
		cout << "Master: There are a total of" << nprocs - 1 << " slave processes" << endl;
		// get_salt_hash function is present in utulity.cpp for parsing the file and extracting the salt + hash for the specified username.
		string salt_hash = get_salt_hash("shadow.txt", "project");	
		// divide_alphabet function is present in utulity.cpp for getting the alphabet division per process (including master, if required)
		map<int, string> distribute_process = divide_alphabet(nprocs-1);	
		// Send salt_hash and the letters assigned to that process to every slave
		for (int i = 1; i < nprocs; i++) {
			// send the salt + hash string to every slave
			MPI_Send(salt_hash.c_str(), 200, MPI_CHAR, i, 100, MPI_COMM_WORLD);
			// send every slave their allocated characters	
			MPI_Send(distribute_process[i].c_str(), 30, MPI_CHAR, i, 101, MPI_COMM_WORLD);	
		}

		// some characters have been allotted to master
		if (distribute_process[0].length() > 0) {	
			string letters = distribute_process[0];
			cout << "Master: " << letters << endl;

			// master needs two threads running in parallel here
			// one to search through its own assigned characters
			// one to wait in case any slave cracks the password and reports to master
			#pragma omp parallel num_threads(2)
			{
				// 1st thread: search through the permuations of it's letters
				if (omp_get_thread_num() == 0) {
					// Go through its assigned characters -> for every character, call the cracking function from Password_Crack.cpp on it.
					/* initial: the character whose possibilities have to be traversed e.g. a -> a,b,c,..,aa,ab, ac...abbbd, abbbe... till azzzzzzzz 
					this function traverses through the character's strings, crypts each using the salt and compares the resulting hash to the salt_hash parameter
					if password is found, function prints result, sends password to master (myrank 0 proc) then terminates*/
					for (int i = 0; i < letters.length(); i++) {
						password_cracker(letters[i], salt_hash);
					}
				}

				// 2nd thread: wait for slave to find the answer instead
				// note: even if master finds the password, it also sends a signal back to itself (and recieves it here)
				else {
					char password[10];
					MPI_Status status;
					MPI_Recv(password, 10, MPI_CHAR, MPI_ANY_SOURCE, 200, MPI_COMM_WORLD, &status);	// blocking recieve waiting for any process to report
					end= MPI_Wtime();
					cout << "Process " << status.MPI_SOURCE << " has cracked the password: '" << password << "' in " << end-start << " seconds."<<  endl;
					cout << "Terminating all processes" << endl;
					// terminates all MPI processes associated with the mentioned communicator (return value 0)
					MPI_Abort(MPI_COMM_WORLD, 0); 

				}
			}
		}
		
		// In this case, the characters have been divided equally among the slaves
		// master has nothing to do except wait for a slave to report password found
		else {
			char password[10];
			end= MPI_Wtime();
			MPI_Status status;
			// blocking recieve waiting for any process to report	
			MPI_Recv(password, 10, MPI_CHAR, MPI_ANY_SOURCE, 200, MPI_COMM_WORLD, &status);	
			cout << "Process " << status.MPI_SOURCE << " has cracked the password: '" << password << "' in " << end-start << " seconds."<<  endl;
			cout << "Terminating all processes" << endl;
			// terminates all MPI processes associated with the mentioned communicator (return value 0)
			MPI_Abort(MPI_COMM_WORLD, 0);	

		}
	}

	// Slave code
	else {
		char salt_hash[200], letters[30];
		// recieve salt_hash (same for every slave)
		MPI_Recv(salt_hash, 200, MPI_CHAR, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// recieve allotted characters (different for every slave)	
		MPI_Recv(letters, 200, MPI_CHAR, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	
		cout << "Slave " << myrank << ": " << letters << endl;
		sleep(2);	// this is just to ensure all slaves print their allotted characters then start execution)
		// go through its assigned characters -> for every character, call the cracking function on it
		for (int i = 0; i < charToString(letters).length(); i++) {
			password_cracker(letters[i], charToString(salt_hash));
		}	
	}
	MPI_Finalize();
	return 0;
}
