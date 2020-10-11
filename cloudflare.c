#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>

char buffer[BUFSIZ];
char errors[BUFSIZ];
enum CONSTEXPR
{
    MAX_REQUEST_LEN = 1024
};
char request[MAX_REQUEST_LEN];
char request_template[] = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
struct protoent *protoent;
char *hostname = "example.com";
char *path;
in_addr_t in_addr;
int request_len;

struct hostent *hostent;
struct sockaddr_in sockaddr_in;
unsigned short server_port = 80;
int ch;

int requests = 1;

// Variables to Return
double fastestTime = INT_MAX;
double slowestTime = INT_MIN;
int smallestResponse = INT_MAX;
int largestResponse = INT_MIN;
double sum = 0.0;
int counter = 0;
double median = 0;
int succeeded = 0;

/* Pthread initialization */
pthread_t *p_tids;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    int max_bytes = 0;
    ssize_t nbytes_total, nbytes_last;
    double times[2];
    struct timeval timeCounter;

    int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (socket_file_descriptor == -1)
    {
        perror("socket");
        sprintf(&errors[strlen(errors)], "Error socket failed: %s", strerror(errno));
        return 0;
    }

    if (connect(socket_file_descriptor, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) == -1)
    {
        perror("connect");
        sprintf(&errors[strlen(errors)], "Error connect failed: %s", strerror(errno));
        return 0;
    }
    /* Send HTTP request. */
    nbytes_total = 0;
    if (gettimeofday(&timeCounter, NULL) != 0)
    {
        perror("timeofday failed");
        sprintf(&errors[strlen(errors)], "Error timeofday failed: %s", strerror(errno));
        return 0;
    }
    times[0] = (timeCounter.tv_sec) + timeCounter.tv_usec / 1000000.;
    while (nbytes_total < request_len)
    {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1)
        {
            perror("write");
            sprintf(&errors[strlen(errors)], "Error write failed: %s", strerror(errno));
            return 0;
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0)
    {
        // in case we need to call read multiple times:
        if (max_bytes < nbytes_total)
            max_bytes = nbytes_total;
        write(STDOUT_FILENO, buffer, nbytes_total);
    }

    if (gettimeofday(&timeCounter, NULL) != 0)
    {
        perror("timeofday failed");
        sprintf(&errors[strlen(errors)], "Error timeofday failed: %s", strerror(errno));
        return 0;
    }

    times[1] = (timeCounter.tv_sec) + timeCounter.tv_usec / 1000000.;

    if (nbytes_total == -1)
    {
        perror("read");
        sprintf(&errors[strlen(errors)], "Error read failed: %s", strerror(errno));
        return 0;
    }

    pthread_mutex_lock(&mutex);
    succeeded++;
    if (counter == requests / 2)
        median = times[1] - times[0];
    counter++;

    sum += (times[1] - times[0]);

    if (times[1] - times[0] < fastestTime)
        fastestTime = times[1] - times[0];
    if (times[1] - times[0] > slowestTime)
        slowestTime = times[1] - times[0];
    if (max_bytes > largestResponse)
        largestResponse = max_bytes;
    if (max_bytes < smallestResponse)
        smallestResponse = max_bytes;
    pthread_mutex_unlock(&mutex);

    close(socket_file_descriptor);

    return 0;
}

