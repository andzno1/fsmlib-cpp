/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_PKTABLEROW_H_
#define FSM_FSM_PKTABLEROW_H_

#include <iostream>

#include "fsm/typedef.inc"

class PkTableRow
{
private:
    // Pointer to I2O map (originating from the DFSM table)
    IOMap& io;

	// Pointer to I2P map
	I2PMap& i2p;
public:

	//TODO
	PkTableRow(IOMap& io, I2PMap& i2p);

	//TODO
	IOMap& getIOMap();

	//TODO
	I2PMap& getI2PMap();

	//TODO
	int get(const int x) const;

	/**
	Check wheter or not, the 2 trace are the same
	\param trace1 The first trace
	\param trace2 The second trace
	\return True if they are the same, false otherwise
	*/
	friend bool operator==(PkTableRow const & row, S2CMap const & s2c);

	/**
	Output the PkTableRow to a standard output stream
	\param out The standard output stream to use
	\param pkTableRow The PkTableRow to print
	\return The standard output stream used, to allow user to cascade <<
	*/
	friend std::ostream & operator<<(std::ostream & out, const PkTableRow & pkTableRow);
};
#endif //FSM_FSM_PKTABLEROW_H_
