Ciocea Bogdan-Andrei 323CA

                        Tema 2 - Client-Server TCP si UDP

* Descriere client

    Clientul a fost foarte usor de implementat, intrucat nu erau chestii foarte
complexe la el, desi pe partea de primire inapoi de la server a raspunsului nu
a mers din prima, dar am rezolvat si totul a fost ok.
    Intrucat nu au fost chestii foarte complicate, nici nu a fost refolosit cod,
am decis sa scriu totul in functia main. In prima parte folosim functia
"setvbuf" pentru a dezactiva buffering-ul la afisare asa cum s-a cerut in enuntul
temei. Apoi am obtinut un socket pentru conectarea cu serverul. Dupa, am completat
in server_addr adresa serverului, familia de adrese si portul pentru conectare.
Apoi pentru a ma conecta la server am folosit functia "connect" si am verificat
daca merge. Apoi, odata ce am vazut ca merge, trebuie sa trimit ID-ul clientului
la server ca acesta sa il salveze in lista lui de clienti. Pe urma, pentru
multiplexare, am folosit POLL si am umplut structura pollfd cu datele necesare
pentru a astepta raspunsul de la server. Dupa ce am primit raspunsul, am afisat
mesajul primit si am iesit din program. Intrucat am comentarii puse aproape
la fiecare rand din cod, nu o sa trec la foarte multe detalii aici.
Foarte pe scurt, am sectionat in 3 parti: stdin, mesaje de la server
si mesaje de la UDP. Pentru fiecare, am cazuri speciale care sunt detaliate in
codul sursa. In final, s-a eliberat socket-ul si s-a iesit din program.

* Descriere server

    Serverul, evident, a fost cel mai complicat de implementat. Intrucat trebuia
sa ne facem o structura de client si o structura de topic, am decis sa folosesc
doua liste, una de clienti si una de topicuri. Fiecare client, intuitiv, are
o singura lista mare/mica (depinde de ce are) de mesaje. Eu am zis ca o sa o numesc
topic, intrucat e mai usor de inteles: fiecare client trebuie sa fie abonat
la un topic anume, ca are nume, tip, continut nu are relevanta momentan, eu asa
am numit-o (deal with it). Acum, structura de topic are cateva campuri necesare
date de cerinta temei, cat si 2 campuri pentru a fi mai usor de folosit. Unul
este un pointer catre urmatorul topic (evident daca avem o lista), iar celalalt
este un boolean "has_wildcard" care ne spune daca acel topic are in nume un
wildcard sau nu. Este foarte important sa stim asta inca de la bun inceput,
pentru a ne face implementarea mai eficienta, chiar daca o sa avem mai multa
memorie in structura noastra. Pentru a nu pierde puncte la aceasta tema referitor
la descrierea protocolului, am decis sa fac un mic rezumat. Structura mesajului
este format din nume, care are 51 de octeti, pointer la urmatorul nod din mesaj,
un int care ne spune de care tip e, un boolean care sa ne spuna daca acel nume
are un wildcard si, de asemenea, continutul mesajului care are 1500 de octeti asa
cum e specificat in textul temei. Structura clientului este formata din ID-ul sau,
socket-ul sau, un pointer catre urmatorul client, un pointer catre lista de topicuri
la care e abonat, un pointer catre adresa sa si un boolean care verifica daca e
conectat.
    Ok. acum ca ne-am familiarizat cu structurile, sa trecem la implementare.
Ca o prima functie avem functia "add_client" care adauga un client in lista de
clienti a serverului. Aceasta functie primeste ca parametrii ID-ul clientului,
socket-ul sau, structura sockaddr_in a clientului si adauga clientul in lista pe
nume clients. Aceasta functie poate sa aiba 3 returnari, 1 daca clientul a fost
adaugat cu succes, -1 daca acest client este deja conectat pe server si 2 daca
acest client a fost conectat si inregistrat pe lista de clienti, dar acesta s-a
deconectat si urmeaza sa fie reconenctat. Aceasta functie este una simpla, intrucat
tot ce face este ca prima data sa se uite pe lista de clienti si sa vada daca
acesta este deja conectat sau daca a fost conectat si deconectat. Daca nu, il
adauga in lista de clienti si, pe urma, o sa se transmita un mesaj de 
conectare a noului client asa cum se cere in enuntul temei. Foarte important
aici este ca o sa dezactivam algoritmul lui Nagle pentru a nu avea probleme cu
buffering-ul folosind functia "setsockopt". Dupa ce am facut asta, o sa trimitem
mesajul de conectare a noului client.
    Urmatoarea functie este "remove_client" care, tot ce face, este sa nu stearga
propriu-zis un client, ci mai mult sa il deconecteze, intrucat noi vrem sa
pastram informatiile despre acesta (id-ul si topicurile la care e abonat). Acesta
o sa schimbe campul de conectat al clientului in "false", o sa inchida socket-ul
si o sa afiseze pe terminalul serverului ca acel client este deconectat. Foarte
simplu si usor.
    Urmatoarea functie este cam tot din aceeasi categorie, insa aceasta chiar
