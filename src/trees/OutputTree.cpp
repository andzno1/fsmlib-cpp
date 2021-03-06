/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#include "trees/OutputTree.h"

using namespace std;

void OutputTree::printChildrenOutput(ostream& out, const shared_ptr<TreeNode>& top, const shared_ptr<int>& idNode, const int idInput) const
{
	int idNodeBase = *idNode;
	for (shared_ptr<TreeEdge> edge : *top->getChildren())
	{
		out << idNodeBase << " -> " << ++ *idNode << "[label = \"" << inputTrace.get().at(idInput) << "/" << edge->getIO() << "\" ];" << endl;
		printChildrenOutput(out, edge->getTarget(), idNode, idInput + 1);
	}
}

OutputTree::OutputTree(const shared_ptr<TreeNode>& root,
                       const InputTrace& inputTrace,
                       const shared_ptr<FsmPresentationLayer>& presentationLayer)
	: Tree(root, presentationLayer), inputTrace(inputTrace)
{

}

OutputTree::OutputTree(const OutputTree* other):
    Tree(other), inputTrace(other->inputTrace.get(), other->presentationLayer)
{

}

InputTrace OutputTree::getInputTrace() const
{
    return inputTrace;
}

bool OutputTree::contains(const OutputTree& ot) const
{
    //Get the input traces
    InputTrace myInputs = getInputTrace();
    InputTrace otherInputs = ot.getInputTrace();
    //Get all output traces
    vector<OutputTrace> myOutputs = getOutputTraces();
    vector<OutputTrace> otherOutputs = ot.getOutputTraces();
    //If the input traces are not equal or this tree has less output traces
    //than the other, this tree cannot contain the other.
    if((not (myInputs == otherInputs))) {
        return false;
    }
    //Sort both sequences of output traces to obtain sorted sets of output
    //traces ( O(nlog(n)) )
    sort(myOutputs.begin(), myOutputs.end());
    sort(otherOutputs.begin(), otherOutputs.end());
    vector<OutputTrace>::const_iterator myLast = unique(myOutputs.begin(), myOutputs.end());
    vector<OutputTrace>::const_iterator otherLast = unique(otherOutputs.begin(), otherOutputs.end());
    //Return whether the set of output traces of this tree contains the set of
    //output traces of the other ( O(n) )
    return includes(myOutputs.cbegin(), myLast, otherOutputs.cbegin(), otherLast);
}

vector<OutputTrace> OutputTree::getOutputTraces() const {
    //Get all traces
    auto lli = getIOLists().getIOLists();
    vector<OutputTrace> traces;
    //Reserve enough space in the result vector
    traces.reserve(lli->size());
    //Move each trace into the result vector and construct an OutputTrace for
    //each
    for(auto &trace : *lli) {
        traces.emplace_back(std::move(trace), presentationLayer);
    }
    return traces;
}

vector<IOTrace> OutputTree::getOutputsIntersection(OutputTree & ot)
{
    vector<IOTrace> thisIOTrace;
    vector<IOTrace> otherIOTrace;
    toIOTrace(thisIOTrace);
    ot.toIOTrace(otherIOTrace);

    vector<IOTrace> intersection;

    bool skip = false;
    for (IOTrace thisTrace : thisIOTrace)
    {
        if (skip)
        {
            skip = false;
            continue;
        }
        for (IOTrace otherTrace : otherIOTrace)
        {
            if (thisTrace == otherTrace)
            {
                intersection.push_back(thisTrace);
                skip = true;
                continue;
            }
        }
    }
    return intersection;
}

void OutputTree::toDot(ostream& out) const
{
	out << "digraph OutputTree {" << endl;
	out << "\trankdir=TB;" << endl;//Top -> Bottom, to create a vertical graph
	out << "\tnode [shape = circle];" << endl;
	shared_ptr<int> id = make_shared<int>(0);
	printChildrenOutput(out, root, id, 0);
	out << "}";
}

void OutputTree::store(ofstream& file)
{
	vector<vector<int>> lli = *getIOLists().getIOLists();
	for (vector<int> lst : lli)
	{
		for (unsigned int i = 0; i < lst.size(); ++ i)
		{
			if (i != 0)
			{
				file << ".";
			}

			file << "(" << inputTrace.get().at(i) << "," << lst.at(i) <<")";
		}
	}
}

void OutputTree::toIOTrace(vector<IOTrace> &iotrVec) {
    
    
    vector<vector<int>> lli = *getIOLists().getIOLists();
    for (vector<int> lst : lli)
    {
        
        OutputTrace otrc(lst,presentationLayer);
        IOTrace iotrc(inputTrace,otrc);
        iotrVec.push_back(iotrc);
        
    }
    
}

void OutputTree::toIOTrace(vector<shared_ptr<IOTrace>> &iotrVec) {
    vector<vector<int>> lli = *getIOLists().getIOLists();
    for (vector<int> lst : lli)
    {
        OutputTrace otrc(lst,presentationLayer);
        shared_ptr<IOTrace> iotrc = make_shared<IOTrace>(inputTrace, otrc);
        iotrVec.push_back(iotrc);
    }
}

OutputTree* OutputTree::_clone() const
{
    return new OutputTree( this );
}

std::shared_ptr<OutputTree> OutputTree::Clone() const
{
    return std::shared_ptr<OutputTree>(_clone());
}

ostream& operator<<(ostream& out, OutputTree& ot)
{
	vector<vector<int>> lli = *ot.getIOLists().getIOLists();
	for (vector<int> lst : lli)
	{
		for (unsigned int i = 0; i < lst.size(); ++ i)
		{
            
            if ( i > 0 ) out << ".";

			out << "(" << ot.presentationLayer->getInId(ot.inputTrace.get().at(i)) << "/" << ot.presentationLayer->getOutId(lst.at(i)) << ")";
		}
		out << endl;
	}
	return out;
}

bool operator==(OutputTree& outputTree1, OutputTree& outputTree2)
{
    
    return ( outputTree1.contains(outputTree2) and outputTree2.contains(outputTree1) );
    
#if 0
    // This does not work for output trees generated by non-observable FSMs
    
	/*
     * If the associated input traces differ,
	 * the output trees can never be equal
     */
	if (!(outputTree1.inputTrace == outputTree2.inputTrace))
	{
		return false;
	}
    
    /* Check whether all output traces of outputTree1 are contained in outputTree2 */
    
    

	/*Since outputTree1 and outputTree2 are two output trees over the same
	input trace, the trees have the same height. We only have
	to check whether each corresponding node has the same number
	of children, and whether corresponding edges carry the same labels
	(output values).*/
	return *outputTree1.getRoot() == *outputTree2.getRoot();
#endif
}

bool operator!=(OutputTree& outputTree1, OutputTree& outputTree2)
{
    return not (outputTree1 == outputTree2);
}

