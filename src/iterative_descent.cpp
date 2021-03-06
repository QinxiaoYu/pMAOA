#include "iterative_descent.h"

namespace maoa {
namespace idesc {

    double getTotalTourDistance(std::list<int> &t, const Graph &g) {
        double tourDistance = g.getDistance(g.depotId(), *t.begin());
        auto c_it = ++t.begin();
        while (c_it != t.end()) {
            tourDistance += g.getDistance(*std::prev(c_it), *c_it);
            c_it++;
        }
        tourDistance += g.getDistance(*--c_it, g.depotId());
        return tourDistance;
    }

    double getTotalCost(std::list<Tour> &tours, const Graph &g) {
        double totalCost = 0;
        for (Tour &t : tours) {
            totalCost += getTotalTourDistance(t.cities, g);
        }
        return totalCost;
    }

    bool improve2opt(std::list<Tour> &tours, const Graph &g) {
        bool changeMade = false;

        int firstNodeIdx, secondNodeIdx;
        long t_size;

        for (Tour &t : tours) {
            t_size = t.cities.size();
            // Compute original tour cost (before improvements).
            double currentTourCost = getTotalTourDistance(t.cities, g);
            double newTourCost;
            std::list<int>::iterator c_it;

            firstNodeIdx = -1;
            while (firstNodeIdx < t_size - 1) {

                secondNodeIdx = firstNodeIdx + 1;
                while (secondNodeIdx < t_size) {

                    // Construct new tour according to firstNode and secondNode.
                    std::list<int> new_tour;
                    // 1. Append every node before `firstNode`.
                    int constructIdx;
                    c_it = t.cities.begin();
                    for (constructIdx = 0; constructIdx <= firstNodeIdx; constructIdx++) {
                        new_tour.push_back(*c_it);
                        c_it++;
                    }
                    // 2. Add from `secondNode` to `firstNode` in reverse order
                    int advanceCounter = (firstNodeIdx == -1) ? secondNodeIdx : secondNodeIdx-firstNodeIdx-1;
                    std::advance(c_it, advanceCounter);
                    for (constructIdx = secondNodeIdx; constructIdx > firstNodeIdx; constructIdx--) {
                        new_tour.push_back(*c_it);
                        c_it--;
                    }
                    // 3. Add rest of route in correct order.
                    std::advance(c_it, secondNodeIdx-firstNodeIdx+1);
                    for (constructIdx = secondNodeIdx + 1; constructIdx < t_size; constructIdx++) {
                        new_tour.push_back(*c_it);
                        c_it++;
                    }

                    newTourCost = getTotalTourDistance(new_tour, g);
                    if (newTourCost < currentTourCost) {
                        changeMade = true;
                        currentTourCost = newTourCost;
                        t.cities.swap(new_tour);
                        firstNodeIdx = -1;
                        secondNodeIdx = 0;
                    } else {
                        secondNodeIdx++;
                    }
                }
                firstNodeIdx++;
            }
        }
        return changeMade;
    }

    bool improveByRelocate(std::list<Tour> &tours, const Graph &g) {
        bool changeMade = false;
        std::list<Tour>::iterator t_it1, t_it2;

        begin:
        // Check all pairs of tours.
        for (t_it1 = tours.begin(); t_it1 != --tours.end(); t_it1++) {
            for (t_it2 = std::next(t_it1); t_it2 != tours.end(); t_it2++) {

                // Check if a node of tour1 can be relocated inside tour2.
                auto c_it1 = t_it1->cities.begin();
                while (c_it1 != t_it1->cities.end()) {
                    int currentT1 = *c_it1;
                    float demandT1 = g.getDemand(currentT1);
                    // Check capacity of tour2.
                    if (t_it2->capacity + demandT1 > g.capacity()) {
                        c_it1++;
                        continue;
                    }

                    // If c_it1 is first node in tour, then previous node is depot.
                    int prevT1 = (c_it1 == t_it1->cities.begin()) ? g.depotId() : *std::prev(c_it1);
                    // If c_it1 is last node in tour, then next node is depot.
                    int nextT1 = (c_it1 == --t_it1->cities.end()) ? g.depotId() : *std::next(c_it1);
                    double gain_fromRemove = g.getDistance(prevT1, currentT1) + g.getDistance(currentT1, nextT1)
                                             - g.getDistance(prevT1, nextT1);

                    // Search for the best place to put currentT1 in tour2.
                    auto c_it2 = t_it2->cities.begin();
                    while (c_it2 != t_it2->cities.end()) {
                        int prevT2 = (c_it2 == t_it2->cities.begin()) ? g.depotId() : *std::prev(c_it2);
                        int currentT2 = *c_it2;

                        double loss_fromAdd = g.getDistance(prevT2, currentT2) - g.getDistance(prevT2, currentT1)
                                              - g.getDistance(currentT1, currentT2);

                        double totalGain = gain_fromRemove + loss_fromAdd;
                        if (totalGain > 0) {
                            // Implement changes.
                            t_it2->cities.insert(c_it2, currentT1);
                            t_it2->capacity += demandT1;
                            t_it1->cities.erase(c_it1);
                            t_it1->capacity -= demandT1;
                            changeMade = true;
                            // Reset search.
                            goto begin;
                        }

                        c_it2++;
                    }
                    c_it1++;
                }
            }
        }
        return changeMade;
    }

