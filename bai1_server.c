
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#define SERVER_PORT 8080
#define MAX_CLIENTS 64
#define BUF_SIZE 512

typedef struct {
    int fd;
    int state;          
    char fullname[256];
    char buffer[BUF_SIZE];
    int buflen;
} ClientInfo;

void trim_string(char *s) {
    int len = strlen(s);

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || isspace((unsigned char)s[len - 1]))) {
        s[len - 1] = '\0';
        len--;
    }

    int start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;

    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

int is_valid_mssv(const char *mssv) {
    if (strlen(mssv) != 8) return 0;

    for (int i = 0; i < 8; i++) {
        if (!isdigit((unsigned char)mssv[i])) return 0;
    }
    return 1;
}

void normalize_vietnamese(const char *src, char *dst, int maxlen) {
    int i = 0, j = 0;

    while (src[i] != '\0' && j < maxlen - 1) {
        unsigned char c = (unsigned char)src[i];

        if (c < 128) {
            if (isalnum(c)) {
                dst[j++] = (char)tolower(c);
            } else if (isspace(c)) {
                dst[j++] = ' ';
            }
            i++;
            continue;
        }

        if (!strncmp(src + i, "à", 2) || !strncmp(src + i, "á", 2) || !strncmp(src + i, "ạ", 2) ||
            !strncmp(src + i, "ả", 2) || !strncmp(src + i, "ã", 2) || !strncmp(src + i, "ă", 2) ||
            !strncmp(src + i, "ắ", 3) || !strncmp(src + i, "ằ", 3) || !strncmp(src + i, "ẳ", 3) ||
            !strncmp(src + i, "ẵ", 3) || !strncmp(src + i, "ặ", 3) || !strncmp(src + i, "â", 2) ||
            !strncmp(src + i, "ấ", 3) || !strncmp(src + i, "ầ", 3) || !strncmp(src + i, "ẩ", 3) ||
            !strncmp(src + i, "ẫ", 3) || !strncmp(src + i, "ậ", 3) ||
            !strncmp(src + i, "À", 2) || !strncmp(src + i, "Á", 2) || !strncmp(src + i, "Ạ", 2) ||
            !strncmp(src + i, "Ả", 2) || !strncmp(src + i, "Ã", 2) || !strncmp(src + i, "Ă", 2) ||
            !strncmp(src + i, "Â", 2)) {
            dst[j++] = 'a';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "è", 2) || !strncmp(src + i, "é", 2) || !strncmp(src + i, "ẹ", 2) ||
                 !strncmp(src + i, "ẻ", 2) || !strncmp(src + i, "ẽ", 2) || !strncmp(src + i, "ê", 2) ||
                 !strncmp(src + i, "ế", 3) || !strncmp(src + i, "ề", 3) || !strncmp(src + i, "ể", 3) ||
                 !strncmp(src + i, "ễ", 3) || !strncmp(src + i, "ệ", 3) ||
                 !strncmp(src + i, "È", 2) || !strncmp(src + i, "É", 2) || !strncmp(src + i, "Ẹ", 2) ||
                 !strncmp(src + i, "Ẻ", 3) || !strncmp(src + i, "Ẽ", 3) || !strncmp(src + i, "Ê", 2)) {
            dst[j++] = 'e';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "ì", 2) || !strncmp(src + i, "í", 2) || !strncmp(src + i, "ị", 2) ||
                 !strncmp(src + i, "ỉ", 3) || !strncmp(src + i, "ĩ", 3) ||
                 !strncmp(src + i, "Ì", 2) || !strncmp(src + i, "Í", 2) || !strncmp(src + i, "Ị", 2)) {
            dst[j++] = 'i';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "ò", 2) || !strncmp(src + i, "ó", 2) || !strncmp(src + i, "ọ", 2) ||
                 !strncmp(src + i, "ỏ", 3) || !strncmp(src + i, "õ", 2) || !strncmp(src + i, "ô", 2) ||
                 !strncmp(src + i, "ơ", 2) || !strncmp(src + i, "ố", 3) || !strncmp(src + i, "ồ", 3) ||
                 !strncmp(src + i, "ổ", 3) || !strncmp(src + i, "ỗ", 3) || !strncmp(src + i, "ộ", 3) ||
                 !strncmp(src + i, "ớ", 3) || !strncmp(src + i, "ờ", 3) || !strncmp(src + i, "ở", 3) ||
                 !strncmp(src + i, "ỡ", 3) || !strncmp(src + i, "ợ", 3) ||
                 !strncmp(src + i, "Ò", 2) || !strncmp(src + i, "Ó", 2) || !strncmp(src + i, "Ọ", 2) ||
                 !strncmp(src + i, "Ô", 2) || !strncmp(src + i, "Ơ", 2)) {
            dst[j++] = 'o';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "ù", 2) || !strncmp(src + i, "ú", 2) || !strncmp(src + i, "ụ", 2) ||
                 !strncmp(src + i, "ủ", 3) || !strncmp(src + i, "ũ", 2) || !strncmp(src + i, "ư", 2) ||
                 !strncmp(src + i, "ứ", 3) || !strncmp(src + i, "ừ", 3) || !strncmp(src + i, "ử", 3) ||
                 !strncmp(src + i, "ữ", 3) || !strncmp(src + i, "ự", 3) ||
                 !strncmp(src + i, "Ù", 2) || !strncmp(src + i, "Ú", 2) || !strncmp(src + i, "Ụ", 2) ||
                 !strncmp(src + i, "Ư", 2)) {
            dst[j++] = 'u';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "ỳ", 3) || !strncmp(src + i, "ý", 2) || !strncmp(src + i, "ỵ", 3) ||
                 !strncmp(src + i, "ỷ", 3) || !strncmp(src + i, "ỹ", 3) ||
                 !strncmp(src + i, "Ý", 2)) {
            dst[j++] = 'y';
            i += (src[i] == '\0') ? 1 : ((unsigned char)src[i] < 0xE0 ? 2 : 3);
        }
        else if (!strncmp(src + i, "đ", 2) || !strncmp(src + i, "Đ", 2)) {
            dst[j++] = 'd';
            i += 2;
        }
        else {
            i++;
        }
    }

    dst[j] = '\0';

    char temp[256];
    int k = 0;
    int prev_space = 1;

    for (i = 0; dst[i] != '\0' && k < (int)sizeof(temp) - 1; i++) {
        if (dst[i] == ' ') {
            if (!prev_space) {
                temp[k++] = ' ';
                prev_space = 1;
            }
        } else {
            temp[k++] = dst[i];
            prev_space = 0;
        }
    }

    if (k > 0 && temp[k - 1] == ' ') k--;
    temp[k] = '\0';
    strcpy(dst, temp);
}