sterge un client, mai multi chiar. Aceasta functie este "free_client_list" care
primeste lista de clienti ca parametru si o sterge (wow, cine ar fi crezut). Dar,
in timp ce se sterg clientii, de fiecare data o sa se inchida socket-ul si o sa
se transmita in terminalul clientului respectiv un mesaj ca se va inchide serverul.
Aceasta functie este utila atunci cand o sa vrem sa, ghici, inchidem serverul.
    O alta functie care ne ajuta la niste sarcini este functia "notify_all_clients".
Aceasta functie este o generalizare a trimiterii unui mesaj la toti clientii.
In aceasta tema, functia este folosita doar cand serverul se opreste, dar, insa
poate sa fie folosita si cu alte scopuri cum ar fi de a trimite reclame la toti
clientii, sau mesaje la un anumit timp cum a fost in laboratorul 7.
    Acum o sa ne uitam la 2 functii care sunt 2 fete ale aceleiasi monede.
Avem "convert_type" si "reverse_convert_type". Prima functie primeste un int
si returneaza un char, iar a doua functie primeste un char si returneaza un int.
Prima functie face conversia de la tipul de date al mesajului dat de clientul UDP,
pentru a se pune in mesajul procesat in timpul trimiterii, pe cand a doua functie
face conversia inversa, intrucat in structura noastra a mesajului, tipul de date
este un int, nu un char. Aceasta alegere a fost formata pe baza faptului ca
putem avea mai multe tipuri de mesaje, unele ale caror denumiri pot depasi 4
octeti, deci ne poate fi mai usor sa folosim un int pentru a reprezenta tipul
de mesaj.
    Urmatoarea functie este "match_topic" care primeste 2 string-uri si verifica
daca acestea se potrivesc. Aceasta functie este folosita pentru a verifica daca
un client este abonat la un topic sau nu. Aceasta verificare este facuta in
functie de faptul daca topicul la care e abonat clientul are sau nu un wildcard.
In cazul in care are, atunci se verifica daca string-urile se potrivesc pana la
wildcard, iar in caz contrar, se verifica daca string-urile sunt identice. Sa
intram la o descriere mai in detaliu: aceasta functie prima data se va duce
la urmatorul wildcard din topicul la care este abonat clientul. Daca nu exista,
atunci se va duce la finalul string-ului si se va verifica daca string-urile
sunt identice. Daca exista, atunci se va face o verificare dupa wildcard. Daca
avem un wildcard cu simbolul "+", atunci ne vom deplasa pana cand gasim un caracter
care sa nu fie "+". Daca avem un wildcard cu simbolul "*", atunci vom verifica
recursiv daca string-urile se potrivesc pana la urmatorul wildcard. Daca nu se
potrivesc, atunci se va verifica daca string-urile sunt identice.
    Urmatoarea functie este "process_udp_message", care scopul ei este sa
primeasca de la clientii UDP mesajel si sa le dea mai departe clientilor TCP
care sunt abonati la topicul respectiv. Prima data trebuie sa luam mesajul de
la clientul UDP si sa il procesam. Asadar, o sa folosim functia "recvfrom" pentru
a primi mesajul de la clientul UDP. Dupa ce am primit mesajul, o sa il procesam.
Asta inseamna ca, prima data, o sa luam topicul mesajului, adica primii 50 de
caractere. Dupa aceea, o sa luam tipul, doar ca tipul o sa ne apara sub forma
de intreg, asa ca o sa avem nevoie sa facem o conversie de la int la char*.
Dupa ce am facut asta, o sa luam continutul mesajului in functie de tipul oferit.
Aici o sa aplicam logica din enuntul temei. Nu cred ca e nevoie sa intru in
detalii aici, intrucat e destul de simplu. Dupa ce am procesat si continutul, o
sa lipim topicul, tipul si continutul intr-un singur mesaj si o sa il trimitem
clientilor TCP care sunt abonati la acel topic. Pentru a face asta, o sa
verificam cu functia "match_topic" daca clientul este abonat la acel topic. Daca
da, atunci o sa trimitem mesajul. Daca nu, o sa trecem la urmatorul client.
Intrucat noi o sa verificam de la inceput daca ce s-a luat de la UDP are un
continut, o sa verificam daca mesajul are continut. Daca nu are, atunci o sa
trimitem un mesaj de eroare "Failed to receive message".
    Urmatoarea functie este "subscribe" care primeste comanda de subscribe si
