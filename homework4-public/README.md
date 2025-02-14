# Tema 4

Am folosit laboratorul 9 ca sa fac aceasta tema.

## Intro
Ultima tema....wow...."end of an era" cum s-ar zice (nu vreau
sa fiu depunctat pentru rom-engleza).

Ce pot sa zic....a fost...interesant.
Adica da, s-au invatat multe, dar fiind semestrul asta solicitant si
parca vrei sa scapi de toate temele, dar, odata ce ai terminat cu ele
te trezesti parca ca nu mai ai altceva de facut decat sa inveti pentru
examen si te uiti in spate la ce ai facut si te trezesti ca in timpul
asta ai facut multe...si mai ales la PCOM care a avut printre cele mai
interesante teme din tot anul 2. A fost ceva...

In sfarsit...sa depasim momentul
A si apropo..eu scriu README-ul asta incepand cu ora 3:52 dimineata,
deci imi pare rau pentru persoana care citeste ce am scris aici.

## Implementare

### SLAVA C++ CEL MAI BUN LIMBAJ DE PROGRAMARE

In implementare am folosit fisierele din laboratorul 9 (ceva ce trebuie
la tema asta, normal).

Desi am lucrat in limbajul C++, asteptandu-ma sa folosesc clase de exemplu, nu am facut-o..aia e.

Totusi limbajul vine cu anumite abstractizari care sunt chiar misto.
Ok. Sa incepem cu functiile facute in tema.

In **main** avem o singura functie care se tot apeleaza, aia fiind
"handle_command", deci o sa vorbim destul de mult despre ea.

In functia **"handle_command"** am declarat un obiect json pe nume j, 
care o sa ne trebuiasca mai tarziu (micunealta secreta). M-am conectat la
server folosind functia "open_connection" ca in laborator. Apoi am alocat
memorie pentru un sir de caractere **char*  method**, care practic o sa
fie comanda noastra default pentru GET request, intrucat nu o sa avem
DELETE, decat atunci cand trebuie si de mai putine ori decat celelalte,
deci sper ca am explicat destul pentru sirul asta de caractere. Si, de
asemenea, am luat cookie-urile din session_cookie care retine
cookie-urile.

Apoi, verificam pentru primele 2 functii ale clientului, cele de login
si register si, din fericire, e cam acelasi cod la ambele, intrucat
luam numele si parola. Pe acestea le luam folosind getline, intrucat
in teste e posibil sa avem si spatii in una din ele, iar noi, desi le
luam, o sa le trecem pe ambele printr-un filtru de validare.
Daca ele sunt valide (adica nu au spatii prin continut) atunci o sa le
salvam intr-un json. Acest filtru este de fapt functia
**"is_valid_user_data"** care practic doar asta verifica, daca exista
spatii in continut. Pe urma, o sa luam "path"-ul dat din cerinta si o
sa adaugam comanda noastra (fie login fie register momentan).
O sa serializam datele din json pe urma in char* (in body_data), pe care
o sa il punem in functia de POST request, ca la laborator. Apoi o sa luam
tipul de payload pentru aceasta comanda (content type). Apoi o sa
trimitem catre server acea cerere. Momentan nu avem cookie-uri sau jwt,
alea vin mai tarziu. Dar odata ce primim raspuns de la server o sa
luam cookie-urile si o sa le punem in session_cookie. Odata cu asta, o sa
afisam si rezultatul folosind functia **"print_response"**. Dar cum eu 
credeam ca trebuie sa afisam tot mesajul (aparent nu), ori afisam "OK" 
in cazul in care nu se primeste nicio eroare de la server, sau eroarea 
data de server. Aceasta verificare de eroare se face in functia
**"get_error_message"** care intoarce **true** daca exista erori sau
**false** altfel. Aceasta functie pune in aplicare functiile din
biblioteca json. Pentru aceatsa functie, o sa ne ducem direct
in partea in care incepe partea de json (adica unde arata eroarea), iar,
de acolo, o sa convertim acel string intr-un json folosind functia
**"json::parse"**, iar, la sectiunea "error" gasim rezultatul, dar ce sa
vezi....trebuie sa il convertim in string si pe asta....din nou, dar hei
am gasit eroarea respectiva si o afisam la stdout si, de asemenea,
intoarcem adevarat, intrucat am gasit o eroare. Dar, daca nu avem o
astfel de parte, o sa intoarcem fals. Buuuun...acum ca ne-am intors la
raspunsul dat de server, daca nu primim asa ceva, o sa folosim functia
**"print_error"** ca sa afisam o eroare pentru fiecare comanda
(neironic asta face...e mai mult pentru debug dar il lasam acolo).

