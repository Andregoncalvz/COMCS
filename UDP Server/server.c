#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define BUF_SIZE 300
#define SERVER_PORT "9999"

const float TEMP_DIFF_THRESHOLD = 2.0;
const float HUMIDITY_DIFF_THRESHOLD = 5.0;
const float TEMP_MIN = 0.0;
const float TEMP_MAX = 50.0;
const float HUMIDITY_MIN = 20.0;
const float HUMIDITY_MAX = 80.0;

void *handleClient(void *arg);

typedef struct {
    struct sockaddr_storage clientAddr;
    char message[BUF_SIZE];
    int messageLen;
    unsigned int addrLen;
    int sock; // Add socket descriptor
} ClientData;

typedef struct ClientRecord {
    char ip[BUF_SIZE];
    char port[BUF_SIZE];
    float lastTemp;
    float lastHumidity;
    struct ClientRecord *next;
} ClientRecord;

ClientRecord *head = NULL;

ClientRecord* findOrCreateClientRecord(const char *ip, const char *port) {
    ClientRecord *current = head;
    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0 && strcmp(current->port, port) == 0) {
            return current; // Found existing record
        }
        current = current->next;
    }

    // Create new record if not found
    ClientRecord *newRecord = (ClientRecord *)malloc(sizeof(ClientRecord));
    if (newRecord == NULL) {
        perror("Failed to allocate memory for new client record");
        exit(1);
    }
    strncpy(newRecord->ip, ip, BUF_SIZE);
    strncpy(newRecord->port, port, BUF_SIZE);
    newRecord->lastTemp = 0; // Initialize previous values
    newRecord->lastHumidity = 0;
    newRecord->next = head;
    head = newRecord;

    return newRecord;
}

int main(void) {
    struct sockaddr_storage client;
    int sock, res, err;
    unsigned int adl;
    char buffer[BUF_SIZE];
    struct addrinfo req, *list;

    // Initialize server
    bzero((char *)&req, sizeof(req));
    req.ai_family = AF_INET6; // Support IPv4 and IPv6
    req.ai_socktype = SOCK_DGRAM; // UDP socket
    req.ai_flags = AI_PASSIVE; // Use local address
    err = getaddrinfo(NULL, SERVER_PORT, &req, &list);
    if (err) {
        printf("Failed to get local address, error: %s\n", gai_strerror(err));
        exit(1);
    }

    sock = socket(list->ai_family, list->ai_socktype, list->ai_protocol);
    if (sock == -1) {
        perror("Failed to open socket");
        freeaddrinfo(list);
        exit(1);
    }

    if (bind(sock, (struct sockaddr *)list->ai_addr, list->ai_addrlen) == -1) {
        perror("Bind failed");
        close(sock);
        freeaddrinfo(list);
        exit(1);
    }

    freeaddrinfo(list);
    puts("Listening for UDP requests (IPv6/IPv4). Use CTRL+C to terminate the server");

    adl = sizeof(client);
    while (1) {
        // Receive data from clients
        res = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&client, &adl);
        if (res > 0) {
            buffer[res] = '\0'; // Null-terminate the received data

            // Create a ClientData struct to pass to the thread
            ClientData *clientData = malloc(sizeof(ClientData));
            clientData->clientAddr = client;
            strncpy(clientData->message, buffer, BUF_SIZE);
            clientData->messageLen = res;
            clientData->addrLen = adl;
            clientData->sock = sock; // Pass socket descriptor

            // Create a new thread to handle the client
            pthread_t thread;
            if (pthread_create(&thread, NULL, handleClient, clientData) != 0) {
                perror("Failed to create thread");
                free(clientData);
            } else {
                pthread_detach(thread); // Detach thread to prevent memory leaks
            }
        }
    }

    close(sock);
    return 0;
}

void *handleClient(void *arg) {
    ClientData *clientData = (ClientData *)arg;
    char cliIPtext[BUF_SIZE], cliPortText[BUF_SIZE];

    // Get the current thread ID
    pthread_t threadID = pthread_self();

    // Get client IP and port in a human-readable format
    if (!getnameinfo((struct sockaddr *)&clientData->clientAddr, clientData->addrLen,
                     cliIPtext, BUF_SIZE, cliPortText, BUF_SIZE, NI_NUMERICHOST | NI_NUMERICSERV)) {
        printf("Thread %lu: Handling request from client IP: %s, Port: %s\n",
               (unsigned long)threadID, cliIPtext, cliPortText);
    } else {
        printf("Thread %lu: Failed to retrieve client address information\n", (unsigned long)threadID);
    }

    // Parse the message (assuming format "QoS: X, Temp: XX.XX, Humidity: YY.YY")
    float qos = 0, temp = 0, humidity = 0;
    if (sscanf(clientData->message, "QoS: %f, Temp: %f, Humidity: %f", &qos, &temp, &humidity) == 3) {
        printf("Thread %lu: Data received -> QoS: %.1f, Temperature: %.2f°C, Humidity: %.2f%%\n", 
               (unsigned long)threadID, qos, temp, humidity);

        // If QoS is 1, send an ACK immediately after receiving the message
        if (qos == 1) {
            char response[] = "ACK: Message Received ANDRÉ";
            sendto(clientData->sock, response, sizeof(response), 0, (struct sockaddr *)&clientData->clientAddr, clientData->addrLen);
            printf("Thread %lu: Sent ACK back to client.\n", (unsigned long)threadID);
        }

        // Perform validation and checks after sending the ACK
        // Validate the received data
        if (temp < TEMP_MIN || temp > TEMP_MAX) {
            printf("Thread %lu: [WARNING] Temperature out of acceptable range (%.1f°C - %.1f°C)\n", (unsigned long)threadID, TEMP_MIN, TEMP_MAX);
        }
        if (humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
            printf("Thread %lu: [WARNING] Humidity out of acceptable range (%.1f%% - %.1f%%)\n", (unsigned long)threadID, HUMIDITY_MIN, HUMIDITY_MAX);
        }
    } else {
        printf("Thread %lu: [ERROR] Invalid data format received: %s\n", (unsigned long)threadID, clientData->message);
    }

    free(clientData); // Free allocated memory
    return NULL;
}