int build_email(const char *fullname, const char *mssv, char *email, int maxlen) {
    char normalized[256];
    char temp[256];
    char *parts[20];
    int count = 0;

    normalize_vietnamese(fullname, normalized, sizeof(normalized));
    strcpy(temp, normalized);

    char *token = strtok(temp, " ");
    while (token != NULL && count < 20) {
        parts[count++] = token;
        token = strtok(NULL, " ");
    }

    if (count < 2) return -1;

    char *ten = parts[count - 1];
    char ho = parts[0][0];
    char dem = parts[1][0];

    snprintf(email, maxlen, "%s.%c%c%s@sis.hust.edu.vn",
             ten, ho, dem, mssv + 2);

    return 0;
}

void send_msg(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

void remove_client(ClientInfo clients[], int *nclients, int idx) {
    close(clients[idx].fd);

    for (int i = idx; i < *nclients - 1; i++) {
        clients[i] = clients[i + 1];
    }

    (*nclients)--;
}

void process_input_line(ClientInfo clients[], int *nclients, int idx, char *line) {
    trim_string(line);

    if (clients[idx].state == 0) {
        if (strlen(line) == 0) {
            send_msg(clients[idx].fd, "Ho ten khong hop le. Vui long nhap lai:\nHo ten: ");
            return;
        }

        strncpy(clients[idx].fullname, line, sizeof(clients[idx].fullname) - 1);
        clients[idx].fullname[sizeof(clients[idx].fullname) - 1] = '\0';
        clients[idx].state = 1;

        send_msg(clients[idx].fd, "MSSV: ");
    }
    else if (clients[idx].state == 1) {
        if (!is_valid_mssv(line)) {
            send_msg(clients[idx].fd, "MSSV: ");
            return;
        }

        char email[256];
        if (build_email(clients[idx].fullname, line, email, sizeof(email)) != 0) {
            send_msg(clients[idx].fd, "Khong the tao email tu du lieu da nhap.\n");
            remove_client(clients, nclients, idx);
            return;
        }

        char reply[512];
        snprintf(reply, sizeof(reply),
                 "Ho ten: %s\nMSSV: %s\nEmail: %s\n",
                 clients[idx].fullname, line, email);

        send_msg(clients[idx].fd, reply);
        remove_client(clients, nclients, idx);
    }
}


int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port %d...\n", SERVER_PORT);

    ClientInfo clients[MAX_CLIENTS];
    int nclients = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].state = 0;
        clients[i].buflen = 0;
        clients[i].fullname[0] = '\0';
        clients[i].buffer[0] = '\0';
    }

    char buf[256];

    while (1) {
        while (1) {
            int client = accept(listener, NULL, NULL);
            if (client == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    break;
                } else {
                    perror("accept() failed");
                    break;
                }
            }

            if (nclients >= MAX_CLIENTS) {
                send_msg(client, "Server dang qua tai.\n");
                close(client);
                continue;
            }

            ul = 1;
            ioctl(client, FIONBIO, &ul);

            clients[nclients].fd = client;
            clients[nclients].state = 0;
            clients[nclients].buflen = 0;
            clients[nclients].fullname[0] = '\0';
            clients[nclients].buffer[0] = '\0';

            printf("New client accepted: %d\n", client);
            send_msg(client, "Chao ban!\nHo ten: ");
            nclients++;
        }
        for (int i = 0; i < nclients; i++) {
            int len = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

            if (len == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                } else {
                    printf("Client %d disconnected or recv error.\n", clients[i].fd);
                    remove_client(clients, &nclients, i);
                    i--;
                    continue;
                }
            }

            if (len == 0) {
                printf("Client %d closed connection.\n", clients[i].fd);
                remove_client(clients, &nclients, i);
                i--;
                continue;
            }

            buf[len] = '\0';

            if (clients[i].buflen + len >= BUF_SIZE - 1) {
                send_msg(clients[i].fd, "Du lieu qua dai.\n");
                remove_client(clients, &nclients, i);
                i--;
                continue;
            }

            memcpy(clients[i].buffer + clients[i].buflen, buf, len);
            clients[i].buflen += len;
            clients[i].buffer[clients[i].buflen] = '\0';

            char *newline;
            while ((newline = strchr(clients[i].buffer, '\n')) != NULL) {
                char line[BUF_SIZE];
                int linelen = newline - clients[i].buffer;
                strncpy(line, clients[i].buffer, linelen);
                line[linelen] = '\0';

                int remaining = clients[i].buflen - linelen - 1;
                memmove(clients[i].buffer, newline + 1, remaining);
                clients[i].buffer[remaining] = '\0';
                clients[i].buflen = remaining;

                process_input_line(clients, &nclients, i, line);

                if (i >= nclients) break;
            }
        }

        usleep(10000); 
    }

    close(listener);
    return 0;
}