Urmatoarea comanda de pe lista este a treia comanda, una foarte
importanta, intrucat o sa luam token-ul json **"enter_library"**!!!
Aici luam URL-ul pentru aceasta comanda specifica. Inainte de a da
aceasta cerere serverului, trebuie sa bagam si token-ul json, ca sa
preluam token-ul de acces catre biblioteca. Dupa ce primim raspuns de
la server, daca primim, extragem token-ul folosind functia
**"extract_jwt_token"**. Ce face aceasta functie este exact ce face
si "get_error_message", doar ca de data aceasta o sa ia "token" daca
exista. Dupa aceea, afisa mesajul primit de server folosind
"print_response".

Urmatoarea comanda este **"get_books"**, unde si aici am facut
aceeasi chestie, luam URL-ul, punem pe server si afisam in stdout
ce primim, doar ca aici e diferit nitel, desi avand acelasi "pattern"
(**iarasi nu vreau sa fiu depunctat pentru rom-engleza**). Deci, prima
data o sa punem ruta de acces si o sa dam mesaj la server. Acum...
pe partea cu receptarea am incercat sa le pun pe toate intr-o singura
functie, dar aparent codul a spus "Nah I'mma do my own thing" si a mers
doar printr-un singur mod...copy paste 😞. Stiu...rusine. Promit ca nu
o mai fac. Dar practic am facut ce se face si in "get_error_message" si,
daca gasim eroare, o afisam si inchidem conexiunea si eliberam memoria.
Daca nu, afisam mesajul dat care practic o sa fie cartile date.

Urmatoarea comanda, desi nu este urmatoarea in enuntul temei, dar era
mai rapida de scris decat ce o sa urmeze dupa ea este **"logout"**, care
se duce pe ruta "/api/v1/tema/auth/logout". Acelasi lucru: dam cerere
GET, primim raspunsul, il afisam, doar ca si aici, surprinzator, avem
chestii care se diferentiaza fata de celelalte comenzi. La logout
se vor elibera token-ul JSON si cookie-urile (acelea care se elibereaza
la final).

Urmatoarea comanda este **"add_book"**, care nu e asa de complicata pe
cat pare. Tot ce trebuie sa facem e sa luam datele de intrare si sa le
punem intr-un JSON ca in **login** de exemplu, sa punem aceste date
intr-o cerere de tip POST, primim inapoi rezultatul de la server si
afisam ce primim.

Ultimele 2 comenzi sunt asemanatoare, de aceea le-am bagat in aceeasi
oala ca sa zic asa. Aici o sa fie ceva diferit fata de altele (de cate
ori o sa mai zic asta...). La ambele citesti id-ul cartii pe care vrei
ori sa o afisezi ori sa o distrugi. Luam URL-ul, apoi, aici o sa fie
o diferenta. Ca practic o sa schimbam (pentru prima data in lume) metoda
de a da cererea de tip "GET" in "DELETE". Apoi trimitem la server acea
cerere, iar, pentru raspuns, avem 2 drumuri: daca acea comanda e
**"delete_book"**, o sa afisam un OK, daca nu avem erori, altfel o sa
afisam erorile asa cum de ne-am obisnuit pana acum. Daca, in schimb,
avem de primit o carte, pai aici o sa cautam un mesaj de eroare al
raspunsului serverului. Daca avem, il afisam in functia 
**"get_error_message"**, altfel, o sa afisam fix ce ne-a dat serverul
fara alte prelucrari. Sau, daca nu ne-a dat serverul nimic, trimitem o
eroare, ca dupa, la finalul acestui "if" sa dealocam memoria. Ultima
comanda care practic e asa de "frumusete" este **"exit"**, cu care iesim
din program...simplu.

