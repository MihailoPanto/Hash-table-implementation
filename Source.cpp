#include <string>
#include <iostream>
#include <fstream>

#define N 10

using namespace std;

typedef unsigned Key;

class AddressFunction {
public:
	virtual unsigned getAddress(Key k, unsigned a, int i, int n) = 0;
};

class DoubleHashing : public AddressFunction {
public:
	DoubleHashing(int p, int q) : p(p), q(q) {}

	void set_p(int p) { this->p = p; }
	void set_q(int q) { this->q = q; }

	unsigned getAddress(Key k, unsigned a, int i, int n) {
		return a + i * (q + (k % p));
	}
private:
	int p, q;
};

class Info {
public:
	string name;
	Key key;
	string* subjects;
	int num_subjects;

	Info() : num_subjects(0), key(0) {
		subjects = new string[N];
	}

	Info(Key key, string name) : key(key), name(name), num_subjects(0) {
		subjects = new string[N];
	}

	Info(Key key, string name, string* subjects, int subj_size) : key(key), name(name), num_subjects(subj_size) {
		this->subjects = new string[subj_size];

		for (int i = 0; i < subj_size; ++i) {
			this->subjects[i] = subjects[i];
		}
	}

	~Info() {
		delete[] subjects;
	}

	void add_subject(string subj) {
		subjects[num_subjects++] = subj;
	}

	void remove_subject(string subj) {
		int write_index = 0;
		for (int i = 0; i < num_subjects; ++i) {
			subjects[write_index] = subjects[i];
			if (subjects[i] != subj) write_index++;
		}
		num_subjects = write_index;
	}

	friend ostream& operator<<(ostream& os, Info& t);
};

ostream& operator<<(ostream& os, Info& t) {
	os << t.key << ", " << t.name << ", ";
	for (int i = 0; i < t.num_subjects; ++i) {
		os << t.subjects[i] << " ";
	}
	return os;
}

class HashTable {
public:

	HashTable(int k, int p, AddressFunction* f) : bucket_capacity(k), func(f), num_keys(0) {
		this->num_bucket = 1 << p;
		table = new Info * *[num_bucket];
		bucket_size = new int[num_bucket];

		for (int i = 0; i < num_bucket; ++i) {
			table[i] = new Info * [bucket_capacity];
			bucket_size[i] = 0;
		}
	}
	~HashTable() {
		for (int i = 0; i < num_bucket; ++i) {
			for (int j = 0; j < bucket_size[i]; ++j)
				delete table[i][j];
			delete[] table[i];
			bucket_size[i] = 0;
		}
		delete[] table;
	}

	Info* findKey(Key k);
	bool insertKey(Key k, Info& i);
	bool deleteKey(Key k);
	void clear();
	int keyCount();
	int tableSize();
	friend ostream& operator<<(ostream& os, HashTable& t);
	double fillRatio();

	bool fillTable(string filename);

private:
	int bucket_capacity;
	int num_bucket;
	int num_keys;
	Info*** table;
	int* bucket_size;
	AddressFunction* func;
	int hash_function(Key k);
	Info* find_key_in_bucket(Key k, int bucket);
	bool insert_key_in_bucket(Info* i, int bucket);
};

int HashTable::hash_function(Key k) {
	return k % num_bucket;
}

Info* HashTable::find_key_in_bucket(Key k, int bucket) {
	for (int i = 0; i < bucket_size[bucket]; ++i) {
		if (table[bucket][i]->key == k) return table[bucket][i];
	}
	return nullptr;
}

Info* HashTable::findKey(Key k) {
	int hash = hash_function(k);
	Info* info = nullptr;

	info = find_key_in_bucket(k, hash);
	if (info != nullptr) return info;

	int attempt = 1;
	unsigned address = hash;
	
	while (bucket_size[address] != 0 && info == nullptr && attempt <= num_bucket) {
		address = (this->func->getAddress(k, hash, attempt, num_bucket)) % num_bucket;
		info = find_key_in_bucket(k, address);
		attempt++;
	}
	return info;
}

bool HashTable::insert_key_in_bucket(Info* info, int hash) {
	for (int j = 0; j < bucket_size[hash]; ++j) {
		if (table[hash][j]->key == -1) {
			table[hash][j] = info;
			num_keys++;
			return true;
		}
	}
	if (bucket_size[hash] < this->bucket_capacity) {
		table[hash][bucket_size[hash]] = info;
		num_keys++;
		bucket_size[hash]++;
		return true;
	}
	return false;
}

bool HashTable::insertKey(Key k, Info& i) {
	int hash = hash_function(k);
	Info* info = findKey(k);
	if (info != nullptr) return false;

	info = new Info(i.key, i.name, i.subjects, i.num_subjects);
	bool inserted = insert_key_in_bucket(info, hash);
	if (inserted) return true;

	int attempt = 1;
	unsigned address = hash;
	while (!inserted && attempt <= num_bucket) {
		address = (this->func->getAddress(k, hash, attempt, num_bucket)) % num_bucket;
		if (inserted = insert_key_in_bucket(info, address)) return true;
		attempt++;
	}
	return false;
}

