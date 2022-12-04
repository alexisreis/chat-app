# 📨 chat-app

Application serveur / client, crée avec des sockets en C pour pouvoir communiquer à plusieurs dans des conversations. 

>Développée par Simon Poulet et Alexis Reis dans le cadre d'un projet de programmation réseau.

# Comment lancer ?
Il suffit de créer les fichier exécutables via la commande `make` dans le repertoire racine du projet. 

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

Si le client existe déjà, il doit s'ensuite s'authentifier avec son mot de passe. Si le client n'existe pas il renseigne le mot de passe qu'il souhaite utiliser. Ces mots de passe sont stockés et envoyés hashés. 

> 🚧 Attention
> 
> Le pseudo de l'utilisateur est **unique**. A la reconnexion au serveur si le client souhaite récupérer ses données il ldoit se connecter avec le même pseudo où un nouveau client sera créé. De même il et impossible que deux clients se connecte avec le même pseudo simultanément. 

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

## Historique et sauvergarde des données
Toutes les données sont stockées dans des fichiers pour pouvoir les recharger lors d'un redémarrage serveur.
Ces données sont :
* les clients (pseudos)
* les amis
* les consersations de groupe (membres et messages)
* les converstions privées (messages)

Ces fichiers sont stockés dans le repertoire `history/`

### Structure de la sauvegarde des clients
> 🚨 **Attention**
>
> **Tous les fichiers de sauvegarde doivent se terminer par un retour à la ligne `\n` où la lecture ne fonctionnera pas**

`history/clients.txt` :

```
Alexis
mot_de_passe_hashé_de_Alexis
Simon
mot_de_passe_hashé_de_Simon


```

### Structure de la sauvegarde des amis
L'amitié est réciproque mais n'est représentée qu'une seule fois. Mais les deux relations sont bien créés.
`history/friends.txt` :

```
Alexis -> Simon


```

### Structure de la sauvegarde des conversations de groupe

`history/groups.txt` :

```
AEDI


```

A chaque griupe est associé un nouveau fichier, sa liste de membres dans le repertoire `history/group` sous le format `nom_du_groupe.mbr`

Par exemple ici `history/group/AEDI.mbr` : 

```
Alexis
Simon


```

# Idées d'amélioration
* **chiffrer** les messages à l'envoi, les **déchiffrer** à la réception
* envoyer des notifications lors de la recpetion d'un message (privé ou groupe)
* envoyer toutes les notifications de messages reçus lors que l'utilisateur était hors-ligne
