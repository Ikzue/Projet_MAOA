#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Nov 29 14:16:07 2019

@author: 3528315
"""

import cvrp
from operator import itemgetter, attrgetter
import math
import random as rd
import numpy as np

#On considere que le depot est egal au sommet 1
def roads_list(I):
    Q = I.Q
    nodes = list(I.G.nodes)
    nodes_with_neighbours = get_neighbours(nodes) #dict sommet -> liste [sommet_voisin, distance, demande]
    list_roads = []
    while len(nodes) > 1 : 
        road = get_road(nodes_with_neighbours,nodes,Q)
        #print("Chemin trouve:  ",road)
        #print("Liste noeuds pas encore pris: ",nodes)
        list_roads.append(road)
    score = get_score(list_roads,I.G)
    return (list_roads,score)


def get_neighbours(nodes):#Retourne un dictionnaire avec en cle le sommet, en valeur une liste de triplets de voisins [sommet_voisin, distance, demande]
    nodes_with_neighbours = {}
    for i in range(0,len(nodes)):
        neighbours = []
        for key,value in I.G[nodes[i]].items():
            weight = value['weight']
            neighbours.append((key,weight,I.get_demand(key))) #[Sommet_voisin, poids, demande]
        neighbours = sorted(neighbours,key=itemgetter(1)) #On trie en fonction de la distance des aretes
        nodes_with_neighbours[nodes[i]] = neighbours
    return nodes_with_neighbours

def get_road(nodes_with_neighbours,nodes,Q): #retourne le plus court chemin partant du sommet 1 sous forme de liste [1,noeud1,...,1]
    neighbour_found = True
    current_node = 1
    road = [1]
    while(neighbour_found):#Tant qu'on trouve un voisin dont la demande peut etre satisfaite
        neighbour_found = False
        neighbours = nodes_with_neighbours[current_node]
        for n in neighbours: #On parcourt tous les voisins du noeud actuel
            if n[0] in nodes and n[0] != 1 and n[2]<Q: #Si le noeud n'a pas deja ete pris ET le voisin n'est pas le depot ET si la demande du noeud est inferieure a la capacite du camion
                neighbour_found = True
                next_node = n[0] #Prochain noeud a tester
                road.append(next_node)
                Q = Q - n[2]
                nodes.remove(next_node)
                current_node = next_node
                break
    road.append(1)
    return road

def get_score(roads, G): #Obtient la somme des distances de la solution globale
    score = 0
    for road in roads:
        score += get_road_score(road,G)
    return score


def get_road_score(road,G): #Obtient la distance parcourue dans un chemin
    score = 0
    for i in range(len(road)-1):
        score += G[road[i]][road[i+1]]['weight']
    return score

def road_to_clients(road):
    return road[1:-1] if len(road)>1 else []

def clients_to_road(clients, depot = 1):
    return [depot] + clients + [depot]

I = cvrp.Instance("Instances/A\A-n32-k5.vrp")
#I = cvrp.Instance("Instances/A\A-n65-k9.vrp") #Opt=9,nombre de camions = 10
#I = cvrp.Instance("Instances/A\A-n32-k5.vrp")
roads, score = roads_list(I)

#print("nombre de camions: ",len(roads))
#print("score: ", score)
#print(roads)







"""
Première étape : amélioration indépendante des tournées
Pour chaque route :
    Calcul du poids de la route
    Pour chaque client de la route :
        Pour chaque autre client de la route :
            Tentative d'échange entre les deux :
                Calcul du poids de la route résultant de l'échange
        Si un échange bat le poids actuel de la route on l'effectue et on actualise le poids
"""
def local_amelioration(road, I):
    best_score = get_road_score(road, I.G)
    clients = road_to_clients(road)
    if len(clients) <= 1:
        return road
    best_swap = (1,1)
    for i, client in enumerate(clients):
        for j, client2 in enumerate(clients):
            # Creation d'une nouvelle liste de clients
            new_clients = list(clients)

            # Inversement des positions dans la nouvelle liste
            new_clients[i], new_clients[j] = new_clients[j], new_clients[i]
            
            # Calcul du nouveau score
            new_score = get_road_score(clients_to_road(new_clients), I.G)

            if new_score < best_score:
                best_score = new_score
                best_swap = (i, j)

    i, j = best_swap
    new_clients = list(clients)
    new_clients[i], new_clients[j] = new_clients[j], new_clients[i]

    return clients_to_road(new_clients)


def chaining_local_ameliration(road, I):
    # On améliore tant que le score s'améliore
    while True:
        best_score = get_road_score(road, I.G)
        new_road = local_amelioration(road, I)
        new_score = get_road_score(new_road, I.G)
        if best_score == new_score:
            break
        else :
            road = new_road

    return new_road

def local_ameliorations(roads, I):
    return [chaining_local_ameliration(road, I) for road in roads]

#locally_ameliorated_roads = local_ameliorations(roads, I)
#print(get_score(roads, I.G))
#print(get_score(locally_ameliorated_roads, I.G))


"""
Deuxième étape : échanges/insertion entre routes


