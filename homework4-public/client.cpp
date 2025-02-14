#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "json/nlohmann/json.hpp"
#include "helpers.h"
#include "requests.h"

using json = nlohmann::json;
using namespace std;

const char *HOST = "34.246.184.49";
#define PORT 8080
#define URL_SIZE 1000

#define forever while (true) // pentru ultima oara din pacate *sad face*

/**
 * Variabile globale pentru a retine cookie-ul de sesiune si token-ul JWT
*/
string session_cookie;
string jwt_token;

/**
 * Functie care extrage token-ul JWT din raspunsul primit de la server
 * @param response - raspunsul primit de la server
 * @return token-ul JWT sau un string gol cu {}
 */
string extract_jwt_token(string response)
{
	/**
	 * Cautam pozitia de inceput a JSON-ului in raspuns
	 */
	size_t pos = response.find("{");
	if (pos != string::npos)
	{
		/**
		 * https://stackoverflow.com/questions/50011977/how-do-you-get-a-json-object-from-a-string-in-nlohmann-json
		*/
		json j = json::parse(response.substr(pos));
		/**
		 * Extragem token-ul
		 */
		if (j.find("token") != j.end())
		{
			return j["token"].get<string>();
		}
	}
	return "{}";
}

/**
 * Converteste jwt_token din string in char*
 * @param j - token-ul JWT
 * @return token-ul JWT dar convertit in char*
 */
char *get_json_token(string j)
{
	if (j.empty())
	{
		return NULL;
	}
	else
	{
		return strdup(j.c_str());
	}
}

/**
 * Functie care afiseaza un mesaj de eroare si il afiseaza cu cuvantul cheie
 * "error" ca sa arate cu rosu la checker
 * @param message - mesajul de eroare
 */
void print_error(string message)
{
	if (message == "login" || message == "register")
	{
		cout << "error: logging in" << endl;
	}
	else if (message == "enter_library")
	{
		cout << "error: entering library" << endl;
	}
	else if (message == "get_books")
	{
		cout << "error: getting books" << endl;
	}
	else if (message == "get_book")
	{
		cout << "error: getting a book" << endl;
	}
	else if (message == "add_book")
	{
		cout << "error: adding a book" << endl;
	}
	else if (message == "delete_book")
	{
		cout << "error: removing a book" << endl;
	}
	else if (message == "logout")
	{
		cout << "error: logging out" << endl;
	}
}

/**
 * Functie care afiseaza mesajul de eroare primit de server
 * intors catre client
 * @param response - raspunsul primit de la server
 * @return true daca exista eroare, false altfel
*/
bool get_error_message(char *response) {
	string json_part(response);
	size_t pos = json_part.find("{");
	if (pos != string::npos)
	{
		/**
		 * https://stackoverflow.com/questions/50011977/how-do-you-get-a-json-object-from-a-string-in-nlohmann-json
		*/
		json j = json::parse(json_part.substr(pos));
		/**
		* Extragem token-ul
		*/
		if (j.find("error") != j.end())
		{
			cout << "error: " << j["error"].get<string>() << endl;
			return true;
		}
	}

	return false;
}

/**
 * Functie pentru a verifica daca numele unui user e valid
 * (adica daca nu are spatii din cate am vazut eu in teste)
 * @param username - numele user-ului
 * @param password - parola user-ului
 * @return true daca sunt valide, false altfel (adica daca are spatii)
*/
bool is_valid_user_data(string username, string password)
{
	for (char c : username)
	{
		if (isspace(c))
		{
			return false;
		}
	}

	for (char c : password)
	{
		if (isspace(c))
		{
			return false;
		}
	}
	return true;
}

/**
 * Functie care afiseaza un mesaj de raspuns sau o eroare daca nu exista
 * (ca sa nu dau copy-paste de atatea ori)
 * @param response - raspunsul primit de la server
 * @param message - mesajul
 */
