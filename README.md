# 📨 chat-app

Application serveur / client, crée avec des sockets en C pour pouvoir communiquer à plusieurs dans des conversations. 

>Développée par Simon Poulet et Alexis Reis dans le cadre d'un projet de programmation réseau.

# Comment lancer ?
Il suffit de créer les fichiers exécutables via la commande `make` dans le répertoire racine du projet. 

```bash
make
```

## Démarrer le serveur de chat
Afin de démarrer le serveur, il suffit de lancer la commande :
```bash
./bin/server
```

## Démarrer une session utilisateur
Afin de se connecter au serveur et utiliser le chat, il suffit de lancer la commande : 
```bash
./bin/client [ADRESSE_DU_SERVEUR] [PSEUDO_DU_CLIENT]
```
`[ADRESSE_DU_SERVEUR]` est à remplacer par l'adresse IP de la machine qur laquelle a été démarré le serveur

`[PSEUDO_DU_CLIENT]` est le pseudo avec lequel le client souhaite se connecter au serveur pour communiquer avec d'autres utilisateurs

> 🚧 Attention
> 
> Le pseudo de l'utilisateur est **unique**. A la reconnexion au serveur si le client souhaite récupérer ses données il doit se connecter avec le même pseudo où un nouveau client sera créé. De même il est impossible que deux clients se connectent avec le même pseudo simultanément. 

# Fonctionnalités implémentées
## Commandes client
Du côté client, des commandes permettent d'effectuer des actions :
* `$addfriend` : ajouter un utilisateur en ami
* `$listfriends` : lister ses amis
* `$dm` : envoyer un message privé à un ami
* `$creategroup` : créer une nouvelle conversation de groupe
* `$addgroup` : ajouter un utilisateur à un groupe
* `$joingroup` : ouvrir une conversation de groupe
* `$listgroups` : lister ses conversations de groupe
* `$help` : afficher toutes les commandes

## Historique et sauvegarde des données
Toutes les données sont stockées dans des fichiers pour pouvoir les recharger lors d'un redémarrage serveur.
Ces données sont :
* les clients (pseudos)
* les amis
* les conversations de groupe (membres et messages)
* les conversations privées (messages)

Ces fichiers sont stockés dans le repertoire `history/`

### Structure de la sauvegarde des clients
> 🚨 **Attention**
>
> **Tous les fichiers de sauvegarde doivent se terminer par un retour à la ligne `\n` où la lecture ne fonctionnera pas**

`history/clients.txt` :

```
Alexis
Simon


```

### Structure de la sauvegarde des amis
`history/friends.txt` :

```
Alexis -> Simon


```

### Structure de la sauvegarde des conversations de groupe

`history/groups.txt` :

```
AEDI


```

A chaque groupe est associé un nouveau fichier, sa liste de membres se situe dans le répertoire `history/group` sous le format `nom_du_groupe.mbr`

Par exemple ici `history/group/AEDI.mbr` : 

```
Alexis
Simon


```

# Idées d'amélioration
* **chiffrer** les messages à l'envoi, les **déchiffrer** à la réception
* ajouter l'authentification de l'utilisateur par **mot de passe hashé**
* envoyer des notifications lors de la réception d'un message (privé ou groupe)
* envoyer toutes les notifications de messages reçus lorsque l'utilisateur était hors-ligne
