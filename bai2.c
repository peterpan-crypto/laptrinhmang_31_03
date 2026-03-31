
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Cach dung: %s port_s ip_d port_d\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("socket() failed");
        return 1;
    }
    unsigned long ul = 1;
    if (ioctl(sock, FIONBIO, &ul) == -1) {
        perror("ioctl() failed");
        close(sock);
        return 1;
    }
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port_s);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
        perror("bind() failed");
        close(sock);
        return 1;
    }
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);

    if (inet_pton(AF_INET, ip_d, &dest_addr.sin_addr) <= 0) {
        printf("Dia chi IP khong hop le\n");
        close(sock);
        return 1;
    }

    if (ioctl(STDIN_FILENO, FIONBIO, &ul) == -1) {
        perror("ioctl(stdin) failed");
        close(sock);
        return 1;
    }

    printf("UDP chat dang chay...\n");
    printf("Nhan tren cong: %d\n", port_s);
    printf("Gui den: %s:%d\n", ip_d, port_d);
    printf("Nhap tin nhan va nhan Enter de gui. Go 'exit' de thoat.\n");

    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];

    while (1) {
        if (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
            send_buf[strcspn(send_buf, "\n")] = '\0';

            if (strcmp(send_buf, "exit") == 0) {
                break;
            }

            if (strlen(send_buf) > 0) {
                int sent = sendto(sock, send_buf, strlen(send_buf), 0,
                                  (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (sent == -1) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("sendto() failed");
                    }
                } else {
                    printf("[Ban]: %s\n", send_buf);
                }
            }
        }

  
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        int len = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0,
                           (struct sockaddr *)&from_addr, &from_len);

        if (len == -1) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("recvfrom() failed");
            }
        } else if (len > 0) {
            recv_buf[len] = '\0';
            printf("[Tu %s:%d]: %s\n",
                   inet_ntoa(from_addr.sin_addr),
                   ntohs(from_addr.sin_port),
                   recv_buf);
        }

        usleep(10000);
    }

    close(sock);
    return 0;
}
