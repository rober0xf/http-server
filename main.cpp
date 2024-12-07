#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// AF_INET ipv4, SOCK_STREAM tcp.
class Socket {
  public:
    explicit Socket(int domain, int type, int protocol) {
        sock_fd = socket(domain, type, protocol);
        if (sock_fd == -1) {
            throw std::runtime_error("error creating the socket");
        }
    }

    // destructor
    ~Socket() {
        if (sock_fd != -1) {
            close(sock_fd);
        }
    }

    // assigns the address to the socket. reinterpret_cast for a safe conversion.
    void bind(const sockaddr_in &address) {
        if (::bind(sock_fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1) {
            throw std::runtime_error("bind error");
        }
    }

    // backlog is the number of pending connections in the queue
    void listen(int backlog) {
        if (::listen(sock_fd, backlog) == -1) {
            throw std::runtime_error("listen error");
        }
    }

    // accept function returns a socket, easier to close it.
    Socket accept(sockaddr_in &client_address) {
        socklen_t client_len = sizeof(client_address);
        int client_fd = ::accept(sock_fd, reinterpret_cast<sockaddr *>(&client_address), &client_len);
        if (client_fd == -1) {
            throw std::runtime_error("accept error");
        }

        return Socket(client_fd);
    }

    // we cant pass the string to write func. we need to "cast" it to an array of chars, like in c.
    void write(const std::string &message) {
        if (::write(sock_fd, message.c_str(), message.size()) == -1) { // -1 because \0 last char.
            throw std::runtime_error("write error");
        }
    }

    std::string read(size_t buffer_size = 1024) {
        char buffer[buffer_size];                                      // we will temporary save the data here, we cant use a vector.
        std::memset(buffer, 0, buffer_size);                           // initialize the buffer with zeros.
        ssize_t bytes_read = ::read(sock_fd, buffer, buffer_size - 1); // ssize_t like in the docs, and because it can be negative.
        if (bytes_read == -1) {
            throw std::runtime_error("read error");
        }

        return std::string(buffer, bytes_read);
    }

  private:
    int sock_fd = -1;                        // because its not associated with any valid socket initially.
    explicit Socket(int fd) : sock_fd(fd) {} // sock_fd its initialized with the value of fd before executing the constructor.
};

int main(int argc, char *argv[]) {
    try {
        Socket server_socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(8000);
        inet_pton(AF_INET, "0.0.0.0", &server_address.sin_addr);

        // link and listen
        server_socket.bind(server_address);
        server_socket.listen(15);
        std::cout << "listening on port 8000" << std::endl;

        // accept connection
        sockaddr_in client_address{};
        Socket client_socket = server_socket.accept(client_address);
        std::cout << "client connected" << std::endl;

        // send and receive
        client_socket.write("hello there\n");
        std::string client_message = client_socket.read();
        std::cout << "message received from the client: " << client_message << std::endl;

    } catch (const std::exception &excp) {
        std::cerr << "error: " << excp.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