bool HashTable::deleteKey(Key k) {
	int hash = hash_function(k);
	Info* info = findKey(k);
	if (info == nullptr) return false;

	num_keys--;
	info->key = -1;
	return true;
}

void HashTable::clear() {
	for (int i = 0; i < num_bucket; ++i) {
		for (int j = 0; j < bucket_size[i]; ++j)
			delete table[i][j];
		bucket_size[i] = 0;
	}
	num_keys = 0;
}

int HashTable::keyCount() {
	return num_keys;
}

int HashTable::tableSize() {
	return num_bucket;
}

bool HashTable::fillTable(string filename) {
	string line;

	ifstream file(filename);
	if (!file) return false;
	getline(file, line);

	while (getline(file, line)) {
		int start = 0, end = line.find(',');
		Key index = stoi(line.substr(start, end - start));
		start = end + 1;
		end = line.find(',', start);
		string name = line.substr(start, end - start);
		string* subjects = new string[N];
		int subj_size = 0;

		start = end + 1;
		end = line.find(' ', start);
		while (end != -1) {
			subjects[subj_size++] = line.substr(start, end - start);
			start = end + 1;
			end = line.find(' ', start);
		}
		subjects[subj_size++] = line.substr(start, end - start);
		Info* i = new Info(index, name, subjects, subj_size);
		this->insertKey(index, *i);
		delete[] subjects;
	}

	file.close();
	return true;
}

double HashTable::fillRatio() {
	return (double)num_keys / (num_bucket * bucket_capacity);
}

ostream& operator<<(ostream& os, HashTable& t) {
	for (int i = 0; i < t.num_bucket; ++i) {
		for (int j = 0; j < t.bucket_size[i]; ++j) {
			if (t.table[i][j]->key == -1) cout << " DELETED ";
			else os << *(t.table[i][j]) << "     ";
		}
		if (t.bucket_size[i] == 0) cout << " EMPTY ";
		cout << endl << endl;
	}
	return os;
}



int main() {
	int option = 1;
	Info* info = nullptr;
	int index, num_subj = 0;
	string name, subject;
	string* subjects = new string[N];

	HashTable* t = nullptr;
	AddressFunction* func = nullptr;
	int bucket_size, num_bits, p, q;

	string filename;
	Key k;


	string pr;

	while (option != 0) {
		cout << "Izaberite opciju: " << endl;
		cout << "1. Pravljenje nove hes tabele" << endl;
		cout << "2. Ucitavanje studenata iz fajla" << endl;
		cout << "3. Unos novog studenta sa standardnog ulaza" << endl;
		cout << "4. Pronalazenje kljuca" << endl;
		cout << "5. Brisanje kljuca" << endl;
		cout << "6. Isprazni tabelu" << endl;
		cout << "7. Broj kljuceva" << endl;
		cout << "8. Velicina tabele" << endl;
		cout << "9. Ispis tabele" << endl;
		cout << "10. Popunjenost tabele " << endl;
		cout << "0. Izlaz" << endl;
		cin >> option;
		switch (option) {
		case 1:
			cout << "Unesite velicinu jednog baketa ";
			cin >> bucket_size;
			cout << "Unesite broj bitova za indeksiranje tabele(parametar p) ";
			cin >> num_bits;
			cout << "Unesite parametar p za sekundarnu hes funkciju";
			cin >> p;
			cout << "Unesite parametar q za sekundarnu hes funkciju";
			cin >> q;

			func = new DoubleHashing(p, q);
			t = new HashTable(bucket_size, num_bits, func);
			break;

		case 2:
			cout << "Unesite naziv fajla ";
			cin >> filename;
			t->fillTable(filename);
			break;

		case 3:
			cout << "Unesite broj indeksa ";
			cin >> index;
			cout << "Unesite ime ";
			cin >> name>>pr;
			cout << "Unesite broj predmeta ";
			cin >> num_subj;
			cout << "Unesite predmete ";
			name += " " + pr;
			for (int i = 0; i < num_subj; ++i) {
				cin >> subject;
				subjects[i] = subject;
			}
			info = new Info(index, name, subjects, num_subj);
			if (t->insertKey(index, *info)) {
				cout << "Kljuc uspesno unet" << endl;
			}
			else {
				cout << "Kljuc vec postoji";
			}
			break;

		case 4:
			cout << "Unesite kljuc ";
			cin >> k;
			if ((info = t->findKey(k)) != nullptr) cout << *info << endl;
			else cout << "Kljuc ne postoji" << endl;
			break;

		case 5:
			cout << "Unesite kljuc ";
			cin >> k;
			if (t->deleteKey(k)) cout << "Kljuc ne postoji" << endl;
			else cout << "Kljuc uspesno obrisan" << endl;
			break;
		case 6:
			t->clear();
			break;
		case 7:
			cout << "Broj kljuceva je: " << t->keyCount() << endl;
			break;
		case 8:
			cout << "Broj ulaza u tabeli je: " << t->tableSize() << endl;
			break;
		case 9:
			cout << *t;
			break;
		case 10:
			cout << t->fillRatio() << endl;
			break;

		case 0:
			break;

		default:
			break;
		}

	}
	return 0;
}