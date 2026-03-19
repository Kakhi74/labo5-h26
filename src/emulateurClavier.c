/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"
#include <stdlib.h>

FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    if (f == NULL){
        perror("fopen failed");
        exit(1);
    }
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}

int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes) {
    unsigned char buf[LONGUEUR_USB_PAQUET] = {0};
    size_t buf_pos;
    size_t i = 0;
    unsigned char prev, hid, shift;
    while (i < len){
        prev = 0;
        buf_pos = 2;
        // space could be either pressed with shift or not, when it is the first character, we will adjust to the shift of the next character
        if (caracteres[i] == ' ') {
            buf[buf_pos] = 44;
            prev = buf[buf_pos];
            ++i;
            ++buf_pos;
        }
        while (buf_pos < LONGUEUR_USB_PAQUET && i < len){
            shift = 0;
            if (caracteres[i] >= 'A' && caracteres[i] <= 'Z') {
                shift = 2;
                hid = caracteres[i] - 'A' + 4;
            } else if (caracteres[i] == ' ') {
                shift = buf[0];
                hid = 44;
            } else if (caracteres[i] >= '1' && caracteres[i] <= '9') {
                hid = caracteres[i] - '1' + 30;
            } else if (caracteres[i] >= 'a' && caracteres[i] <= 'z') {
                hid = caracteres[i] - 'a' + 4;
            } else if (caracteres[i] == '0') {
                hid = 39;
            } else if (caracteres[i] == '\n') {
                hid = 40;
            } else if (caracteres[i] == ',') {
                hid = 54;
            } else if (caracteres[i] == '.') {
                hid = 55;
            } else {
                return -1;
            }
            // check that current HID is bigger than prev HID, because even tho they are in the correct order in the packet, the output is always interpreted in ascending order of HID
            // checking hid < prev solves the issues I reported at
            // https://discord.com/channels/1455270549025587252/1455272461154390077/threads/1482141859810906253
            if (hid <= prev) break;
            if (!(buf_pos == 3 && buf[2] == 44) && buf_pos != 2 && shift != buf[0]) break;
            buf[0] = shift;
            buf[buf_pos] = hid;
            prev = hid;
            ++i;
            ++buf_pos;
        }
        // printf("Sent packet content : [%i, %i, %i, %i, %i, %i, %i, %i]\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        if (fwrite(buf, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        memset(buf, 0, LONGUEUR_USB_PAQUET);
        if (fwrite(buf, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        usleep(tempsTraitementParPaquetMicroSecondes);
    }
    return (int)i;
}