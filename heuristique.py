#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Nov 29 14:16:07 2019

@author: 3528315
"""

import cvrp
from operator import itemgetter, attrgetter

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
Deuxième étape : échanges entre routes
Tant que non convergence :
    On prends la route la plus lourde :
        On regarde un client au hasard de cette route :
            Si il peut aller dans la route la plus légère :
                Il se met à chaque place possible de la route :
                    Pour chaque place on calcule le poids total de toutes les routes
                Il se place à la place qui a minimisé le poids total
"""


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

# test_road = [1, 6, 11, 9, 25, 26, 19, 28, 21, 1]
print(local_ameliorations(roads, I))