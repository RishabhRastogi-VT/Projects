#include <map>
#include <string>
#include <vector>
#include <string.h>
#include <sstream>
#include <fstream>
#include <algorithm>

using std::map;
using std::ios;
using std::cout;
using std::fstream;
using std::string;
using std::vector;
using std::stringstream;

template <typename T, typename U>
void print_map(map<T, U> to_print) {
	// A templatised function to output a map's key value pairs

	for (auto itr = to_print.begin(); itr != to_print.end(); ++itr) {
		cout << itr->first << ": " << itr->second << "\n";
	}
}

string charToString(char arr[]) {
	// Function that returns a string representation of a character array

	string str = "";

	for (int i = 0; arr[i] != '\0'; i++)
		str += arr[i];

	return str;
}

vector<string> split_string(const string str, const char delim) {

	// this is a custom function to split a string
	// returns a vector of "words", according to the delimiter char
    
    string token;
    vector<string> out;
    stringstream ss(str);

    while (getline(ss, token, delim))	// splitting or tokenizing the string according to the specified delimiter
        out.push_back(token);	// adding each token to the vector

    return out;
}

string get_salt_hash(const string file, const string username) {

	/* initial: The character whose possibilities have to be traversed e.g. a -> a,b,c,..,aa,ab, ac...abbbd, abbbe... till azzzzzzzz 
	this function traverses through the character's strings, crypts each using the salt and compares the resulting hash to the salt_hash parameter
	if password is found, function prints result, sends password to master (rank 0 proc) then terminates*/
	string line, salt_hash;
	fstream infile;
    infile.open(file, ios::in);
	
	if (infile.is_open()) {
		while (getline(infile, line)) {
			if (strstr(line.c_str(), username.c_str())) {	// If line has specified username
				vector<string> tokens = split_string(line, ':');	// Split by " : " character
				salt_hash = tokens[1];	// Salt and hash are after first colon and before second
				break;
			}
		}

		return salt_hash;
	}

	else {
		cout << "Shadow file did not open\n";
		return "";
	}
}

map<int, string> divide_alphabet(const int slave_procs) {

	/* this function divides the alphabet characters and assigns certain number of characters to each process
	 also takes into account the case when alphabet is not perfectly divisible by number of slave processes
	 so may also assign master some characters
	 the distribution is stored in a map with (key, value) pair being (process rank, characters assigned)
	 rank 0 is master, rank 1 is slave 1, rank 2 is slave 2 and so on */

	map<int, string> distrib;
    string alphabet = "abcdefghijklmnopqrstuvwxyz";
    random_shuffle(alphabet.begin(), alphabet.end());
    
	// case 1: Alphabet perfectly divisible by number of slaves
    if (alphabet.length() % slave_procs == 0) {
        int chunk = alphabet.length() / slave_procs;
	// No letters assigned to master (characters can be evenly distributed among the slaves), still we keep a record for master in case required
		distrib[0] = "";	
		int index = 0;
        for (int i = 1; i <= slave_procs; i++) {
            int start = index;

			string letters = alphabet.substr(start, chunk);
	// Assigning letters to that slave rank
			distrib[i] = letters;	
			index += chunk;
		}
          
    }
	// Case 2: Alphabet not perfectly divisible, some characters will have to be allotted to master too
    else {
        int mod = alphabet.length() % slave_procs;
		distrib[0] = alphabet.substr(0,mod);
        int new_len = alphabet.length()-mod;
        int mod1 = new_len/slave_procs;
		int index = mod;
        for (int i = 1 ; i <= slave_procs; i++) {
            int start = index;
            string letters = alphabet.substr(start, mod1);
			distrib[i] = letters;
            index+=mod1;
        }
    }
	return distrib;
}
