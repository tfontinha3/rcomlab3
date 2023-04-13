// transmitter prototype
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

bool send_packet(FILE file, int packet_num)
{
    char packet[132];
    int i, c;

    packet[0] = SOH;
    packet[1] = packet_num;
    packet[2] = ~packet_num;

    for (i = 0; i < 128; i++) {
        c = fgetc(file);
        if (c == EOF) {
            break;
        }
        packet[i + 3] = c;
    }

    if (i < 128) {
        memset(packet + i + 3, 0x1A, 128 - i);
    }

    unsigned char checksum = 0;
    for (i = 3; i < 131; i++) {
        checksum += packet[i];
    }
    packet[131] = checksum;

    for (i = 0; i < 10; i++) {
        fputc(packet[i], stdout);
    }

    c = fgetc(stdin);
    if (c == ACK) {
        return true;
    } else if (c == CAN) {
        printf("Received CAN, aborting\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Unexpected response 0x%02X, retrying\n", c);
        return false;
    }
}

int main(int argc, charargv[])
{
    if (argc != 2) {
        printf("Usage: xmodem <filename>\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        printf("Failed to open file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int packet_num = 1;
    bool sent_packet;
    do {
        sent_packet = send_packet(file, packet_num);
        packet_num++;
    } while (sent_packet);

    fputc(EOT, stdout);
    int c = fgetc(stdin);
    if (c == ACK) {
        printf("File sent successfully\n");
    } else if (c == CAN) {
        printf("Received CAN, aborting\n");
        return EXIT_FAILURE;
    } else {
        printf("Unexpected response 0x%02X\n", c);
        return EXIT_FAILURE;
    }

    fclose(file);
    return EXIT_SUCCESS;
}