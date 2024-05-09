#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define CLIENT_PROTOCOL_VERSION "0.1"
#define OUTPUT_FILE_NAME "sorted_numbers"
#define CLIENT_CONFIG "client.conf"

struct ClientConfig {
    std::string serverIP;
    int serverPort;
    double number;
};

void handleUniqueDoubles(int, struct sockaddr_in, socklen_t);
void writeVectorToBinaryFile(const std::vector<double>& vec, const std::string& filename);
void readClientConfig(const std::string&, ClientConfig&);

int main() {
    int sockfd;
    char server_answer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    struct ClientConfig conf;

    readClientConfig(CLIENT_CONFIG, conf);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    std::cout<<conf.number<<";"<<conf.serverIP<<";"<<conf.serverPort<<std::endl;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(conf.serverPort);
    server_addr.sin_addr.s_addr = inet_addr(conf.serverIP.c_str());

    int len, n;
    std::string syn_msg = "SYN:";
    std::string ack_msg = "ACK";
 
    syn_msg += CLIENT_PROTOCOL_VERSION;
    sendto(sockfd, syn_msg.c_str(), syn_msg.length(), MSG_CONFIRM, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    n = recvfrom(sockfd, (char *)server_answer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&len);
    if (n <= 0) {
        int max_attempts = 5;
        int attempts = 0;

        while (attempts < max_attempts) {
            n = recvfrom(sockfd, (char *)server_answer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&len);
            if (n <= 0) {
                break;
            }

            attempts++;
            sleep(1);
        }

        if (attempts == max_attempts) {
            std::cerr << "Max connection attempts reached. Unable to connect to the server." << std::endl;
            return -1;
        }
    }
    
    std::string server_msg = server_answer;
    if (server_msg != ack_msg) {
        std::cout << server_msg << std::endl;
        return -1;
    }

    n = recvfrom(sockfd, (char *)server_answer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&len);
    if (n <= 0) {
        int max_attempts = 5;
        int attempts = 0;

        while (attempts < max_attempts) {
            n = recvfrom(sockfd, (char *)server_answer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&server_addr, (socklen_t *)&len);
            if (n <= 0) {
                break;
            }

            attempts++;
            sleep(1);
        }

        if (attempts == max_attempts) {
            std::cerr << "Max connection attempts reached. Unable to connect to the server." << std::endl;
            return -1;
        }
    }
    char* endp;
    int newPort = strtol(server_answer, &endp, 10);
    server_addr.sin_port = htons(newPort);
    sleep(3);

    std::string numberStr = std::to_string(conf.number);
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
        std::cout<<buffer<<std::endl;
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
        std::cout<<num<< ";";
    }
    writeVectorToBinaryFile(uniqueDoubles, OUTPUT_FILE_NAME);
}

void writeVectorToBinaryFile(const std::vector<double>& vec, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file for writing." << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&vec[0]), vec.size() * sizeof(double));

    file.close();
}

void readClientConfig(const std::string& filename, ClientConfig& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("Server ip") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                config.serverIP = line.substr(pos + 2);
            }
        } else if (line.find("Server port") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                config.serverPort = std::stoi(line.substr(pos + 2));
            }
        } else if (line.find("Number") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                config.number = std::stod(line.substr(pos + 2));
            }
        }
    }
}