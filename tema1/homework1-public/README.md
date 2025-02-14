# DATAPLANE ROUTER

Ciocea Bogdan-Andrei 323CA

## DISCLAIMER!!!
Tin sa spun ca am folosit urmatorul cod folosit in laborator cu scopul 
de a rezolva tema (se potrivea mult prea bine si nu cred ca sunt 
singurul care a facut asta):

```c
struct arp_table_entry *get_mac_entry(uint32_t given_ip)
{
	for (int i = 0; i < mac_table_len; i++)
	{
		if (mac_table[i].ip == given_ip)
		{
			return &mac_table[i];
		}
	}

	return NULL;
}
```

### * Procesul de dirijare (30p)

Pentru prima parte a programului, trebuie sa vorbim despre procesul 
de dirijare.

Aceasta dirijare este facuta cu ajutorul IPv4.

Se verifica daca pachetul are ca destinatar routerul insusi.

```c
if (inet_addr(get_interface_ip(interface)) == ip_header->daddr)
{
    icmp(interface, len, ip_header, eth_hdr, buf, ICMP_ECHO_REQUEST);
    continue;
}
```

Functia **inet_addr** face conversia dintr-o adresa normala/punctata 
intr-un format binar.

Daca intr-adevar pachetul are ca destinatar router-ul, se va trimite un 
pachet ICMP **ICMP_ECHO_REPLY**.

Apoi, pentru a nu face alte calcule redundante, se verifica daca campul 
**ttl** al pachetului este valid, daca nu, atunci se trimite un pachet 
ICMP cu mesajul **TIME_EXCEEDED**.

O sa luam cea mai buna ruta folosind functia **get_best_route**.

Pe urma se face verificarea **checksum**, unde, ca si in laboratorul 
4, nu prea se schimba.

Apoi decrementam campul **ttl**, ca daca nu facem asta ramane pachetul 
in aer in cel mai rau caz.

Atat in laborator, cat si in tema, trebuie sa cautam in tabela de 
rutare adresa IP destinatie ca sa stim urmatorul hop.

Acum, daca nu am gasit nicio intrare, o sa trimitem un pachet ICMP cu 
eroarea de **DESTINATION_UNREACHABLE**

Pe urma, se recalculeaza campul checksum.

Ca sa trimitem un pachet, avem nevoie sa ne folosim un pic de protocolul 
ARP.

O sa ne dorim sa ne luam adresa mac de la urmatorul hop folosind:
```c
struct arp_table_entry *mac;
        mac = get_mac_entry(route_entry->next_hop);
```
Daca nu exista, atunci pachetul este pus intr-o coada si se initiaza o
cerere ARP prin functia *arp_request*.
Daca exista, se pregateste pachetul *packet* care o sa fie trimis mai departe 

### * Longest Prefix Match eficient (16p)
Spre deosebire fata de laborator, nu avem voie cu o cautare liniara,
deoarece este ineficienta cand vorbim de o tabela mare de rutare. Ca sa facem 
aceasta cautare mai rapida, am ales sa folosesc cautarea binara iar 
complexitatea e O(logn) ceea ce e mult mai buna fata de O(n).
Pentru a intelege mai bine codul:

```c
struct route_table_entry *binary_search(uint32_t target_ip)
{
	struct route_table_entry *best_match = NULL;
	uint32_t left = 0;
	uint32_t right = rtable_len - 1;

	do
	{
		uint32_t mid = left + (right - left) / 2;

		if (rtable[mid].prefix == (target_ip & rtable[mid].mask))
		{
			best_match = &rtable[mid];
			if (left == right)
			{
				return best_match;
			}
			else
			{
				right = mid;
				left = 0;
				continue;
			}
		}

		if ((target_ip & rtable[mid].mask) <= rtable[mid].prefix)
		{
			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}

		if (left > original_right || right < 0)
		{
			break;
		}
	} while (left <= right);

	return best_match;
}

struct route_table_entry *get_best_route(uint32_t target_ip) {
    return binary_search(target_ip);
}
```

Functia **get_best_route** se foloseste de functia **binary_search**.
Acum, functia **binary_search** este o cautare binara normala, dar t
rebuie cautat adresa din tabela de rutare folosind principiul
**Longest Prefix Match**. Acum partea mai interesanta. Daca se 
indeplineste acest principiu, se returneaza ruta. Din enunt: 

