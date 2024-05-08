#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unordered_set>
#include <random>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define ARRAY_SIZE 1000000
#define MAX_DOUBLE_LENGTH 9

std::unordered_set<double> generateUniqueDoubles(double);
void sendUniqueDoubles(int, struct sockaddr_in, socklen_t, std::unordered_set<double>);

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }

    socklen_t len = sizeof(client_addr);
    int n;

    while (1) {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        buffer[n] = '\0';
        std::cout << "Client : " << buffer << std::endl;




        char* endp;
        double X = strtod(buffer, &endp);
        std::unordered_set<double> uniqueDoubles = generateUniqueDoubles(X);

        for (double num : uniqueDoubles) {
            std::cout << num << std::endl;
        }


        sendUniqueDoubles(sockfd, client_addr, len, uniqueDoubles);
    }


    close(sockfd);
    return 0;
}

std::unordered_set<double> generateUniqueDoubles(double X) {
    std::unordered_set<double> uniqueNumbers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1.0 * X, X);
    double num;

    while (uniqueNumbers.size() < ARRAY_SIZE) {
        num = dis(gen);
        uniqueNumbers.insert(num);
    }

    return uniqueNumbers;
}

void sendUniqueDoubles(int sockfd, struct sockaddr_in client_addr, socklen_t len, std::unordered_set<double> uniqueDoubles)
{
    std::string dataStr;

    for (double num : uniqueDoubles) {
        dataStr +=  std::to_string(num) + ";";
        if (dataStr.size() + MAX_DOUBLE_LENGTH >= BUFFER_SIZE) {
            sendto(sockfd, dataStr.c_str(), dataStr.length(), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);
            std::cout<<"Sended\n"<<dataStr;
            dataStr.clear();
        }
    }
    sendto(sockfd, "E", sizeof("E"), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);
}