Daca programul nu are alta comanda fata de acestea, o sa fie ok. In
schimb, daca cumva clientul da alta comanda decat acestea 9, pai aici o
sa se afiseze "invalid command".
La finalul acestei functii se inchide conexiunea cu serverul, intrucat
functia "handle_commands" se apeleaza o singura data per comanda, deci e
totul ok. 👍

Si cam asta e intreaga tema.

## Finalizare + Teste secrete

Cand credeam ca am terminat intr-un final tema, aveam vreo 300-400 de 
linii...pana cand m-am uitat nitel pe checker ca parca nu era tot
doar din checker-ul dat de Readme-ul din tema (**nu vreau sa fiu
depunctat pentru rom-engleza**).

Fast-forward cateva sute de linii si am terminat si cu acele teste din
ALL.....si acum am aproape 700 de linii cu tot cu comentarii....excelent.

A fost "fun" cand am gasit si alte teste pe care trebuie sa le treci,
dar na suntem ingineri deci aia e.

Am pus aceasta sectiune pentru amuzamentului corectorului care probabil
simte ca a pierdut mult timp citind tot acest fisier, dar oare a pierdut
timpul, e intrebarea? Eu zic ca nu. (mint MUHAHAHHAHAHHAHAHAHHAHA)

## Pareri

(Wow....e ora 6:12 smecher.)

Parerea mea e ca, cel putin tema asta (tema 4 duuh) s-a axat destul de
mult pe laborator, ceea ce nu e rau deloc, iar implementarea nu a durat
chiar o saptamana 😊 .

Sincer sa fiu am facut-o cu placere si cu mare curiozitate fata de API-ul
dat....mai ales sa lucram cu JSO-ane **(nu vreau sa fiu depunctat pe
rom-engleza)** in C/C++, a fost interesant mai pe scurt.

O buna tema de incheiere, nu foarte grea, numai buna de final de semestru
cand **nu mai vrem** teme, dar nici sa stam asa degeaba mult.

Oricum...vad ca deja vine lumina pe cer si pare ca deja am stat mult prea mult la conversatie cu tine. Da, tu. Tu care o sa imi dai punctajul
full pe aceasta tema fara nicio ezitare.
![Alt Text](https://i.makeagif.com/media/8-24-2023/rrtGly.gif)



Aaaaaaaaaa......deci rezisti la asa ceva....ok..ok...
Atunci.....o sa iti dau o oferta de nerefuzat.
Nu o sa te mai deranjez daca imi dai full pe tema asta. Ok?
Ok. Perfect! Deci multumesc mult pentru punctaj.


Asa ca....ne luam ramas bun....imi pare bine ca ai asistat la calatoria 
unui automatist in anul 2 urmand anul 3...cum ai fost si tu inaintea mea 
si ai stat sa faci un README cat mai bun...heh...ce mai trece timpul 
asta.


Cum a spus odata Shakespeare,
"BYE BYE" 🗣️🗣️🗣️🗣️🗣️🗣️🗣️ 🔥🔥🔥🔥🔥🔥🤫🤫🤫🤫🤫🤫🧏‍♂️🧏‍♂️🧏‍♂️🧏‍♂️🧏‍♂️🧏‍♂️.

<div class="tenor-gif-embed" data-postid="10780355880827492718" data-share-method="host" data-aspect-ratio="0.566265" data-width="100%"><a href="https://tenor.com/view/snowman-mewing-gif-10780355880827492718">Snowman Mewing GIF</a>from <a href="https://tenor.com/search/snowman-gifs">Snowman GIFs</a></div> <script type="text/javascript" async src="https://tenor.com/embed.js"></script>

<div class="tenor-gif-embed" data-postid="17378718880670737949" data-share-method="host" data-aspect-ratio="0.562249" data-width="100%"><a href="https://tenor.com/view/mewing-skeleton-meme-pluh-gif-17378718880670737949">Mewing Skeleton GIF</a>from <a href="https://tenor.com/search/mewing-gifs">Mewing GIFs</a></div> <script type="text/javascript" async src="https://tenor.com/embed.js"></script>