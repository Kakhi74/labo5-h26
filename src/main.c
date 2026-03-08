/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier principal
 ******************************************************************************/

#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "emulateurClavier.h"
#include "tamponCirculaire.h"



static void* threadFonctionClavier(void* args){
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee
    struct requete req;
    while(1){
        // TODO
        if (consommerDonnee(&req) == 0){
            usleep(500);
        } else {
            // TODO: review ecrire caracteres, la portion req.data, not sure
            if (ecrireCaracteres(infos->pointeurClavier, req.data, req.taille, infos->tempsTraitementParCaractereMicroSecondes) < 0){
                fprintf(stderr, "ecrireCaracteres failed");
                pthread_exit(1);
            }
            free(req.data);
        }
    }
    return NULL;
}

static void* threadFonctionLecture(void *args){

    // Implementez ici votre fonction de thread pour la lecture sur le named pipe
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadLecture *infos = (struct infoThreadLecture *)args;
    
    // Ces champs vous seront utiles pour l'appel a select()
    fd_set setFd;
    int nfds = infos->pipeFd + 1;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Remplir setFd en utilisant FD_ZERO et FD_SET correctement, pour faire en sorte
    //      d'attendre sur infos->pipeFd
    // 2) Appeler select(), sans timeout, avec setFd comme argument de lecture (on veut bien
    //      lire sur le pipe)
    // 3) Lire les valeurs sur le named pipe
    // 4) Si une de ses valeurs est le caracteres ASCII EOT (0x4), alors c'est la fin d'un
    //      message. Vous creez alors une nouvelle requete et utilisez insererDonnee() pour
    //      l'inserer dans le tampon circulaire. Notez que le caractere EOT ne doit PAS se
    //      retrouver dans le champ data de la requete! N'oubliez pas egalement de donner
    //      la bonne valeur aux champs taille et tempsReception.

    struct requete req;
    ssize_t bytes;
    size_t bytes_read;
    size_t buf_size;
    int eot;
    while(1){
        // TODO
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);
        if (select(nfds, &setFd, NULL, NULL, NULL) > 0){
            if (FD_ISSET(infos->pipeFd, &setFd)){
                eot = 0;
                bytes = 0;
                bytes_read = 0;
                buf_size = 1024;
                req.data = malloc(buf_size);
                if (!req.data){
                    perror("malloc failed");
                    pthread_exit(1);
                }
                while (!eot) {
                    if (bytes_read == buf_size){
                        buf_size *= 2;
                        req.data = realloc(req.data, buf_size);
                        if (!req.data){
                            perror("realloc failed");
                            pthread_exit(1);
                        }
                    }
                    bytes = read(infos->pipeFd, req.data + bytes_read, buf_size - bytes_read);
                    if (bytes == 0){
                        fprintf(stderr, "payload is missing eot");
                        pthread_exit(1);
                    }
                    if (bytes < 0){
                        perror("read failed");
                        pthread_exit(1);
                    }
                    while (bytes > 0){
                        if (req.data[bytes_read] == 0x4){
                            eot = 1;
                            --bytes_read;
                            req.data = realloc(req.data, bytes_read);
                            req.taille = bytes_read;
                            req.tempsReception = get_time();
                            insererDonnee(&req);
                            break;
                        }
                        ++bytes_read;
                        --bytes;
                    }
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){
    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }

    // A ce stade, vous pouvez consider que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire

    // Vous avez plusieurs taches d'initialisation a faire :
    //
    // 1) Ouvrir le named pipe

    // TODO
    // TODO: Check later if we need read and write
    int pipeFd = open(argv[1], O_RDWR, 0777);
    if (pipeFd < 0){
        perror("open failed");
        return -1;
    }

    // 2) Declarer et initialiser la barriere
    
    // TODO
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, 2);

    // 3) Initialiser le tampon circulaire avec la bonne taille

    // TODO
    size_t buf_size = atoi(argv[3]);
    if (initTamponCirculaire(buf_size) < 0){
        fprintf(stderr, "initTamponCirculaire(%d) failed", buf_size);
        return -1;
    }

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective
    
    // TODO
    pthread_t thread_clavier;
    struct infoThreadClavier info_clavier;
    info_clavier.barriere = &barrier;
    info_clavier.pointeurClavier = initClavier();
    info_clavier.tempsTraitementParCaractereMicroSecondes = atoi(argv[2]);
    pthread_create(&thread_clavier, NULL, threadFonctionClavier, &info_clavier);

    pthread_t thread_lecteur;
    struct infoThreadLecture info_lecture;
    info_lecture.barriere = &barrier;
    info_lecture.pipeFd = pipeFd;
    pthread_create(&thread_lecteur, NULL, threadFonctionLecture, &info_lecture);

    pthread_join(thread_clavier, NULL);
    pthread_join(thread_lecteur, NULL);

    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les 2 secondes
        calculeStats(&stats);
        afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(2e6);
    }
    return 0;
}
