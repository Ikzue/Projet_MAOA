#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Nov 29 14:30:27 2019

@author: 3528315
"""
import networkx as nx
import math
class Instance:
    """
    name : nom
    G : graph 
    Q : capacite
    n : nombre sommets
    m_opt : nombre camions (tournees)
    
    """
        
    def __init__(self,file):
        with open(file) as f:
            #Initialisation parametes graphe
            lines = f.readlines()
            self.name = lines[0].split(' ')[2]
            self.m_opt = int((lines[1].split(',')[1]).split(' ')[-1])
            self.n = int(lines[3].split(' ')[2])
            self.Q = int(lines[5].split(' ')[2])
            #Initialisation noeuds et coordonnees
            G = nx.Graph()
            i = 7
            nodes = []
            while(lines[i].strip() != "DEMAND_SECTION"):
                node = [int(k) for k in lines[i].strip().split(' ')]
                nodes.append(node)
                i += 1
            #Initialisation aretes
            for node in nodes:
                for node2 in nodes:
                    if node != node2:
                        weight = (node[1] - node2[1])**2 + (node[2] - node2[2])**2
                        weight = math.sqrt(weight)
                        G.add_edge(node[0], node2[0], weight = weight)
            i = i + 1
            #Initialisation demandes
            while(lines[i].strip() != "DEPOT_SECTION"):
                demand = [int(k) for k in lines[i].strip().split(' ')]
                G.nodes[demand[0]]["demand"] = demand[1]
                i = i+1
            self.G = G

    def get_demand(self,key):
        return self.G.nodes[key]['demand']



                    
                
            
Instance("Instances/A\A-n32-k5.vrp")