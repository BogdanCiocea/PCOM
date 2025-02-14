#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include "list.h"
#include <string.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#define DESTINATION_UNREACHABLE 0
#define TIME_EXCEEDED 1
#define RTABLE_ENTRIES 1e6

#define forever while (1) // ;) *wink* *wink*

#define IP_TYPE 0x0800
#define TTL 60
#define ICMP_PROTOCOL 1
#define IPV4_VERSION 4

#define ICMP_TIME_EXCEEDED 11
#define ICMP_DESTINATION_UNREACHABLE 3
#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

#define ARP_HTYPE 1
#define ARP_HLEN 6
#define ARP_PLEN 4

#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ARP_TYPE 0x0806

struct route_table_entry *rtable;
int rtable_len;

struct arp_table_entry *mac_table;
int mac_table_len;

/**
 * @brief Function that implements the binary search algorithm
 * @param target_ip - the ip address that we are looking for
 * @param left - the left index of the array
 * @param right - the right index of the array
 * @return the best match for the target_ip
 */
struct route_table_entry *binary_search(uint32_t target_ip)
{
	struct route_table_entry *best_match = NULL;
	uint32_t left = 0;
	uint32_t right = rtable_len - 1;

	do
	{
		/**
		 * For avoiding overflow
		*/
		uint32_t mid = left + (right - left) / 2;

		if (rtable[mid].prefix == (target_ip & rtable[mid].mask))
		{
			/**
			 * Update the best match
			 */
			best_match = &rtable[mid];
			if (left == right)
			{
				return best_match;
			}
			else
			{
				/**
				 * In this case, we found a suitable match, but we need to
				 * continue the search for a better match, and hence we sorted
				 * the routing table in descending order, will we continue
				 * the search on the left side of the array
				 */
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
	} while (left <= right);

	return best_match;
}

/**
 * @brief Function that returns the best route for a given target_ip
 * @param target_ip - the ip address that we are looking for
 * @return the best route for the target_ip
 */
struct route_table_entry *get_best_route(uint32_t target_ip)
{
	return binary_search(target_ip);
}

/**
 * This one is from the lab
 * @brief Function that returns the mac address of the interface
 * @param given_ip the ip address given
 * @return the mac address of the interface
 */
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

/**
 * Swap the source and destination addresses
 * @param ip_header the ip header
 */
void swap_addresses(struct iphdr **ip_header)
{
	uint32_t aux = (*ip_header)->saddr;
	(*ip_header)->saddr = (*ip_header)->daddr;
	(*ip_header)->daddr = aux;
}

/**
 * Populate the ip header's fields and swap the source and destination
 * addresses
 * @param ip_header the ip header
 */
void set_ip_header(struct iphdr *ip_header)
{
	/**
	 * The default values for the ip header with the "don't care" fields
	 */
	ip_header->ttl = TTL;
	ip_header->protocol = ICMP_PROTOCOL;
	ip_header->tot_len = htons(sizeof(struct icmphdr) + sizeof(struct iphdr));
	ip_header->check = htons(checksum((uint16_t *)ip_header,
									  sizeof(struct iphdr)));
	ip_header->tos = 0;
	ip_header->id = 1;
	ip_header->frag_off = 0;
	ip_header->ihl = 5;
	ip_header->version = IPV4_VERSION;

	swap_addresses(&ip_header);
}

/**
 * Function where it sends the icmp packet with the error message if necessary
 * @param interface the interface
 * @param len the length of the buffer
 * @param ip_header the ip header
 * @param ether_hdr the ethernet header
 * @param buf the buffer
 * @param error_message the error message
 */
void icmp(int interface, size_t len, struct iphdr *ip_header,
		  struct ether_header *ether_hdr, char *buf, short error_message)
{
	/**
	 * Get the best route for the destination address
	 */
	struct route_table_entry *route_entry = get_best_route(ip_header->daddr);

	/**
	 * Initialize the icmp header
	 */
	struct icmphdr *icmp_header = malloc(sizeof(struct icmphdr));
	DIE(icmp_header == NULL, "malloc");

	/**
	 * Populate the ip header's fields
	 */
	set_ip_header(ip_header);

	/**
	 * Get the error if there is one and set it to the type of icmp header
	 * If not, set the type to ICMP_ECHO_REPLY
	 */
	switch (error_message)
	{
	case DESTINATION_UNREACHABLE:
		icmp_header->type = ICMP_DESTINATION_UNREACHABLE;
		break;
	case TIME_EXCEEDED:
		icmp_header->type = ICMP_TIME_EXCEEDED;
		break;
	case ICMP_ECHO_REQUEST:
		icmp_header->type = ICMP_ECHO_REPLY;
		break;
	}

	/**
	 * Get the mac address
	 */
	uint8_t *mac_address = malloc(sizeof(uint8_t));
	DIE(mac_address == NULL, "malloc");
	get_interface_mac(route_entry->interface, mac_address);

	/**
	 * Set the source and destination mac addresses
	 */
	memcpy(ether_hdr->ether_shost, mac_address, 6 * sizeof(uint8_t));

	/**
	 * Populate the icmp header's fields
	 * The code 0 for the icmp header is because in every case we have the
	 * same code
	 */
	icmp_header->code = 0;
	icmp_header->checksum = 0;
	icmp_header->checksum = htons(checksum((uint16_t *)icmp_header,
										   sizeof(struct icmphdr)));

	/**
	 * Get the total length of the packet and prepare the packet to be sent
	 */
	int total_length = sizeof(struct ether_header) +
					   2 * sizeof(struct iphdr) + sizeof(struct icmphdr);
	int offset = sizeof(struct ether_header) + sizeof(struct iphdr);

	char packet[MAX_PACKET_LEN];
	memset(packet, 0, MAX_PACKET_LEN);

	memcpy(packet, ether_hdr, sizeof(struct ether_header));
	memcpy(packet + sizeof(struct ether_header), ip_header,
		   sizeof(struct iphdr));
	memcpy(packet + offset, icmp_header, sizeof(struct icmphdr));
	memcpy(packet + offset + sizeof(struct icmphdr),
		   buf + sizeof(struct ether_header), sizeof(struct iphdr));

	send_to_link(route_entry->interface, packet, total_length);
}

/**
 * Function where the arp packet's fields are populated.
 * Depending on the operation, the fields are populated differently (request/reply)
 *
 * @param arp the arp header
 * @param route_entry the route entry
 * @param mac_to_next_hop the mac address of the next hop
 * @param op the operation
 */
void build_arp(struct arp_header *arp,
			   struct route_table_entry *route_entry, uint8_t *mac_to_next_hop,
			   int op)
{
	arp->htype = htons(ARP_HTYPE);
	arp->ptype = htons(IP_TYPE);
	arp->hlen = ARP_HLEN;
	arp->plen = ARP_PLEN;
	arp->op = htons(op);
	arp->spa = inet_addr(get_interface_ip(route_entry->interface));
	arp->tpa = route_entry->next_hop;
}

/**
 * This function sends an ARP request to the next hop
 * @param route_entry the route entry
 */
void arp_request(struct route_table_entry *route_entry)
{
	/**
	 * Set the broadcast mac address for the ethernet header because there is
	 * no destination yet
	 */
	uint8_t *broadcast_mac = malloc(6 * sizeof(uint8_t));
	DIE(broadcast_mac == NULL, "broadcast_mac");
	memset(broadcast_mac, 0xFF, 6 * sizeof(uint8_t));

	/**
	 * Allocate memory for the ethernet header
	 * Set the destination mac address to the broadcast mac address
	 */
	struct ether_header *eth_hdr = malloc(sizeof(struct ether_header));
	DIE(eth_hdr == NULL, "eth_hdr");
	memcpy(eth_hdr->ether_dhost, broadcast_mac, 6 * sizeof(uint8_t));

	uint8_t *mac_to_next_hop = malloc(sizeof(uint8_t));
	DIE(mac_to_next_hop == NULL, "mac_to_next_hop");
	get_interface_mac(route_entry->interface, mac_to_next_hop);

	/**
	 * Set the source mac address to the mac address of the interface
	 */
	memcpy(eth_hdr->ether_shost, mac_to_next_hop, sizeof(uint8_t));
	eth_hdr->ether_type = htons(ARP_TYPE);

	struct arp_header *arp = malloc(sizeof(struct arp_header));
	DIE(arp == NULL, "arp");

	/**
	 * Build the arp packet to be sent to the next hop
	 */
	char *arp_request_packet = calloc(MAX_PACKET_LEN, sizeof(char));
	DIE(arp_request_packet == NULL, "arp_request_packet");
	memcpy(arp_request_packet, eth_hdr, sizeof(struct ether_header));

	/**
	 * Populate the fields of the arp packet
	 */
	build_arp(arp, route_entry, mac_to_next_hop, ARP_OP_REQUEST);

	/**
	 * Copy the data to the packet
	 */
	memcpy(arp_request_packet, eth_hdr, sizeof(struct ether_header));
	memcpy(arp_request_packet + sizeof(struct ether_header), arp,
		   sizeof(struct arp_header));

	/**
	 * Calculate the length of the packet and send the packet
	 */
	int total_length = sizeof(struct ether_header) + sizeof(struct arp_header);

	send_to_link(route_entry->interface, arp_request_packet, total_length);
}

/**
 * This function generates an ARP reply
 * @param arp the arp header
 * @param eth_hdr the ethernet header
 */
void arp_reply(struct arp_header *arp, struct ether_header *eth_hdr)
{
	/**
	 * Firstly we will get the best route to the source ip address to send the
	 * reply
	 */
	struct route_table_entry *route_entry = get_best_route(arp->spa);

	/**
	 * Now we will allocate memory for the ethernet header
	 */
	struct ether_header *eth_header = malloc(sizeof(struct ether_header));
	DIE(eth_header == NULL, "malloc");

	/**
	 * Set the destination mac address to the source mac address
	 */
	memcpy(eth_header->ether_dhost, (char *)eth_hdr->ether_shost,
		   6 * sizeof(uint8_t));
	/**
	 * Set the ethernet type to ARP_TYPE
	 */
	eth_header->ether_type = htons(ARP_TYPE);

	/**
	 * Now we will need to get the mac address of the next hop
	 */
	uint8_t *mac_to_next_hop = malloc(sizeof(uint8_t));
	DIE(mac_to_next_hop == NULL, "malloc");

	get_interface_mac(route_entry->interface, mac_to_next_hop);

	/**
	 * Set the source mac address to the mac address of the next hop
	 */
	memcpy(eth_header->ether_shost, mac_to_next_hop, 6 * sizeof(uint8_t));

	/**
	 * Now we will allocate memory for the arp header
	 */
	struct arp_header *new_arp = malloc(sizeof(struct arp_header));
	DIE(new_arp == NULL, "malloc");

	/**
	 * Populate the fields of the arp header
	 */
	build_arp(new_arp, route_entry, mac_to_next_hop, ARP_OP_REPLY);

	memcpy(new_arp->sha, mac_to_next_hop, 6 * sizeof(uint8_t));
	memcpy(new_arp->tha, eth_header, sizeof(struct ether_header));

	/**
	 * Make the packet to be sent
	 */
	char arp_reply_packet[MAX_PACKET_LEN];
	memcpy(arp_reply_packet, eth_header, sizeof(struct ether_header));
	memcpy(arp_reply_packet + sizeof(struct ether_header), new_arp,
		   sizeof(struct arp_header));

	/**
	 * Calculate the length of the packet and send the packet
	 */
	int total_length = sizeof(struct ether_header) + sizeof(struct arp_header);

	send_to_link(route_entry->interface, arp_reply_packet, total_length);
}

/**
 * The comparator used to sort the routing table
 * It compares the masks and prefixes of the two elements
 * @param a the first element
 * @param b the second element
 *
 * @return the result of the comparison
 */
int comparator(const void *a, const void *b)
{
	if (((struct route_table_entry *)a)->mask !=
		((struct route_table_entry *)b)->mask)
	{
		return (((struct route_table_entry *)a)->mask >
				((struct route_table_entry *)b)->mask)
				   ? -1
				   : 1;
	}

	if (((struct route_table_entry *)a)->prefix !=
		((struct route_table_entry *)b)->prefix)
	{
		return (((struct route_table_entry *)a)->prefix >
				((struct route_table_entry *)b)->prefix)
				   ? -1
				   : 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	rtable = malloc(RTABLE_ENTRIES * sizeof(struct route_table_entry));
	DIE(rtable == NULL, "malloc");

	rtable_len = read_rtable(argv[1], rtable);

	mac_table = malloc(sizeof(struct arp_table_entry) * 100);
	DIE(mac_table == NULL, "malloc");

	mac_table_len = 0;

	queue que = queue_create();

	qsort((void *)rtable, rtable_len, sizeof(struct route_table_entry),
		  comparator);

	forever
	{

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *)buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is
		needed when sending a packet on the link, */

		struct iphdr *ip_header = (struct iphdr *)(buf +
												   sizeof(struct ether_header));

		/**
		 * Check if the packet is for the router. If it is, send an echo reply
		 */
		if (inet_addr(get_interface_ip(interface)) == ip_header->daddr)
		{
			icmp(interface, len, ip_header, eth_hdr, buf,
				 ICMP_ECHO_REQUEST);
			continue;
		}

		/**
		 * Check the TTL
		 * If not valid, send an ICMP packet with the error message
		 */
		if (ip_header->ttl <= 1)
		{
			ip_header->ttl = TTL;
			ip_header->check = htons(checksum((uint16_t *)ip_header,
											  sizeof(struct iphdr)));
			ip_header->daddr = ip_header->saddr;

			icmp(interface, len, ip_header, eth_hdr, buf, TIME_EXCEEDED);
			continue;
		}

		/**
		 * Check if the packet is an ARP packet
		 */
		if (eth_hdr->ether_type == htons(ARP_TYPE))
		{
			struct arp_header *arp_received =
				(struct arp_header *)(buf + sizeof(struct ether_header));

			/**
			 * Now we will add the mac address to the mac table and the ip
			 * address
			 */
			memcpy(mac_table[mac_table_len].mac, arp_received->sha,
				   6 * sizeof(uint8_t));
			mac_table[mac_table_len++].ip = arp_received->spa;

			/**
			 * Check the operation of the packet
			 */
			switch (ntohs(arp_received->op))
			{
			/**
			 * If the operation is a request, generate an arp reply
			 */
			case ARP_OP_REQUEST:
				arp_reply(arp_received, eth_hdr);
				continue;
				/**
				 * If the operation is a reply, send the packets from the queue
				 */
			case ARP_OP_REPLY:
				/**
				 * While the queue is not empty, send the packets from the queue
				 */
				while (!queue_empty(que))
				{
					struct cell *cell = queue_deq(que);
					/**
					 * Extract the ip header from the packet
					 */
					struct iphdr *ip_header =
						(struct iphdr *)(cell->element +
										 sizeof(struct ether_header));

					int total_length = sizeof(struct ether_header) +
									   sizeof(cell->element);
					/**
					 * Get the best route for the destination address
					 */
					struct route_table_entry *best_route =
						get_best_route(ip_header->daddr);

					/**
					 * Send the packet
					 */
					send_to_link(best_route->interface, cell->element,
								 total_length);

					free(cell->element);
					free(cell);
				}
				continue;
			}
		}

		/**
		 * Save the checksum and set it to 0 for the verification
		 */
		uint16_t save_check = ip_header->check;
		ip_header->check = 0;

		/**
		 * Check if the checksum is correct
		 */
		if (checksum((uint16_t *)ip_header, sizeof(struct iphdr)) !=
			ntohs(save_check))
		{
			continue;
		}

		/**
		 * Decrease the TTL
		 */
		ip_header->ttl--;

		/**
		 * Try to get the best route for the destination address
		 */
		struct route_table_entry *route_entry =
			get_best_route(ip_header->daddr);

		/**
		 * If there is no route, send an ICMP packet with the error message
		 */
		if (!route_entry)
		{
			ip_header->check = htons(checksum((uint16_t *)ip_header,
											  sizeof(struct iphdr)));
			ip_header->ttl = TTL;
			ip_header->daddr = ip_header->saddr;

			icmp(interface, len, ip_header, eth_hdr, buf,
				 DESTINATION_UNREACHABLE);
			continue;
		}

		/**
		 * Update the checksum
		 */
		ip_header->check = htons(checksum((uint16_t *)ip_header,
										  sizeof(struct iphdr)));

		/**
		 * Try to get the mac address of the next hop
		 */
		struct arp_table_entry *mac_address;
		mac_address = get_mac_entry(route_entry->next_hop);

		/**
		 * If there is a mac address, send the packet
		 * If not, add the packet to the queue and generate an arp request
		 */
		if (mac_address)
		{
			/**
			 * Set the destination mac address
			 */
			memcpy(eth_hdr->ether_dhost, mac_address->mac, ETH_ALEN);

			/**
			 * Set the offset to the payload
			 */
			int payload_offset = sizeof(struct ether_header) +
								 sizeof(struct iphdr);

			/**
			 * Copy the data to the packet
			 */
			char packet[MAX_PACKET_LEN];
			memcpy(packet, eth_hdr, sizeof(struct ether_header));
			memcpy(packet + sizeof(struct ether_header), ip_header,
				   sizeof(struct iphdr));
			memcpy(packet + payload_offset, buf + payload_offset, len);

			send_to_link(route_entry->interface, packet, len);
			continue;
		}
		else
		{
			/**
			 * Allocate memory for the cell and the element to be added to
			 * the queue
			 */
			struct cell *cell = malloc(sizeof(struct cell));
			DIE(cell == NULL, "malloc");

			cell->element = malloc(len);
			DIE(cell->element == NULL, "malloc");

			memcpy(cell->element, buf, len);
			queue_enq(que, cell);

			/**
			 * Next we will generate an arp request
			 */
			arp_request(route_entry);
			continue;
		}
	}

	free(rtable);
	free(mac_table);
}
