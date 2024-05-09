#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <random>
#include <future>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define ARRAY_SIZE 1000
#define MAX_DOUBLE_LENGTH 9
#define ACK_MSG "ACK"
#define SERVER_PROTOCOL_VERSION "0.1"
#define SERVER_CONFIG "server.conf"
#define PORT_RANGE_START 1024
#define PORT_RANGE_END 65535

std::unordered_set<double> generateUniqueDoubles(double);
void sendUniqueDoubles(int, struct sockaddr_in, socklen_t, std::unordered_set<double>);
void handleClient(int, struct sockaddr_in, socklen_t, const char*);
int findAvailablePort(const std::unordered_set<int>&);
int getPortFromFile(const std::string&);

std::mutex mtx;
std::unordered_set<int> usedPorts;

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    std::vector<std::future<void>> futures;
    int port = getPortFromFile(SERVER_CONFIG);

    std::cout<<port<<std::endl;


    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }

    socklen_t len = sizeof(client_addr);
    int n;

    while (1) {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        if(n <= 0) continue;
        buffer[n] = '\0';

        futures.push_back(std::async(std::launch::async, [sockfd, client_addr, len, buffer]() {
            handleClient(sockfd, client_addr, len, buffer);
        }));
    }

    for (auto& future : futures) {
        future.wait();
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

void handleClient(int sockfd, struct sockaddr_in client_addr, socklen_t len, const char* buffer)
{
    std::cout << "Function is executing in thread: " << std::this_thread::get_id() << std::endl;

    int i = 0;
    std::string version;

    std::cout << "Client : " << buffer << std::endl;
                
    while (buffer[i] != ':' && buffer[i] != '\0') {
        i++;
    }
    if (buffer[i] == '\0') {
        std::string errorMsg = "Unsupported syn message format";
        std::cout<<"Unsupported format: "<<buffer<<std::endl;
        sendto(sockfd, errorMsg.c_str(), errorMsg.length(), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);
        return;
    }
    if (buffer[i] == ':') {
        i++;
        std::string clientVersion = buffer + i;
        std::string serverVersion = SERVER_PROTOCOL_VERSION;

        if (clientVersion != serverVersion) {
            std::string errorMsg = "Wrong protocol version. SERVER_VERSION ";

            errorMsg += SERVER_PROTOCOL_VERSION;
            sendto(sockfd, errorMsg.c_str(), errorMsg.length(), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);
            return;
        }
    }
    int n;
    char clientMsg[BUFFER_SIZE];

    sendto(sockfd, ACK_MSG, sizeof(ACK_MSG), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);
    mtx.lock();
    int newPort = findAvailablePort(usedPorts);
    std::string portString = std::to_string(newPort);
    sendto(sockfd, portString.c_str(), portString.length(), MSG_CONFIRM, (const struct sockaddr *)&client_addr, len);

    int newSocket;
    struct sockaddr_in newServerAddr;

    memset(&newServerAddr, 0, sizeof(newServerAddr));

    newServerAddr.sin_family = AF_INET;
    newServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    newServerAddr.sin_port = htons(newPort);

    if ((newSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }
    if (bind(newSocket, (const struct sockaddr *)&newServerAddr, sizeof(newServerAddr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return;
    }
    usedPorts.insert(newPort);
    mtx.unlock();

    n = recvfrom(newSocket, (char *)clientMsg, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &len);

    char* endp;
    double X = strtod(clientMsg, &endp);
    std::unordered_set<double> uniqueDoubles = generateUniqueDoubles(X);

    sendUniqueDoubles(sockfd, client_addr, len, uniqueDoubles);
}

int findAvailablePort(const std::unordered_set<int>& usedPorts) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    for (int port = PORT_RANGE_START; port <= PORT_RANGE_END; ++port) {
        if (usedPorts.find(port) == usedPorts.end()) {
            addr.sin_port = htons(port);
            if (bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
                close(sockfd);
                std::cout << port << std::endl;
                return port;
            }
        }
    }

    close(sockfd);
    return -1;
}

int getPortFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find("Port : ");
        if (pos != std::string::npos) {
            std::string portStr = line.substr(pos + 7);
            return std::atoi(portStr.c_str());
        }
    }

    std::cerr << "Port not found in file: " << filename << std::endl;
    return -1;
}