/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de gestion du tampon circulaire
 ******************************************************************************/

#include "tamponCirculaire.h"

// Plusieurs variables globales statiques (pour qu'elles ne soient accessible que dans les
// fonctions de ce fichier) sont declarees ici. Elle servent a conserver l'etat du tampon
// circulaire ainsi qu'a mesurer certains elements utiles au calcul des statistiques.
// Vous etes libres d'en creer d'autres si vous en voyez le besoin.

// Pointe vers la memoire allouee pour le tampon circulaire
static char* memoire;

// Taile du tampon circulaire (en nombre d'elements de type struct requete)
static size_t memoireTaille;

// Positions de lecture et d'ecriture, et longueur actuelle du tampon circulaire
static unsigned int posLecture, posEcriture, longueurCourante;

// Mutex permettant de proteger les acces au tampon circulaire
// N'oubliez pas que _deux_ threads vont tenter de faire des operations en parallele!
pthread_mutex_t mutexTampon;

// Pour les statistiques
static unsigned int nombreRequetesRecues, nombreRequetesTraitees, nombreRequetesPerdues;

// Le tempsDebutPeriode permet de se rappeler du debut de la periode ou les statistiques sont mesurees
// sommeTempsAttente contient la somme de toutes les periodes d'attente pour les requetes
// (vous pourrez donc calculer la moyenne du temps d'attente en utilisant les autres variables sur le
// nombre de requetes).
static double tempsDebutPeriode, sommeTempsAttente;

int initTamponCirculaire(size_t taille){
    // Initialisez ici:
    // La memoire, en utilisant malloc ou calloc (rappelez-vous que votre tampon circulaire doit
    // pouvoir contenir _taille_ fois la taille d'une struct requete)
    //
    // Les positions de lecture, d'ecriture et de longueur courante.
    //
    // Le mutex
    //
    // Les variables de statistiques

    // TODO
    memoireTaille = taille;
    memoire = malloc(taille * sizeof(struct requete));
    if (!memoire){
        perror("malloc failed");
        return -1;
    }

    posLecture = 0;
    posEcriture = 0;
    longueurCourante = 0;

    if (pthread_mutex_init(&mutexTampon, NULL) != 0){
        perror("pthread_mutex_init(&mutexTampon) failed");
        return -1;
    }

    resetStats();

    return 0;
}

void resetStats(){
    // Reinitialise les variables de statistique

    // TODO
    // QUESTION_PROF: proteger par mutex?
    // pthread_mutex_lock(&mutexTampon);
    nombreRequetesRecues = 0;
    nombreRequetesTraitees = 0;
    nombreRequetesPerdues = 0;

    sommeTempsAttente = 0;
    tempsDebutPeriode = get_time();
    // pthread_mutex_lock(&mutexTampon);
}

void calculeStats(struct statistiques *stats){
    // TODO
    // QUESTION_PROF: proteger par mutex?
    // pthread_mutex_lock(&mutexTampon);
    stats->nombreRequetesTraitees = nombreRequetesTraitees;
    stats->nombreRequetesPerdues = nombreRequetesPerdues;
    stats->nombreRequetesEnAttente = longueurCourante;
    
    stats->tempsTraitementMoyen= sommeTempsAttente / nombreRequetesTraitees;

    double dt = get_time() - tempsDebutPeriode;
    stats->lambda = nombreRequetesRecues / dt;
    stats->mu = nombreRequetesTraitees / dt;
    stats->rho = stats->lambda / stats->mu;
    // pthread_mutex_unlock(&mutexTampon);
}

int insererDonnee(struct requete *req){
    // Dans cette fonction, vous devez :
    //
    // Determiner a quel endroit copier la requete req dans le tampon circulaire
    //
    // Copier celle-ci
    //
    // Mettre a jour posEcriture et longueurCourante (toujours) et possiblement
    // posLecture (si vous vous etes "mordu la queue" et que vous etes revenu au
    // debut de votre tampon circulaire, il faut aussi repousser le pointeur de lecture
    // pour que le prochain element lu soit le plus ancien!)
    //
    // Mettre a jour les variables necessaires aux statistiques (comme nombreRequetesRecues, par exemple)
    //
    // N'oubliez pas de proteger les operations qui le necessitent par un mutex!
   
    // TODO
    pthread_mutex_lock(&mutexTampon);
    if (longueurCourante == memoireTaille){
        posLecture = (posLecture + 1) % memoireTaille;
        ++nombreRequetesPerdues;
    } else {
        ++longueurCourante;
    }
    memcpy((struct requete *)memoire + posEcriture, req, sizeof(struct requete));
    posEcriture = (posEcriture + 1) % memoireTaille;
    ++nombreRequetesRecues;
    pthread_mutex_unlock(&mutexTampon);

    return 0;
}

int consommerDonnee(struct requete *req){
    // Dans cette fonction, vous devez :
    //
    // Determiner si une requete est disponible dans le tampon circulaire
    //
    // S'il n'y en a _pas_, retourner 0.
    //
    // S'il y en a une, alors :
    //      Copier cette requete dans la structure passee en argument
    //      Modifier la valeur de posLecture et longueurCourante
    //      Mettre a jour les variables necessaires aux statistiques (comme sommeTempsAttente)
    //      Retourner 1 pour indiquer qu'une requete disponible a ete copiee dans req.
    //
    // N'oubliez pas de proteger les operations qui le necessitent par un mutex!
    
    // TODO
    // QUESTION_PROF: should i make the get_time() call outside of mutex lock?
    pthread_mutex_lock(&mutexTampon);
    if (longueurCourante == 0){
        pthread_mutex_unlock(&mutexTampon);
        return 0;
    }
    memcpy(req, (struct requete *)memoire + posLecture, sizeof(struct requete));
    posLecture = (posLecture + 1) % memoireTaille;
    --longueurCourante;
    ++nombreRequetesTraitees;
    sommeTempsAttente += get_time() - req->tempsReception;;
    pthread_mutex_unlock(&mutexTampon);

    return 1;
}

unsigned int longueurFile(){
    // Retourne la longueur courante de la file contenue dans votre tampon circulaire.
    
    // TODO
    return longueurCourante;
}
