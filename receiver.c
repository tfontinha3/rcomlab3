// receiver prototype
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

bool receive_packet(FILE *file, int packet_num)
{
    char packet[132];
    int i, c;

    for (i = 0; i < 10; i++) {
        c = fgetc(stdin);
        packet[i] = c;
    }

    if (packet[0] != SOH) {
        printf("Unexpected packet header 0x%02X, retrying\n", packet[0]);
        return false;
    }

    if (packet[1] != packet_num) {
        printf("Unexpected packet number %d, retrying\n", packet[1]);
        return false;
    }

    if ((packet[1] + packet[2]) != 0xFF) {
        printf("Packet number complement error, retrying\n");
        return false;
    }

    unsigned char checksum = 0;
    for (i = 3; i < 131; i++) {
        checksum += packet[i];
    }
    if (checksum != packet[131]) {
        printf("Checksum error, retrying\n");
        return false;
    }

    for (i = 3; i < 131; i++) {
        fputc(packet[i], file);
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: xmodem <filename>\n");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "wb");
    if (file == NULL) {
        printf("Failed to open file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    bool received_packet;
    int packet_num = 1;
    do {
        fputc(NAK, stdout);
        received_packet = receive_packet(file, packet_num);
        if (received_packet) {
            packet_num++;
        }
    } while (!received_packet);

    fputc(ACK, stdout);
    int c = fgetc(stdin);
    if (c == EOT) {
        fputc(ACK, stdout);
        printf("File received successfully\n");
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
*/