void getRequestsSequential()
{
    int requestCounter = requests;
    while (requestCounter > 0)
    {

        int max_bytes = 0;
        ssize_t nbytes_total, nbytes_last;
        double times[2];
        struct timeval timeCounter;

        int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
        if (socket_file_descriptor == -1)
        {
            perror("socket");
            sprintf(&errors[strlen(errors)], "Error socket failed: %s", strerror(errno));
            continue;
        }

        if (connect(socket_file_descriptor, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) == -1)
        {
            perror("connect");
            sprintf(&errors[strlen(errors)], "Error connect failed: %s", strerror(errno));
            continue;
        }
        /* Send HTTP request. */
        nbytes_total = 0;
        if (gettimeofday(&timeCounter, NULL) != 0)
        {
            perror("timeofday failed");
            sprintf(&errors[strlen(errors)], "Error timeofday failed: %s", strerror(errno));
            continue;
        }
        times[0] = (timeCounter.tv_sec) + timeCounter.tv_usec / 1000000.;
        while (nbytes_total < request_len)
        {
            nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
            if (nbytes_last == -1)
            {
                perror("write");
                sprintf(&errors[strlen(errors)], "Error write failed: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            nbytes_total += nbytes_last;
        }

        /* Read the response. */
        while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0)
        {
            // in case we need to call read multiple times:
            if (max_bytes < nbytes_total)
                max_bytes = nbytes_total;
            write(STDOUT_FILENO, buffer, nbytes_total);
        }

        if (gettimeofday(&timeCounter, NULL) != 0)
        {
            perror("timeofday failed");
            sprintf(&errors[strlen(errors)], "Error timeofday failed: %s", strerror(errno));
            continue;
        }

        times[1] = (timeCounter.tv_sec) + timeCounter.tv_usec / 1000000.;

        if (nbytes_total == -1)
        {
            perror("read");
            sprintf(&errors[strlen(errors)], "Error read failed: %s", strerror(errno));
            continue;
        }

        close(socket_file_descriptor);

        succeeded++;
        if (counter == requests / 2)
            median = times[1] - times[0];
        counter++;

        sum += (times[1] - times[0]);

        if (times[1] - times[0] < fastestTime)
            fastestTime = times[1] - times[0];
        if (times[1] - times[0] > slowestTime)
            slowestTime = times[1] - times[0];
        if (max_bytes > largestResponse)
            largestResponse = max_bytes;
        if (max_bytes < smallestResponse)
            smallestResponse = max_bytes;

        requestCounter -= 1;
    }
}

int main(int argc, char **argv)
{

    bool sequential = false;

    static struct option long_options[] = {
        {"url", required_argument, NULL, 'u'},
        {"help", no_argument, NULL, 'h'},
        {"profile", required_argument, NULL, 'p'},
        {"sequential", no_argument, NULL, 's'},
    };

    while ((ch = getopt_long(argc, argv, "u:hp:", long_options, NULL)) != -1)
    {
        switch (ch)
        {
        case 'u':
            hostname = optarg;
            strtok(hostname, "//");
            hostname = strtok(NULL, "/");
            path = strtok(NULL, "/");
            break;
        case 'h':
            printf("Make sure you build using: make all \nRemove bin using: make clean\nusage: ./bin/cloudflare [options] \noptions: \n --url : Needs to be a full url such as https://my-worker.ea6lee.workers.dev/links \n--help : Prints help page \n--profile : Provide number of requests to try. Default is 1. By default uses pthreads \n--sequential: Please use this option if you want to do many requests. Otherwise your system may limit the number of pthreads you can use. \n");
            return 0;
            break;
        case 'p':
            requests = atoi(optarg);
            break;
        case 's':
            sequential = true;
            break;
        case '?':
            break;

        default:
            abort();
        }
    }

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, path, hostname);
    if (request_len >= MAX_REQUEST_LEN)
    {
        fprintf(stderr, "request length large: %d\n", request_len);
        exit(EXIT_FAILURE);
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL)
    {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }

    /* Build the address. */
    hostent = gethostbyname(hostname);
    if (hostent == NULL)
    {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
        exit(EXIT_FAILURE);
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1)
    {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        exit(EXIT_FAILURE);
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    pthread_t *p_tids = malloc(sizeof(pthread_t) * requests);

    if (!sequential)
    {
        // Depending on limits of the system, you may get Resource Temporarily Unavailable if you request too many threads (e.g. 1000)
        // Use the --sequential flag if you want to run a lot of threads
        for (int i = 0; i < requests; i++)
        {
            if (0 != pthread_create(&p_tids[i], NULL, worker, NULL))
            {
                printf("Error: %s\n", strerror(errno));
                return -1;
            };
        }

        for (int i = 0; i < requests; i++)
        {
            if (0 != pthread_join(p_tids[i], NULL))
            {
                printf("Error: %s\n", strerror(errno));
                return -1;
            }
        }
    }
    else
    {
        getRequestsSequential();
    }

    //Output Summary:
    fprintf(stdout, "\nNumber of Requests: %i\n", requests);
    fprintf(stdout, "The fastest time: %f seconds\n", fastestTime);
    fprintf(stdout, "The slowest time: %f seconds\n", slowestTime);
    fprintf(stdout, "The mean time: %f seconds\n", sum / (double)requests);
    fprintf(stdout, "The median time: %f seconds\n", median);
    fprintf(stdout, "The percentage of requests that succeeded: %i %% \n", succeeded / requests * 100);
    fprintf(stdout, "The size in bytes of the smallest response: %i\n", smallestResponse);
    fprintf(stdout, "The size in bytes of the largest response: %i\n", largestResponse);

    // ATTN: Errors are just output from a buffer. They are also outputted inline above.
    printf("Errors if there are any:\n");
    printf("%s", errors);

    // Cleanup
    free(p_tids);
    pthread_mutex_destroy(&mutex);

    exit(EXIT_SUCCESS);
}