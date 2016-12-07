/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_PKTABLE_H_
#define FSM_FSM_PKTABLE_H_

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "fsm/typedef.inc"

#include "fsm/PkTableRow.h"
#include "interface/FsmPresentationLayer.h"
class Dfsm;

/**
 Class for representing Pk-Tables
 */
class PkTable
{
private:
    /**
     * One table row for each FSM state
     */
    std::vector<std::shared_ptr<PkTableRow> > rows;
    
    /**
     * Map each state s to its associated Pk-equivalence class s2c[s]
     */
    S2CMap s2c;
    
    /**
     * Maximal value of the input alphabet
     */
    int maxInput;
    
    /**
     * The presentation layer used by the pktable
     */
    std::shared_ptr<FsmPresentationLayer> presentationLayer;
public:
    /**
     * Create an empty Pktable, with each row set to nullptr
     * \param numStates The number of rows (1 row = 1 state)
     * \param maxInput The maximal input
     * \param presentationLayer The presentation layer used by the PkTable
     */
    PkTable(const int numStates, const int maxInput, const std::shared_ptr<FsmPresentationLayer> presentationLayer);
    
    /**
     * Create a Pktable
     * \param numStates The number of rows (1 row = 1 state)
     * \param maxInput The maximal input
     * \param rows The rows used by the PkTable
     * \param presentationLayer The presentation layer used by the PkTable
     */
    PkTable(const int numStates, const int maxInput, const std::vector<std::shared_ptr<PkTableRow>> rows, const std::shared_ptr<FsmPresentationLayer> presentationLayer);
    
    /**
     * Set the row of the Pk-equivalence class at the position s
     * \param s The position
     * \param row The row
     */
    void setRow(const int s, const std::shared_ptr<PkTableRow> row);
    
    /**
     * Set the id of the Pk-equivalence class at the position n
     * \param n The position
     * \param c The id
     */
    void setClass(const int n, const int c);
    
    /**
     * Get the id of the Pk-equivalence class at the position n
     * \param n The position
     * \return The id
     */
    int getClass(const int n) const;
    
    /**
     * Get the maximum id of the Pk-equivalence class
     * \return The id
     */
    int maxClassId() const;
    
    /**
     * Generate the P(k+1) table from this Pk-Table
     * \return null if no new equivalence classes can be generated from this Pk-Table
     * the P(k+1)-Table otherwise
     */
    std::shared_ptr<PkTable> getPkPlusOneTable() const;
    
    /**
     * Generate a DFSM form thus pktable
     * \return The DFSM generated
     */
    Dfsm toFsm(std::string name);
    
    /**
     * Return members of an equivalence class c as set string
     */
    std::string getMembers(const int c) const;
    
    /**
     * Output the DFSMTable to a standard output stream in LaTeX
     *  tabular format.
     * \param out The standard output stream to use
     * \param dfsmTable The DFSMTable to print
     * \return The standard output stream used, to allow user to cascade <<
     */
    friend std::ostream & operator<<(std::ostream & out, const PkTable & pkTable);
    
    
};
#endif //FSM_FSM_PKTABLE_H_



