    bool improveByExchange(std::list<Tour> &tours, const Graph &g) {
        bool changeMade = false;
        std::list<Tour>::iterator t_it1, t_it2;

        begin:
        // Check all pairs of tours.
        for (t_it1 = tours.begin(); t_it1 != --tours.end(); t_it1++) {
            for (t_it2 = std::next(t_it1); t_it2 != tours.end(); t_it2++) {

                // Check all pairs of cities within the tours.
                auto c_it1 = t_it1->cities.begin();
                while (c_it1 != t_it1->cities.end()) {
                    int currentT1 = *c_it1;
                    float demandT1 = g.getDemand(currentT1);

                    // If c_it1 is first node in tour, then previous node is depot.
                    int prevT1 = (c_it1 == t_it1->cities.begin()) ? g.depotId() : *std::prev(c_it1);
                    // If c_it1 is last node in tour, then next node is depot.
                    int nextT1 = (c_it1 == --t_it1->cities.end()) ? g.depotId() : *std::next(c_it1);

                    double distanceT1 = g.getDistance(prevT1, currentT1) + g.getDistance(currentT1, nextT1);

                    auto c_it2 = t_it2->cities.begin();
                    while (c_it2 != t_it2->cities.end()) {
                        int currentT2 = *c_it2;
                        float demandT2 = g.getDemand(currentT2);

                        // Check that c1 can be exchanged with c2.
                        if (t_it2->capacity -demandT2 +demandT1 > g.capacity()
                                || t_it1->capacity -demandT1 +demandT2 > g.capacity()) {
                            c_it2++;
                            continue;
                        }

                        // If c_it1 is first node in tour, then previous node is depot.
                        int prevT2 = (c_it2 == t_it2->cities.begin()) ? g.depotId() : *std::prev(c_it2);
                        // If c_it1 is last node in tour, then next node is depot.
                        int nextT2 = (c_it2 == --t_it2->cities.end()) ? g.depotId() : *std::next(c_it2);

                        // Compute distance of current tour
                        double currentDistance = distanceT1 + g.getDistance(prevT2, currentT2)
                                                 + g.getDistance(currentT2, nextT2);

                        double distanceIfExchange = g.getDistance(prevT1, currentT2)
                                                    + g.getDistance(currentT2, nextT1)
                                                    + g.getDistance(prevT2, currentT1)
                                                    + g.getDistance(currentT1, nextT2);

                        if (distanceIfExchange < currentDistance) {
                            // Implement changes
                            t_it1->cities.insert(c_it1, currentT2);
                            t_it2->cities.insert(c_it2, currentT1);
                            t_it1->cities.erase(c_it1);
                            t_it2->cities.erase(c_it2);
                            t_it1->capacity += demandT2 -demandT1;
                            t_it2->capacity += demandT1 -demandT2;
                            changeMade = true;
                            // Reset search.
                            goto begin;
                        }

                        c_it2++;
                    }

                    c_it1++;
                }
            }
        }
        return changeMade;
    }

    void descent(std::list<Tour> &tours, const Graph &g) {
        bool changeMade;
        do {
            changeMade = false;
            changeMade = changeMade || improve2opt(tours, g);
            changeMade = changeMade || improveByRelocate(tours, g);
            changeMade = changeMade || improveByExchange(tours, g);
            double totalCost = getTotalCost(tours, g);
            std::cout << "Total cost is: " << totalCost << std::endl;
        } while (changeMade);
    }
} // namespace idesc
} // namespace maoa