TODO : Ajouter du recuit simulé pour la première phase
"""

def road_demand(road, I):
    """
    Calcul la somme des demandes d'une route
    """
    return sum([I.G.nodes[c]['demand'] for c in road])

def roads_demands(roads, I):
    return [road_demand(road, I) for road in roads]

def weight_ponderation_vector(roads, I, maximisation = True, exponant = 2):
    """
    Prends une liste de routes et renvoie un vecteur de probabilité pondérant fortement 
    les routes lourdes/faibles selon si maximisation = True/False
    """
    weights = []
    scores = [get_road_score(road, I.G) for road in roads]
    if maximisation:
        # En maximisation, on calcul les différences au score le plus bas
        min_weight = min(scores)
        weights = [get_road_score(road, I.G) - min_weight for road in roads]
    else:
        # En minimisation, on calcul les différences au score le plus haut
        max_weight = max(scores)
        weights = [max_weight - get_road_score(road, I.G) for road in roads]

    # On met les score à l'exposant 'exponant' pour accentuer la pondération
    incr_weights = [w**exponant for w in weights]
    total_incr_weights = sum(incr_weights)
    probabilities = [w / total_incr_weights for w in incr_weights]

    return probabilities

def insertable_client(client, road_to_go, I):
    """
    Prends un client et une route et calcule si on peut mettre le client dans la route
    """

    space = I.Q - road_demand(road_to_go, I)
    return space >= I.G.nodes[client]['demand']

def insert_client(client, road_to_go, I):
    """
    Prends une route et un client et le place à l'endroit qui maximise le nouveau score de sa route
    """
    assert insertable_client(client, road_to_go, I)

    best_place = 0
    best_score = 1e5
    clients = road_to_clients(road_to_go)

    # Test d'insertion dans les n = len(clients) premières places
    for i in range(len(clients)):
        new_clients = clients[0:i] + [client] + clients[i:len(clients)]
        new_score = get_road_score(clients_to_road(new_clients), I.G)
        if new_score < best_score :
            best_score = new_score
            best_place = i

    # Test d'insertion à la fin
    new_clients = clients + [client]
    new_score = get_road_score(clients_to_road(new_clients), I.G)
    if new_score < best_score :
        return clients_to_road(new_clients)

    return clients_to_road(clients[0:best_place] + [client] + clients[best_place:len(clients)])

def weighted_choice(weights):
    P = np.random.uniform()
    i = -1
    w = 0
    while P > w:
        i += 1
        w += weights[i]
    return i

def roads_from_exchange(roads, I):
    """
    CALCUL DU VOISIN DANS LE CAS ECHANGE :
    On tire deux routes au hasard
    On tire deux clients parmi ces routes jusqu'à ce que les deux puissent aller dans les routes
    Chaque client choisit la place qui optimise le score de sa route
    La nouvelle liste obtenue avec l'échange de clients à leur nouvelles places est retournée
    """

    # Choix des routes aléatoire
    first_road = rd.randrange(0, len(roads))
    second_road = rd.randrange(0, len(roads))
    while second_road == first_road and len(roads) > 1:
        second_road = rd.randrange(0, len(roads))
    
    # Conversion en liste de clients
    first_clients = road_to_clients(roads[first_road])
    second_clients = road_to_clients(roads[second_road])

    # Choix des clients échangeables entre les deux routes selon leurs demandes
    first_client = first_clients[rd.randrange(0, len(first_clients))]
    second_client = second_clients[rd.randrange(0, len(second_clients))]
    first_clients_wtht = [c for c in first_clients if c != first_client]
    second_clients_wtht = [c for c in second_clients if c != second_client]

    k = 0
    while ((not insertable_client(first_client, second_clients_wtht, I)) or \
        (not insertable_client(second_client, first_clients_wtht, I))) and \
        k < 20:
        
        first_client = first_clients[rd.randrange(0, len(first_clients))]
        second_client = second_clients[rd.randrange(0, len(second_clients))]
        first_clients_wtht = [c for c in first_clients if c != first_client]
        second_clients_wtht = [c for c in second_clients if c != second_client]
        k += 1


    insertable = k < 20
    if not insertable:
        return roads

    # Conversion de clients vers routes
    first_new_road = clients_to_road(first_clients_wtht)
    second_new_road = clients_to_road(second_clients_wtht)

    # Insertion des clients dans les nouvelles routes
    first_new_road = insert_client(second_client, first_new_road, I)
    second_new_road = insert_client(first_client, second_new_road, I)

    new_roads = [r for r in roads]
    new_roads[first_road] = first_new_road
    new_roads[second_road] = second_new_road

    return new_roads

def roads_from_insertion(roads, I):
    """
    CALCUL DU VOISIN DANS LE CAS INSERTION :
    On crée un vecteur de probabilité favorisant le choix d'une route lourde
    On crée un vecteur de probabilité favorisant le choix d'une route légère
    On tire une route lourde
    On tire une route légère
    On prends un client de la route lourde au hasard
    Il se met à chaque place possible de la route légère (si il peut y aller) :
        Pour chaque place on calcule le poids total de toutes les routes
    Il se place à la meilleure place
    La nouvelle liste de route obtenue est le voisin de current_roads : neighbour
    """

    heavy_road_index = weighted_choice(weight_ponderation_vector(roads, I))
    light_road_index = weighted_choice(weight_ponderation_vector(roads, I, maximisation = False))

    heavy_road = roads[heavy_road_index]
    light_road = roads[light_road_index]

    if heavy_road == light_road:
        return roads

    heavy_road_clients = road_to_clients(heavy_road)
    random_position = rd.randrange(0, len(heavy_road_clients))
    heavy_road_client = heavy_road_clients[random_position]
    t = 0
    while (not insertable_client(heavy_road_client, light_road, I)) and t<20:
        random_position = rd.randrange(0, len(heavy_road_clients))
        heavy_road_client = heavy_road_clients[random_position]
        t += 1

    insertable = t < 20
    if not insertable:
        return roads

    light_road = insert_client(heavy_road_client, light_road, I)
    heavy_road = heavy_road[:random_position+1] + heavy_road[random_position+2:]

    new_roads = [r for r in roads]
    new_roads[light_road_index] = light_road
    new_roads[heavy_road_index] = heavy_road

    return new_roads



def roads_exchange_simulated_aneling(roads, I, nb_iter = 1000):
    """
    Recuit simulé :

    Tant que k < kmax :
        CHOIX DU CAS :
        Calcul des sommes des demandes de current_roads
        Si random() > min / max :
            INSERTION
        Sinon :
            ECHANGE

        CALCUL D'INSERTION OU NON DU VOISIN :
        s = score(current_roads)
        sn = score(neighbour)
        Si sn < s OU random() < exp((s-sn) / T) :
            current_roads = neighbour
        k += 1

    retourner current_roads
    """

    current_roads = roads
    score = get_score(roads, I.G)
    k = 0
    kmax = nb_iter

    # T0 est de l'ordre de grandeur d'une variation de score, ici de l'ordre de la dizaine
    T = 10

    while k < kmax :
        new_roads = []

        INSERTION = False

        demands = roads_demands(current_roads, I)
        # Choix du mode INSERTION / ECHANGE qui dépends des places occupées dans la route la
        # plus surchargée et dans la moins surchargée
        P = float(min(demands)) / (4 * max(demands))
        if np.random.uniform() > P:
            INSERTION = True

        ############################
        INSERTION = False
        ############################

        if INSERTION: # Cas ou on insère un client dans une route
            new_roads = roads_from_insertion(current_roads, I)

        else : # cas ou on echange deux clients entre deux routes
            new_roads = roads_from_exchange(current_roads, I)


        # Garder on non la nouvelle solution selon le recuit simulé
        current_score = get_score(current_roads, I.G)
        new_score = get_score(new_roads, I.G)
        if new_score < current_score or \
            math.exp((current_score - new_score) * kmax / (T * (kmax - k))) > np.random.uniform() :

            current_roads = new_roads

        k += 1

    return current_roads



new_roads = roads_exchange_simulated_aneling(roads, I, nb_iter = 3000)
print(get_score(new_roads, I.G))