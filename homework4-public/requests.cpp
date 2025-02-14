#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
						  char **cookies, char *jwt,
						  char *method)
{
	char *message = (char *)calloc(BUFLEN, sizeof(char));
	char *line = (char *)calloc(LINELEN, sizeof(char));

	// Step 1: write the method name, URL, request params (if any) and protocol type
	sprintf(line, "%s %s HTTP/1.1", method, url);
	compute_message(message, line);

	// Step 2: add the host
	sprintf(line, "Host: %s", host);
	compute_message(message, line);
	memset(line, 0, LINELEN);

	/**
	 * Am adaugat aici pentru JWT
	*/
	if (jwt)
	{
		sprintf(line, "Authorization: Bearer %s", jwt);
		compute_message(message, line);
		memset(line, 0, LINELEN);
	}

	// Step 3 (optional): add headers and/or cookies, according to the protocol format
	if (cookies[0] != NULL)
	{
		sprintf(line, "Cookie: %s", cookies[0]);
		compute_message(message, line);
		memset(line, 0, LINELEN);
	}

	// Step 4: add final new line
	compute_message(message, "");
	free(line);
	return message;
}

char *compute_post_request(char *host, char *url, char *content_type,
						   char **body_data, int body_data_fields_count,
						   char **cookies, int cookies_count, char *jwt)
{
	char *message = (char *)calloc(BUFLEN, sizeof(char));
	char *line = (char *)calloc(LINELEN, sizeof(char));

	// Step 1: write the method name, URL and protocol type
	sprintf(line, "POST %s HTTP/1.1", url);
	compute_message(message, line);
	// Step 2: add the host
	sprintf(line, "Host: %s", host);
	compute_message(message, line);

	if (jwt)
	{
		sprintf(line, "Authorization: Bearer %s", jwt);
		compute_message(message, line);
	}

	/* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
			in order to write Content-Length you must first compute the message size
	*/
	int content_length = 0;
	for (int i = 0; i < body_data_fields_count; i++)
	{
		content_length += strlen(body_data[i]);
	}
	sprintf(line, "Content-Length: %d", content_length);
	compute_message(message, line);
	sprintf(line, "Content-Type: %s", content_type);
	compute_message(message, line);

	// Step 4 (optional): add cookies
	for (int i = 0; i < cookies_count; i++)
	{
		sprintf(line, "Cookie: %s", cookies[i]);
		compute_message(message, line);
	}

	// Step 5: add new line at end of header
	strcat(message, "\r\n");
	// Step 6: add the actual payload data
	for (int i = 0; i < body_data_fields_count; i++)
	{
		strcat(message, body_data[i]);
	}

	free(line);
	return message;
}