clientul care vrea sa se aboneze la acel topic. Inainte de a face altceva,
trebuie sa procesam si comanda data, intrucat noi o primim sub forma "raw",
adica trebuie sa o structuram cumva. Asa o sa si facem. Luam topicul mesajului,
luam tipul mesajului, iar pe urma continutul. Apoi, o sa verificam daca acest
client este deja abonat, daca da, atunci o sa intoarcem valoarea -1 care ne
sugereaza asta mai departe in program. Daca nu, atunci e bine. O sa verificam
daca acest mesaj are un wildcard ascuns pe undeva ca sa nu avem surprize, dupa
care o sa il punem in lista de topicuri a clientului. O sa il punem fix in fata
ca sa nu ne chinuim foarte mult. Dupa aceea, o sa trimitem o confirmare catre
client ca s-a abonat cu succes la acel topic.
    Urmatoarea functie este "unsubscribe" care primeste comanda de unsubscribe
si clientul care vrea sa se dezaboneze de la acel topic. Aceasta functie este
foarte asemanatoare cu functia de subscribe, doar ca aici o sa stergem clientul
din lista de topicuri a clientului. Dupa ce am facut asta, o sa trimitem un
mesaj de confirmare catre client ca s-a dezabonat cu succes de la acel topic.
Foarte important aici, daca clientul nu e abonat la acel topic, o sa trimitem
un mesaj de eroare catre client "Topic not found".
    Urmatoarea functie este una de debug numita "show", care, ce face, este sa
putem vedea prin terminalul serverului, la ce topicuri este abonat un client.
Aceasta functie ajuta sa vedem daca intr-adevar clientul s-a abonat la topicul
respectiv sau nu.
    Urmatoarea functie este "accept_connection", care intoarce un file
descriptor daca client TCP vrea sa se conecteze la server. Desi este separata
de restul codului, am ales sa o pun ca si functie intrucat se poate generaliza
pentru orice tip de server, nu doar pentru serverul nostru.
    Urmatoarea functie pana in functia main este "get_client_by_fd" care
este facuta din cauza ca noi o sa avem nevoie de pointerul clientului de fiecare
data cand vrem sa facem ceva cu clientul in sine. Deci, ca sa nu avem cod copy
paste, am ales sa fac o singura functie si sa o apelam de fiecare data cand este
nevoie. Aceasta functie verifica in lista de clienti file descriptor-ul dat.
Daca gaseste un client cu acelasi file descriptor, intoarce pointerul catre
clientul respectiv. Daca nu, intoarce NULL.
    Si acum am ajuns in logica din functia main. A fost un maraton pana acum,
dar ne indreptam inspre final ceea ce e bine. Deja am spus functionalitatea
functiilor, deci o sa trecem mai usor peste aceasta parte.
    Prima data o sa dezactivam buffering-ul la afisare, apoi o sa obtinem un
port dat de stdin. O sa ne initializam lista de clienti cu NULL. Dupa, o sa
initializam socket-ul si o sa ne conectam la server. Dupa ce am facut asta, o
sa completam  campurile structurii sockaddr_in cu adresa serverului, familia
de adrese si portul. Pe urma, o sa legam socket-ul UDP si TCP la adresa
serverului. Apoi, o sa ascultam pe socket-ul TCP. Dupa ce am facut asta, o sa
initializam structura pollfd si o sa bagam pe primele 3 pozitii stdin, socket-ul
TCP si socket-ul UDP. Dupa ce am facut asta, o sa procedam cum a fost si in
laboratorul 7 de PCOM. La stdin o sa verificam doar daca serverul o sa fie oprit.
La socket-ul TCP o sa verificam daca se va conecta un client nou. La socket-ul
UDP o sa verificam daca o sa primim un mesaj de la un client UDP care o sa fie
procesat in functia "process_udp_message". Daca intampinam un alt eveniment
fata de aceste 3, o sa consideram ca primim mesaje de la clientii TCP. O sa
citim mesajul si o sa ne directionam in functie de el. Daca acel mesaj nu are
un continut anume, atunci inseamna ca acel client TCP se va deconecta (nasol).
Daca nu, atunci o sa vedem ce tip de mesaj e, ori subscribe, unsubscribe, show.
In cazul in care subscribe returneaza -1, atunci o sa trimitem un mesaj de
"eroare" catre client. Daca nu, atunci o sa trimitem un mesaj de confirmare.
Ultima parte este sa resetam buffer-ul si sa eliberam memoria alocata pentru
clienti. Dupa ce am facut asta, o sa inchidem socket-ul si
o sa iesim din program.

Si cam asta e. Sper ca a fost ok cat de cat descrierea. *smiley face*.
Multumesc pentru rabdare.

A si...mai avem inca ceva:
* Observatii

1. Trebuia sa scriu acest cod in C++, intrucat scriam mult mai putine randuri,
dar cum de e obisnuinta sa scrii cod in C, am ales calea grea.
2. E o tema foarte interesanta, buna de pus la CV.
3. Pentru debugging, ca sa nu interferez cu rularea programului, am folosit
"show" pentru a afisa la ce sunt eu abonat ca si client, dar, repet, nu are
legatura cu rularea programului.

Gata acum s-a sfarsit.









































Sau nu? To be continued.....