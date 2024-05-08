#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handleUniqueDoubles(int, struct sockaddr_in, socklen_t);

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.1");

    int len;

    sleep(3);

    double number = 3.14;

    std::string numberStr = std::to_string(number);
    sendto(sockfd, numberStr.c_str(), numberStr.length(), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));

    handleUniqueDoubles(sockfd, server_addr, len);

    close(sockfd);
    return 0;
}

void handleUniqueDoubles(int sockfd, struct sockaddr_in server_addr, socklen_t len)
{
    std::vector<double>  uniqueDoubles;
    char buffer[BUFFER_SIZE];
    int n;

    while(1)
    {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&len);
        if (n <= 0) break;
        buffer[n] = '\0';
        if(*buffer == 'E') break;
        std::istringstream iss(buffer);
        std::string token;
        while (std::getline(iss, token, ';')) {
            double num = std::stod(token);
            uniqueDoubles.push_back(num);
        }
    }
    std::sort(uniqueDoubles.begin(), uniqueDoubles.end(), std::greater<double>());
    for (double num : uniqueDoubles) {
        std::cout<<num<<std::endl;
    }
}