```
"Este posibil ca mai multe intrări să se potrivească; în acest caz, 
routerul trebuie s-o aleagă pe cea mai specifică, i.e. cea cu masca cea 
mai mare."
```

Modul in care tabela de rutare a fost sortata este in ordine 
descrescatoare, atat din punctul de vedere al prefixelor cat si al 
mastilor. Asadar, daca practic nu s-a gasit vreo ruta initial, dar se 
indeplineste conditia din LPM, cautam in partea din stanga, intrucat 
acolo o sa fie mastile cele mai mari. In cazul cel mai defavorabil, 
returnam **NULL**, deoarece nu s-a gasit nimic ceea ce e destul de trist.

Pentru a sorta tabela de rutare, am decis sa folosesc **qsort**, intrucat are
o complexitate decenta, iar criteriul de comparare a fost in ordine
descrescatoare a prefixelor, apoi a mastilor.

### * Protocolul ARP (33p)
Acest protocol se gaseste atat in functia **main**, cat si in functiile 
**arp_reply** si **arp_request** (evident)...

O sa incepem cu partea din main si dupa la celelalte, intrucat ultimele 
2 sunt cam la fel.

Acum, cand noi vrem sa verificam daca un pachet este de tip ARP, o sa 
avem asa:
```c
if (eth_hdr->ether_type == htons(ARP_TYPE))
```

Daca intr-adevar acest pachet este de tip ARP, o sa vrem sa adaugam 
adresa MAC a emitatorului in tabela noastra de ARP, daca nu exista deja. 
Aceasta se face pentru a putea raspunde la viitoarele cereri ARP sau 
pentru a trimite pachete catre aceasta adresa.
```c
memcpy(mac_table[mac_table_len].mac, arp_received->sha, 6 * sizeof(uint8_t));
mac_table[mac_table_len++].ip = arp_received->spa;
```
Apoi, o sa verificam tipul operatiei ARP:

Daca este cerere, o sa trimitem un raspuns folosind 
**arp_reply**. Daca nu, atunci este un raspuns, deci o sa 
trebuieasca sa luam elementele din coada cu elemente si sa le incapsulam 
in pachete ca dupa aceea sa fie trimise. Dar, inainte de toate, trebuie 
sa gasim cea mai buna ruta pentru a trimite pachetele:
```c
struct route_table_entry *best_route = get_best_route(ip_header->daddr);
```
Apoi le trimitem pur si simplu.

Pentru **arp_request**:
Avem nevoie de a seta adresa pachetului ca fiind **FF:FF:FF:FF:FF:FF**. 
Aceasta adresa o vom stoca in variabila **broadcast_mac**. Aceasta 
adresa o sa fie mai tarziu pusa in antetul Ethernet **eth_hdr**.

O sa luam adresa mac al urmatorului hop si o sa il punem ca si adresa 
sursei. Apoi o sa ii precizam tipul fiind ARP:

```c
eth_hdr->ether_type = htons(ARP_TYPE);
```

O sa avem nevoie sa alocam memorie pentru un pachet ARP cat si pentru un antet 
ARP caruia sa ii populam campurile. Pachetul ARP o sa fie trimis mai departe, 
odata ce au fost copiate toate datele din antetul Ethernet si din ARP.

Apoi, acest pachet o sa fie trimis:

```c
int total_length = sizeof(struct ether_header) + sizeof(struct arp_header);
send_to_link(route_entry->interface, arp_request_packet, total_length);
```
Pentru functia **arp_reply**, principiul e aproape acelasi.

### * Protocolul ICMP (21p)
Acest protocol este mai usor decat cel ARP, deci nu o sa fie o descriere asa de 
lunga.

Tot ce avem de facut aici e:
*  Sa gasim cea mai buna ruta
*  Sa populam antetul IP
*  Sa configuram tipul pachetului ICMP
*  Sa umplem sursa antetului Ethernet cu adresa MAC
*  Sa umplem campurile antetului ICMP
*  Sa umplem pachetul care trebuie trimis
*  Sa trimitem pachetul