void print_response(char *response, string message)
{
	if (response)
	{
		if (get_error_message(response)) {
			return;
		}

		if (message == "login") {
			cout << "200 - OK - Bun venit!" << endl;
		} else if (message == "register") {
			cout << "200 - OK - Utilizator înregistrat cu succes!" << endl;
		} else if (message == "enter_library") {
			cout << "200 - OK - Ai intrat cu succes in biblioteca" << endl;
		} else if (message == "add_book") {
			cout << "200 - OK - Cartea s-a adaugat cu succes!" << endl;
		} else if (message == "delete_book") {
			cout << "200 - OK - Cartea s-a sters cu succes!" << endl;
		} else if (message == "logout") {
			cout << "200 - OK - Utilizatorul s-a delogat cu succes!" << endl;
		} else {
			cout << "200 - OK" << endl;
		}
	}
	else
	{
		print_error(message);
	}
}

/**
 * Functie care se ocupa de comanda data de client
 * @param command - comanda
 */
void handle_command(string command)
{
	json j;
	int sockfd;

	/**
	 * Ne conectam cu serverul
	 */
	sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		cerr << "Failed to connect to the server." << endl;
		exit(1);
	}

	char *response;
	char *method = (char *)calloc(7, sizeof(char));
	if (!method) {
		cerr << "Failed to allocate memory for method." << endl;
		exit(1);
	}
	/**
	 * Aici avem GET deoarece majoritatea request-urilor sunt GET practic
	 * doar la DELETE o sa ii schimbam datele metodei
	*/
	strcpy(method, "GET");

	/**
	 * Luam cookies-urile
	*/
	char *cookies[1] = {&session_cookie[0]};

	if (command == "login" || command == "register")
	{
		/**
		 * Aici o sa citim datele
		 */
		cin.ignore();

		cout << "username=";
		string username;
		getline(cin, username);

		cout << "password=";
		string password;
		getline(cin, password);

		if (!is_valid_user_data(username, password)) {
            cout << "error: invalid user data" << endl;
			close(sockfd);
			free(method);
            return;
        }

		/**
		 * Le bagam elegant datele in JSON :))
		 */
		j["username"] = username;
		j["password"] = password;

		/**
		 * Hardcodare++
		 */
		string fullPath = "/api/v1/tema/auth/" + command;
		/**
		 * Deci aici o sa convertim JSON-ul in char* ca sa fie mai usor de
		 * trimis, nicio magie neagra aici
		 */
		char *body_data[1] = {strdup(j.dump().c_str())};

		/**
		 * Hardcodare++ x2
		 */
		char *content_type = (char *)calloc(URL_SIZE, sizeof(char));
		if (!content_type) {
			cerr << "Failed to allocate memory for content_type. #1" << endl;
			exit(1);
		}
		strcpy(content_type, "application/json");

		/**
		 * Aici trimitem POST request catre server
		 */
		send_to_server(sockfd, compute_post_request(strdup(HOST),
													strdup(fullPath.c_str()),
													content_type, body_data, 1,
																NULL, 0, NULL));

		/**
		 * Aici o sa vedem daca primim mesaj de la server
		 */
		char *response = receive_from_server(sockfd);
		if (response)
		{
			print_response(response, command);

			/**
			 * Aici o sa cautam cookie-ul de sesiune...o smecherie aici hehe
			 */
			char *cookie_pos = strstr(response, "Set-Cookie: ");
			if (cookie_pos)
			{
				cookie_pos += strlen("Set-Cookie: ");
				char *cookie_end = strstr(cookie_pos, ";");
				if (cookie_end)
				{
					session_cookie = string(cookie_pos, cookie_end);
				}
			}
		}
		/**
		 * Daca nu avem raspuns de la server dam o eroare
		 */
		else
		{
			print_error(command);
		}

		free(body_data[0]);
		free(content_type);
		free(response);
	}
	else if (command == "enter_library")
	{
		/**
		 * Hardcodare++ x3
		*/
		char *url = (char *)calloc(URL_SIZE, sizeof(char));
		if (!url) {
			cerr << "Failed to allocate memory for url. #2" << endl;
			exit(1);
		}
		strcpy(url, "/api/v1/tema/library/access");

		/**
		 * Trimitem request-ul la server
		*/
		send_to_server(sockfd, compute_get_request((char *)HOST, url, NULL,
												   cookies,
											get_json_token(jwt_token), method));
		
		/**
		 * Vedem raspunsul de la server
		*/
		response = receive_from_server(sockfd);
		if (response)
		{
			/**
			 * Aici e foarte important, intrucat aici si doar aici o sa
			 * extragem token-ul si o sa ni-l aducem aminte tot restul vietii
			 * noastre :)
			*/
			jwt_token = extract_jwt_token(response);
		}
		
		print_response(response, command);

		free(url);
		free(response);
	}
	else if (command == "get_books")
	{
		/**
		 * Hardcodare++ x4
		*/
		char *url = (char *)calloc(URL_SIZE, sizeof(char));
		if (!url) {
			cerr << "Failed to allocate memory for url. #3" << endl;
			exit(1);
		}
		strcpy(url, "/api/v1/tema/library/books");

		/**
		 * Trimitem request-ul catre server
		*/
		send_to_server(sockfd, compute_get_request((char *)HOST, url, NULL,
						cookies,
						get_json_token(jwt_token), method));
		/**
		 * Verificam raspunsul dat de server
		*/
		response = receive_from_server(sockfd);
		if (response)
		{
			/**
			 * Nu am vrut sa fie cod copy-paste aici dar nu merge
			 * fara codul asta aici
			 * 
			 * ...daaaar merge brici si asa deci e bine :)
			*/
			if (strstr(response, "error")) {
				string json_part(response);
				size_t pos = json_part.find("{");
				if (pos != string::npos)
				{
					json j = json::parse(json_part.substr(pos));
					/**
					* Extragem token-ul
					*/
					if (j.find("error") != j.end())
					{
						cout << "error: " << j["error"].get<string>() << endl;
						free(response);
						close(sockfd);
						return;
					}
				}
			} else {
				cout << response << endl;
			}
		}
		else
		{
			print_error(command);
		}

		free(response);
		free(url);
	}
	else if (command == "logout")
	{
		/**
		 * Hardcodare++ x5
		*/
		string url = "/api/v1/tema/auth/" + command;

		/**
		 * Trimitem request-ul
		*/
		send_to_server(sockfd, compute_get_request((char *)HOST,
													(char *)url.c_str(),
												   NULL, cookies,
												   get_json_token(jwt_token),
												   method));
		/**
		 * Aici primim de la server
		*/
		response = receive_from_server(sockfd);

		/**
		 * Vedem aici daca returneaza cu OK sau cu fail
		*/
		print_response(response, command);

		free(response);

		/**
		 * Aici o sa stergem cookie-ul de sesiune si token-ul JWT
		*/
		session_cookie.clear();
		jwt_token.clear();
	}
	else if (command == "add_book")
	{
		string title, author, genre, publisher, page_count_input;
		int page_count;
		/**
		 * Daca stergem asta, dupa add_book o sa avem
		 * title=author= si nu e ok
		*/
		cin.ignore();

		/**
		 * Se iau datele de la tastatura
		*/
		cout << "title=";
		getline(cin, title);
		cout << "author=";
		getline(cin, author);
		cout << "genre=";
		getline(cin, genre);
		cout << "publisher=";
		getline(cin, publisher);
		cout << "page_count=";
		getline(cin, page_count_input);

		/**
		 * Aici verificam daca titlul sau page count-ul e invalid
		 * si returnam o eroare daca e cazul
		*/
		if (title.empty() || author.empty() || genre.empty()
														|| publisher.empty())
		{
			cout << "error: invalid fields" << endl;
			free(method);
			close(sockfd);
			return;
		}

		/**
		 *  https://www.geeksforgeeks.org/exception-handling-c/
		 * ...efectiv seamana cu POO aici
		*/
		try
		{
			page_count = stoi(page_count_input);
		}
		catch (const invalid_argument &e)
		{
			cout << "error: invalid page_count" << endl;
			free(method);
			close(sockfd);
			return;
		}

		/**
		 * Acele date le punem in JSON
		*/
		j["title"] = title;
		j["author"] = author;
		j["genre"] = genre;
		j["publisher"] = publisher;
		j["page_count"] = page_count;

		/**
		 * Hardcodare++ x6 cum ne asteptam deja ca nu se poate altfel
		*/
		char *url = (char *)calloc(URL_SIZE, sizeof(char));
		if (!url) {
			cerr << "Failed to allocate memory for url. #4" << endl;
			exit(1);
		}
		strcpy(url, "/api/v1/tema/library/books");

		/**
		 * Luam JSON-ul si il convertim in char* ca sa fie mai usor de trimis
		*/
		char *body_data[1] = {strdup(j.dump().c_str())};

		/**
		 * Opaaaaa avem hardcodare++ x7
		*/
		char *content_type = (char *)calloc(URL_SIZE, sizeof(char));
		if (!content_type) {
			cerr << "Failed to allocate memory for content_type." << endl;
			exit(1);
		}
		strcpy(content_type, "application/json");

		/**
		 * Aici iarasi trimitem la server
		*/
		send_to_server(sockfd, compute_post_request((char *)HOST, url,
													content_type, body_data, 1,
													cookies, 1,
													get_json_token(jwt_token)));
		/**
		 * Aici primim raspunsul de la server
		*/
		response = receive_from_server(sockfd);

		/**
		 * Afisam OK sau error daca e cazul
		*/
		print_response(response, command);

		free(body_data[0]);
		free(url);
		free(content_type);
		free(response);
	}
	/**
	 * Ce bine e ca avem comenzi care primesc acelasi input
	*/
	else if (command == "get_book" || command == "delete_book")
	{
		/**
		 * Citim id-ul
		*/
		cout << "id=";
		string id;
		cin >> id;

		/**
		 * HAHAAAAAAAAAAA EXCELENT PUSTIULE HARDCODARE++ x8
		*/
		char *url = (char *)calloc(URL_SIZE, sizeof(char));
		if (!url) {
			cerr << "Failed to allocate memory for url. #5" << endl;
			exit(1);
		}
		strcpy(url, "/api/v1/tema/library/books/");
		strcat(url, id.c_str());

		/**
		 * In cazul in care avem delete_book o sa schimbam metoda in DELETE
		*/
		if (command == "delete_book")
		{
			memset(method, 0, 6);
			strcpy(method, "DELETE");
		}
		/**
		 * Trimitem la server
		*/
		send_to_server(sockfd, compute_get_request((char *)HOST, url, NULL,
													   cookies,
													get_json_token(jwt_token),
													method));
		/**
		 * Primim (Doamne ajuta) raspunsul de la server
		*/
		response = receive_from_server(sockfd);
		/**
		 * Aici afisam raspunsul potrivit de la server in functie de caz
		*/
		if (response)
		{
			if (command == "delete_book")
			{
				print_response(response, command);
			}
			else
			{
				/**
				 * Daca avem eroare la raspundere, atunci o sa afisam eroarea
				 * si o sa inchidem conexiunea si o sa eliberam memoria
				*/
				if (get_error_message(response)) {
					free(response);
					free(url);
					close(sockfd);
					return;
				}

				cout << response << endl;
			}
		}
		/**
		 * Daca nu avem niciun raspuns inseamna ca e ceva neinregula
		*/
		else
		{
			print_error(command);
		}

		free(url);
		free(response);
	}
	else if (command == "exit")
	{
		/**
		 * exit. ....bam bam
		*/
		error("exit");
	}
	else
	{
		/**
		 * e invalid man....
		*/
		cout << "error: invalid command" << endl;
	}

	close(sockfd);
}

int main()
{
	string command;

	/**
	 * Citim comanda pana la adanci batraneti
	*/
	forever
	{
		cin >> command;
		handle_command(command);
	}

	return 0;
}
