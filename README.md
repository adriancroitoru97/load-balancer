# Copyright 2021 Adrian-Valeriu Croitoru

## Structuri de Date - Tema #2 - Load Balancer


*A fost folosita implementarea listei circulare dublu inlantuite(CDLL) generice
 realizata in cadrul laboratorului #2.
*A fost folosita implementarea hashtable-ului,
 realizata in cadrul laboratorului #4.
*A fost folosit headerul "utils.h" preluat din cadrul resurselor utile ale
 laboratorului.


*Fiecare SERVER este reprezentat in memorie ca o structura definita de
 un hashtable. In acest hashtable se vor stoca perechile (key, value)
 aferente fiecarui obiect de pe serverul respectiv. Tinand cont ca server-ul
 reprezinta, in esenta, un hashtable, toate operatiile din server.c sunt,
 practic, operatii pe hashtable.
*LOAD BALANCER-UL este reprezentat de o structura formata din 3 CDLL-uri.
 Prima - hashring-ul - reprezinta structura prin care se va face management-ul
 eficient al load-ului de pe fiecare server. Pe ea sunt stocate atat id-urile
 serverelor, cat si id-urile replicilor serverelor.
 Urmatoarele doua liste sunt utilizate pentru a stoca serverele efective.
 Astfel, servers contine adresele serverelor, iar servers_ids contine id-urile
 fiecarui server. Practic, aceste doua liste sunt sincronizate, facand legatura
 dintre adresa unui server si id-ul sau. O abordare mai eficienta si clara
 ar fi fost concatenarea acestor liste in una singura si crearea unei structuri
 pentru servere. S-a ales aceasta ultima varianta, insa s-a renuntat la ea
 din cauza unor erori si a timpului limitat pentru debug.


*Operatii:

 	### init_load_balancer
 		*Se initializeaza memoria load_balancer-ului, fiind folosita functia
 		dll_create pentru initializarea celor 3 liste aferente.

	### loader_store
		*Se utilizeaza functia find_server pentru a determina serverul pe care
		 trebuie stocat obiectul dat ca parametru. Daca obiectul nu se afla
		 deja pe acest server, este adaugat prin functia server_store.

	### loader_retrieve
		*Se utilizeaza functia find_server pentru a determina serverul pe care
		 se afla obiectul dat ca parametru. Se returneaza functia
		 server_retrieve, prin care este intoarsa valoarea efectiva (value) a
		 obiectului cu cheia key de pe serverul gasit anterior.

	### loader_add_server
		*Se adauga 3 noi pozitii pe hashring, aferente serverului efectiv, dar
		 si celor doua replici. Noul server este initializat, se adauga adresa
		 noului server in lista main->servers si id-ul acestuia in lista
		 main->servers_ids.
		*Se rebalanseaza obiectele de pe sistem. Astfel, pentru fiecare intrare
		 noua de pe hashring, se verifica acele obiecte care se aflau pe
		 serverul urmator serverului nou adaugat si care au hash-ul mai mic
		 decat hash-ul noului server. Acestea sunt obiectele ce vor fi mutate
		 pe acest server adaugat.

	### loader_remove_server
		*Se elimina cele 3 pozitii de pe hashring, aferente serverului efectiv,
		 dar si celor doua replici. Obiectele de pe serverul scos vor fi
		 redistribuite. Astfel, toate obiectele de pe vechiul server sunt iar
		 trecute prin loader si adaugate pe celelalte servere. In final,
		 se elimina serverul din liste (de adrese si de id-uri) si se da free
		 la memorie.

	### free_load_balancer
		*Se elibereaza memoria hashring-ului, se da free pentru fiecare server
		 in parte (functia free_server_memory), iar apoi se elibereaza memoria
		 ocupata de listele aferente serverelor (adrese + id-uri).
