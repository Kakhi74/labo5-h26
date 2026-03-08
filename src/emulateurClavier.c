/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"

FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}


int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    // TODO ecrivez votre code ici. Voyez les explications dans l'enonce et dans
    // emulateurClavier.h
    // numbers 48 - 57
    // lower 97 - 122
    // upper 65 - 90
    char buf[LONGUEUR_USB_PAQUET] = {0};
    size_t buf_pos;
    size_t i = 0;
    while (i < len){
        buf_pos = 2;
        if (caracteres[i] >= 'A' && caracteres[i] <= 'Z'){ // upper
            buf[0] = 2;
            buf[buf_pos++] = caracteres[i++] - 'A' + 4;
            while (buf_pos < LONGUEUR_USB_PAQUET && i < len){
                if (caracteres[i] >= 'A' && caracteres[i] <= 'Z'){
                    buf[buf_pos++] = caracteres[i++] - 'A' + 4;
                } else if (caracteres[i] == ' '){
                    buf[buf_pos++] = 44;
                    ++i;
                } else {
                    if (caracteres[i] == '0'
                        || caracteres[i] == ','
                        || caracteres[i] == '.'
                        || caracteres[i] == '\n'
                        || (caracteres[i] >= 'a' && caracteres[i] <= 'z')
                        || (caracteres[i] >= '1' && caracteres[i] <= '9')){
                        break;
                    }
                    return -1;
                }
            }
        } else {
            while (buf_pos < LONGUEUR_USB_PAQUET && i < len){
                if (caracteres[i] >= 'a' && caracteres[i] <= 'z'){ // lower
                    buf[buf_pos++] = caracteres[i++] - 'a' + 4;
                } else if (caracteres[i] >= '1' && caracteres[i] <= '9'){ // 1..9
                    buf[buf_pos++] = caracteres[i++] - '1' + 30;
                } else if (caracteres[i] == '0'){
                    buf[buf_pos++] = 39;
                    ++i;
                } else if (caracteres[i] == ','){
                    buf[buf_pos++] = 54;
                    ++i;
                } else if (caracteres[i] == '.'){
                    buf[buf_pos++] = 55;
                    ++i;
                } else if (caracteres[i] == '\n'){
                    buf[buf_pos++] = 40;
                    ++i;
                } else if (caracteres[i] == ' '){
                    buf[buf_pos++] = 44;
                    ++i;
                } else {
                    if (caracteres[i] >= 'A' && caracteres[i] <= 'Z'){
                        break;
                    }
                    return -1;
                }
            }
        }
        if (fwrite(buf, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        memset(buf, 0, LONGUEUR_USB_PAQUET);
        if (fwrite(buf, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        usleep(tempsTraitementParPaquetMicroSecondes);
    }
    return i;
}
