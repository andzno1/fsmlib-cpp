/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */
#include <deque>

#include "fsm/FsmNode.h"
#include "fsm/FsmTransition.h"
#include "fsm/InputTrace.h"
#include "fsm/OutputTrace.h"
#include "fsm/OFSMTable.h"
#include "fsm/DFSMTableRow.h"
#include "fsm/PkTable.h"
#include "fsm/RDistinguishability.h"
#include "trees/TreeEdge.h"
#include "trees/TreeNode.h"
#include "trees/OutputTree.h"
#include "trees/Tree.h"
#include "trees/TreeNode.h"
#include "trees/IOListContainer.h"
#include "interface/FsmPresentationLayer.h"
#include "logging/easylogging++.h"

using namespace std;

FsmNode::FsmNode(const int id, const shared_ptr<FsmPresentationLayer>& presentationLayer)
: id(id),
visited(false),
color(white),
presentationLayer(presentationLayer),
derivedFromPair(nullptr),
isInitialNode(false),
dReachTrace(nullptr)
{
    rDistinguishability = make_shared<RDistinguishability>(presentationLayer);
}

FsmNode::FsmNode(const int id, const string & name,
                 const shared_ptr<FsmPresentationLayer>& presentationLayer)
: FsmNode(id, presentationLayer)
{
    this->name = name;
}

void FsmNode::addTransition(std::shared_ptr<FsmTransition> transition)
{
    
    // Do not accept another transition with the same label and the
    // the same target node
    for ( auto tr : transitions ) {
        if ( tr->getTarget() == transition->getTarget()
            and
            tr->getLabel() == transition->getLabel() ) {
            return;
        }
    }
    
    transitions.push_back(transition);
}

bool FsmNode::removeTransition(const std::shared_ptr<FsmTransition>& t)
{
    auto it = find(transitions.begin(), transitions.end(), t);
    if (it != transitions.end())
    {
        transitions.erase(it);
        return true;
    }
    return false;
}

void FsmNode::setTransitions(std::vector<std::shared_ptr<FsmTransition>> transitions)
{
    this->transitions = transitions;
}

vector<shared_ptr<FsmTransition> >& FsmNode::getTransitions()
{
    return transitions;
}

vector<shared_ptr<FsmTransition>> FsmNode::getDeterminisitcTransitions() const
{
    vector<shared_ptr<FsmTransition>> result;
    unordered_map<int, int> inputOccurences;
    for (const shared_ptr<FsmTransition>& t : transitions)
    {
        inputOccurences[t->getLabel()->getInput()]++;
    }

    for (const shared_ptr<FsmTransition>& t : transitions)
    {
        if (inputOccurences.at(t->getLabel()->getInput()) == 1)
        {
            result.push_back(t);
        }
    }
    return result;
}

vector<shared_ptr<FsmTransition>> FsmNode::getNonDeterminisitcTransitions() const
{
    vector<shared_ptr<FsmTransition>> result;
    unordered_map<int, int> inputOccurences;
    for (const shared_ptr<FsmTransition>& t : transitions)
    {
        inputOccurences[t->getLabel()->getInput()]++;
    }

    for (const shared_ptr<FsmTransition>& t : transitions)
    {
        if (inputOccurences.at(t->getLabel()->getInput()) > 1)
        {
            result.push_back(t);
        }
    }
    return result;
}

int FsmNode::getId() const
{
    return id;
}

string FsmNode::getName() const
{
    return presentationLayer->getStateId(id, name);
}

bool FsmNode::hasBeenVisited() const
{
    return visited;
}

void FsmNode::setVisited()
{
    visited = true;
}

void FsmNode::setUnvisited() {
    visited = false;
}

void FsmNode::setDReachable(shared_ptr<IOTrace> trace)
{
    dReachTrace = trace;
    dReachable = true;
}

void FsmNode::setReachTrace(shared_ptr<IOTrace> trace)
{
    reachTrace = trace;
}

void FsmNode::setNotDReachable() {
    dReachable = false;
}

void FsmNode::setPair(const shared_ptr<FsmNode>& l, const shared_ptr<FsmNode>& r)
{
    derivedFromPair = make_shared<pair<shared_ptr<FsmNode>, shared_ptr<FsmNode>>>(l, r);
}

void FsmNode::setPair(const shared_ptr<pair<shared_ptr<FsmNode>, shared_ptr<FsmNode>>>& p)
{
    derivedFromPair = p;
}

bool FsmNode::isDerivedFrom(const shared_ptr<pair<shared_ptr<FsmNode>, shared_ptr<FsmNode>>>& p) const
{
    return derivedFromPair != nullptr && *derivedFromPair == *p;
}

bool FsmNode::isDReachable() const
{
    return dReachable;
}

std::shared_ptr<IOTrace> FsmNode::getDReachTrace() const
{
    return dReachTrace;
}

std::shared_ptr<IOTrace> FsmNode::getReachTrace() const
{
    return reachTrace;
}

shared_ptr<pair<shared_ptr<FsmNode>, shared_ptr<FsmNode>>> FsmNode::getPair() const
{
    return derivedFromPair;
}

shared_ptr<RDistinguishability> FsmNode::getRDistinguishability()
{
    return rDistinguishability;
}

vector<shared_ptr<FsmNode>> FsmNode::getPossibleOutputs(const int x, vector<shared_ptr<OutputTrace>> & outputs) const
{
    outputs = vector<shared_ptr<OutputTrace>>();
    vector<shared_ptr<FsmNode>> result;

    if (x == FsmLabel::EPSILON)
    {
        vector<int> traceRaw({FsmLabel::EPSILON});
        shared_ptr<OutputTrace> oT = make_shared<OutputTrace>(traceRaw, presentationLayer);
        outputs.push_back(oT);
        result.push_back(static_pointer_cast<FsmNode>(const_pointer_cast<FsmNode>(shared_from_this())));
        return result;
    }

    for (auto transition : transitions)
    {
        if (transition->getLabel()->getInput() == x)
        {
            result.push_back(transition->getTarget());
            vector<int> traceRaw({transition->getLabel()->getOutput()});
            shared_ptr<OutputTrace> oT = make_shared<OutputTrace>(traceRaw, presentationLayer);
            outputs.push_back(oT);
        }
    }
    return result;
}

void FsmNode::getPossibleOutputs(const InputTrace& inputTrace,
                                 vector<shared_ptr<OutputTrace>>& producedOutputTraces,
                                 vector<shared_ptr<FsmNode>>& reachedNodes) const
{
    vector<int> rawInputTrace = inputTrace.get();
    if (rawInputTrace.size() == 0)
    {
        reachedNodes.push_back(const_pointer_cast<FsmNode>(shared_from_this()));
        return;
    }

    int input = rawInputTrace.at(0);
    vector<shared_ptr<OutputTrace>> nextOutputs;
    vector<shared_ptr<FsmNode>> nextTargets = getPossibleOutputs(input, nextOutputs);

    if (nextOutputs.size() != nextTargets.size())
    {
        LOG(FATAL) << "Number of produced outputs and targets does not match.";
    }

    vector<shared_ptr<OutputTrace>> newlyProducedOutputTraces;
    for (size_t i = 0; i < nextOutputs.size(); ++i)
    {
        shared_ptr<OutputTrace> nextOutput = nextOutputs.at(i);
        shared_ptr<FsmNode> nextTarget = nextTargets.at(i);

        vector<shared_ptr<OutputTrace>> nextOutputCopy;
        nextOutputCopy.push_back(nextOutput);

        nextTarget->getPossibleOutputs(InputTrace(inputTrace, 1), nextOutputCopy, reachedNodes);
        if (producedOutputTraces.size() > 0)
        {
            vector<shared_ptr<OutputTrace>> producedOutputTracesCopy = vector<shared_ptr<OutputTrace>>(producedOutputTraces);
            for (shared_ptr<OutputTrace> oldTrace : producedOutputTracesCopy)
            {
                for (shared_ptr<OutputTrace> nOTrace : nextOutputCopy)
                {
                    shared_ptr<OutputTrace> oldTraceCopy = make_shared<OutputTrace>(*oldTrace);
                    oldTraceCopy->append(*nOTrace);
                    newlyProducedOutputTraces.push_back(oldTraceCopy);
                }
            }
        }
        else
        {
            for (shared_ptr<OutputTrace> nOTrace : nextOutputCopy)
            {
                newlyProducedOutputTraces.push_back(nOTrace);
            }
        }
    }
    producedOutputTraces = newlyProducedOutputTraces;

    VLOG(3) << "getPossibleOutputs(): " << getName() << ", " << inputTrace << ", " << producedOutputTraces.size() << ", " << reachedNodes.size();
    stringstream ss;
    ss << "  reached nodes: ";
    for (auto n : reachedNodes)
    {
        ss << n->getName() << ", ";
    }
    VLOG(3) << ss.str();
    ss.str(std::string());
    ss << "  outputs: ";
    for (auto n : producedOutputTraces)
    {
        ss << *n << ", ";
    }
    VLOG(3) << ss.str();
}

void FsmNode::getPossibleOutputs(const InputTrace& input, vector<shared_ptr<OutputTrace>>& producedOutputs) const
{
    vector<shared_ptr<FsmNode>> rN;
    getPossibleOutputs(input, producedOutputs, rN);
}

vector<shared_ptr<OutputTrace>> FsmNode::getPossibleOutputs(const int x) const
{
    vector<shared_ptr<OutputTrace>> result;
    for (auto transition : transitions)
    {
        if (transition->getLabel()->getInput() == x)
        {
            vector<int> traceRaw({transition->getLabel()->getOutput()});
            shared_ptr<OutputTrace> oT = make_shared<OutputTrace>(traceRaw, presentationLayer);
            result.push_back(oT);
        }
    }
    return result;
}

bool FsmNode::hasTransition(const int input, const int output) const
{
    for (shared_ptr<FsmTransition> trans : transitions)
    {
        if (trans->getLabel()->getInput() == input && trans->getLabel()->getOutput() == output)
        {
            return true;
        }
    }
    return false;
}

bool FsmNode::hasTransition(const int input) const
{
    for (shared_ptr<FsmTransition> trans : transitions)
    {
        if (trans->getLabel()->getInput() == input)
        {
            return true;
        }
    }
    return false;
}

vector<int> FsmNode::getNotDefinedInputs(const int& maxInput) const
{
    VLOG(2) << "getNotDefinedInputs()";
    vector<int> result;
    for (int i = 0; i <= maxInput; ++i)
    {
        bool inputDefined = false;
        for (const shared_ptr<FsmTransition>& t : transitions)
        {
            if (t->getLabel()->getInput() == i)
            {
                inputDefined = true;
                break;
            }
        }
        if (!inputDefined){
            VLOG(2) << "  " << presentationLayer->getInId(static_cast<unsigned int>(i));
            result.push_back(i);
        }
    }
    return result;
}

vector<int> FsmNode::getNotDefinedOutputs(const int& input, const int& maxOutput) const
{
    VLOG(2) << "getNotDefinedOutputs() for input " << presentationLayer->getInId(static_cast<unsigned int>(input));
    vector<int> result;
    for (int o = 0; o <= maxOutput; ++o)
    {
        bool outputDefined = false;
        for (const shared_ptr<FsmTransition>& t : transitions)
        {
            if (t->getLabel()->getInput() == input && t->getLabel()->getOutput() == o)
            {
                outputDefined = true;
                break;
            }
        }
        if (!outputDefined)
        {
            VLOG(2) << "  " << presentationLayer->getOutId(static_cast<unsigned int>(o));
            result.push_back(o);
        }
    }
    return result;
}

bool FsmNode::isPossibleOutput(const int x, const int y) const
{
    for (auto transition : transitions)
    {
        if (transition->getLabel()->getInput() == x && transition->getLabel()->getOutput() == y)
        {
            return true;
        }
    }
    return false;
}

bool FsmNode::isPossibleInput(const int x) const
{
    for (auto transition : transitions)
    {
        if (transition->getLabel()->getInput() == x)
        {
            return true;
        }
    }
    return false;
}

shared_ptr<FsmNode> FsmNode::apply(const int e, OutputTrace & o)
{
    for (shared_ptr<FsmTransition> tr : transitions)
    {
        if (tr->getLabel()->getInput() == e)
        {
            o.add(tr->getLabel()->getOutput());
            return tr->getTarget();
        }
    }
    return nullptr;
}

OutputTree FsmNode::apply(const InputTrace& itrc, bool markAsVisited)
{
    deque<shared_ptr<TreeNode>> tnl;
    unordered_map<shared_ptr<TreeNode>, shared_ptr<FsmNode>> t2f;
    
    shared_ptr<TreeNode> root = make_shared<TreeNode>();
    OutputTree ot = OutputTree(root, itrc, presentationLayer);
    
    if (itrc.get().size() == 0)
    {
        return ot;
    }
    
    t2f[root] = shared_from_this();
    
    for (auto it = itrc.cbegin(); it != itrc.cend(); ++ it)
    {
        int x = *it;
        
        vector< shared_ptr<TreeNode> > vaux = ot.getLeaves();
        
        for ( auto n : vaux ) {
            tnl.push_back(n);
        }
        
        while (!tnl.empty())
        {
            shared_ptr<TreeNode> thisTreeNode = tnl.front();
            tnl.pop_front();
            
            shared_ptr<FsmNode> thisState = t2f.at(thisTreeNode);
            if ( markAsVisited ) thisState->setVisited();
            
            for (shared_ptr<FsmTransition> tr : thisState->getTransitions())
            {
                if (tr->getLabel()->getInput() == x)
                {
                    int y = tr->getLabel()->getOutput();
                    shared_ptr<FsmNode> tgtState = tr->getTarget();
                    shared_ptr<TreeNode> tgtNode = make_shared<TreeNode>();
                    shared_ptr<TreeEdge> te = make_shared<TreeEdge>(y, tgtNode);
                    thisTreeNode->add(te);
                    t2f[tgtNode] = tgtState;
                    if ( markAsVisited ) tgtState->setVisited();
                }
            }
        }
    }
    return ot;
}

unordered_set<shared_ptr<FsmNode>> FsmNode::after(const vector<int>& itrc)
{
    unordered_set<shared_ptr<FsmNode>> nodeSet;
    nodeSet.insert(shared_from_this());

    for (auto it = itrc.begin(); it != itrc.end(); ++it)
    {
        int x = *it;
        unordered_set<shared_ptr<FsmNode>> newNodeSet;

        for (shared_ptr<FsmNode> n : nodeSet)
        {
            unordered_set<shared_ptr<FsmNode>> ns = n->afterAsSet(x);
            newNodeSet.insert(ns.begin(), ns.end());
        }
    nodeSet = newNodeSet;
    }
    return nodeSet;
}

unordered_set<shared_ptr<FsmNode>> FsmNode::after(const InputTrace & itrc, const OutputTrace & otrc)
{
    vector<int> itrcRaw = itrc.get();
    vector<int> otrcRaw = otrc.get();
    unordered_set<shared_ptr<FsmNode>> nodeSet;

    if (itrcRaw.size() != otrcRaw.size())
    {
        return nodeSet;
    }

    nodeSet.insert(shared_from_this());

    for (size_t i = 0; i < itrcRaw.size(); ++i)
    {
        int x = itrcRaw.at(i);
        int y = otrcRaw.at(i);
        unordered_set<shared_ptr<FsmNode>> newNodeSet;

        for (shared_ptr<FsmNode> n : nodeSet)
        {
            unordered_set<shared_ptr<FsmNode>> ns = n->afterAsSet(x, y);
            newNodeSet.insert(ns.begin(), ns.end());
        }
        nodeSet = newNodeSet;
    }
    return nodeSet;
}

unordered_set<shared_ptr<FsmNode>> FsmNode::after(const InputTrace& itrc)
{
    unordered_set<shared_ptr<FsmNode>> nodeSet;
    nodeSet.insert(shared_from_this());

    for (auto it = itrc.cbegin(); it != itrc.cend(); ++it)
    {
        int x = *it;
        unordered_set<shared_ptr<FsmNode>> newNodeSet;

        for (shared_ptr<FsmNode> n : nodeSet)
        {
            unordered_set<shared_ptr<FsmNode>> ns = n->afterAsSet(x);
            newNodeSet.insert(ns.begin(), ns.end());
        }
        nodeSet = newNodeSet;
    }
    return nodeSet;
}

std::unordered_set<std::shared_ptr<FsmNode>> FsmNode::after(const IOTrace& trace)
{
    return after(trace.getInputTrace(), trace.getOutputTrace());
}

std::unordered_set<std::shared_ptr<FsmNode>> FsmNode::after(const std::shared_ptr<TraceSegment> seg) {
    
    unordered_set<shared_ptr<FsmNode>> nodeSet;
    nodeSet.insert(shared_from_this());
    
    size_t len = 0;
    for (auto it = seg->get()->begin();
         it != seg->get()->end() and len++ < seg->getPrefix();
         ++it)
    {
        int x = *it;
        unordered_set<shared_ptr<FsmNode>> newNodeSet;
        
        for (shared_ptr<FsmNode> n : nodeSet)
        {
            unordered_set<shared_ptr<FsmNode>> ns = n->afterAsSet(x);
            newNodeSet.insert(ns.begin(), ns.end());
        }
        nodeSet = newNodeSet;
    }
    return nodeSet;
    
    
}

vector<shared_ptr<FsmNode>> FsmNode::after(const int x)
{
    vector<shared_ptr<FsmNode> > lst;

    if (x == FsmLabel::EPSILON)
    {
        lst.push_back(shared_from_this());
        return lst;
    }

    for (auto tr : transitions)
    {
        if (tr->getLabel()->getInput() == x)
        {
            lst.push_back(tr->getTarget());
        }
    }
    return lst;
}

vector<shared_ptr<FsmNode>> FsmNode::after(const int x, std::vector<int>& producedOutputs)
{
    vector<shared_ptr<FsmNode> > lst;
    for (auto tr : transitions)
    {
        if (tr->getLabel()->getInput() == x)
        {
            lst.push_back(tr->getTarget());
            producedOutputs.push_back(tr->getLabel()->getOutput());
        }
    }
    return lst;
}

unordered_set<shared_ptr<FsmNode>> FsmNode::afterAsSet(const int x)
{
    unordered_set<shared_ptr<FsmNode>> nodeSet;

    if (x == FsmLabel::EPSILON)
    {
        nodeSet.insert(shared_from_this());
        return nodeSet;
    }
    
    for (auto tr : transitions)
    {
        if (tr->getLabel()->getInput() == x)
        {
            nodeSet.insert(tr->getTarget());
        }
    }
    return nodeSet;
}

unordered_set<shared_ptr<FsmNode>> FsmNode::afterAsSet(const int x, const int y)
{
    unordered_set<shared_ptr<FsmNode>> lst;

    if (x == FsmLabel::EPSILON && y == FsmLabel::EPSILON)
    {
        lst.insert(shared_from_this());
        return lst;
    }

    for (auto tr : transitions)
    {
        if (tr->getLabel()->getInput() == x && tr->getLabel()->getOutput() == y)
        {
            lst.insert(tr->getTarget());
        }
    }
    return lst;
}

void FsmNode::setColor(const int pcolor)
{
    color = pcolor;
}

int FsmNode::getColor()
{
    return color;
}

shared_ptr<DFSMTableRow> FsmNode::getDFSMTableRow(const int maxInput)
{
    shared_ptr<DFSMTableRow> r = make_shared<DFSMTableRow>(id, maxInput);
    
    IOMap& io = r->getioSection();
    I2PMap& i2p = r->geti2postSection();
    
    for (auto tr : transitions)
    {
        int x = tr->getLabel()->getInput();
        
        /*Check whether transitions from this state are nondeterministic.
         This is detected when detecting a second transition triggered
         by the same input. In this case we cannot calculate a  DFSMTableRow.*/
        if (io.at(x) >= 0)
        {
            LOG(ERROR) << "Cannot calculated DFSM table for nondeterministic FSM.";
            return nullptr;
        }
        
        io[x] = tr->getLabel()->getOutput();
        i2p[x] = tr->getTarget()->getId();
    }
    return r;
}

bool FsmNode::distinguished(const shared_ptr<FsmNode>& otherNode, const vector<int>& iLst)
{
    InputTrace itr = InputTrace(iLst, presentationLayer);
    OutputTree ot1 = apply(itr);
    OutputTree ot2 = otherNode->apply(itr);
    
    return !(ot1 == ot2);
}

shared_ptr<InputTrace> FsmNode::distinguished(const shared_ptr<FsmNode>& otherNode, shared_ptr<Tree> w)
{
    IOListContainer iolc = w->getIOLists();
    shared_ptr<vector<vector<int>>> inputLists = iolc.getIOLists();
    
    for (vector<int>& iLst : *inputLists)
    {
        if (distinguished(otherNode, iLst))
        {
            return make_shared<InputTrace>(iLst, presentationLayer);
        }
    }
    return nullptr;
}

bool FsmNode::rDistinguished(const shared_ptr<FsmNode>& otherNode, const vector<int>& iLst)
{
    if (iLst.size() < 1)
    {
        return false;
    }
    InputTrace itr = InputTrace(iLst, presentationLayer);
    OutputTree ot1 = apply(itr);
    OutputTree ot2 = otherNode->apply(itr);
    vector<IOTrace> intersection = ot1.getOutputsIntersection(ot2);
    return intersection.size() == 0;
}

shared_ptr<InputTrace> FsmNode::rDistinguished(const shared_ptr<FsmNode>& otherNode, shared_ptr<Tree> w)
{
    IOListContainer iolc = w->getIOLists();
    shared_ptr<vector<vector<int>>> inputLists = iolc.getIOLists();

    for (vector<int>& iLst : *inputLists)
    {
        if (rDistinguished(otherNode, iLst))
        {
            return make_shared<InputTrace>(iLst, presentationLayer);
        }
    }
    return nullptr;
}

InputTrace FsmNode::calcDistinguishingTrace(const shared_ptr<FsmNode>& otherNode,
                                            const vector<shared_ptr<PkTable>>& pktblLst,
                                            const int maxInput)
{
    /*Determine the smallest l >= 1, such that this and otherNode are
     distinguished by P_l, but not by P_(l-1).
     Note that table P_n is found at pktblLst.get(n-1)*/
    unsigned int l;
    for (l = 1; l <= pktblLst.size(); ++ l)
    {
        /*Two nodes are distinguished by a Pk-table, if they
         reside in different Pk-table classes.*/
        shared_ptr<PkTable> pk = pktblLst.at(l - 1);
        if (pk->getClass(this->getId()) != pk->getClass(otherNode->getId()))
        {
            break;
        }
    }
    
    shared_ptr<FsmNode> qi = shared_from_this();
    shared_ptr<FsmNode> qj = otherNode;
    
    InputTrace itrc = InputTrace(presentationLayer);
    
    for (int k = 1; l - k > 0; ++ k)
    {
        bool foundNext = false;
        shared_ptr<PkTable> plMinK = pktblLst.at(l - k - 1);
        /*Determine input x such that qi.after(x) is distinguished
         from qj.after(x) in plMinK*/
        
        for (int x = 0; x <= maxInput; ++ x)
        {
            /*We are dealing with completely defined DFSMs,
             so after() returns an ArrayList containing exactly
             one element.*/
            shared_ptr<FsmNode> qiNext = qi->after(x).front();
            shared_ptr<FsmNode> qjNext = qj->after(x).front();
            
            if ( plMinK->getClass(qiNext->getId()) != plMinK->getClass(qjNext->getId()) )
            {
                qi = qiNext;
                qj = qjNext;
                itrc.add(x);
                foundNext = true;
                break;
            }
        }
        
        if ( not foundNext ) {
            cerr << "ERROR: inconsistency 1 detected when deriving distinguishing trace from Pk-Tables" << endl;
        }
        
    }
    
    /*Now the case l == k. qi and qj must be distinguishable by at least
     one input*/
    bool foundLast = false;
    for (int x = 0; x <= maxInput; ++ x) {
        
        OutputTrace oti = OutputTrace(presentationLayer);
        OutputTrace otj = OutputTrace(presentationLayer);
        qi->apply(x, oti);
        qj->apply(x, otj);
        if (oti.get().front() != otj.get().front())
        {
            itrc.add(x);
            foundLast = true;
            break;
        }
    }
    
    if ( not foundLast ) {
        cerr << "ERROR: inconsistency 2 detected when deriving distinguishing trace from Pk-Tables" << endl;
    }
    
    return itrc;
}

InputTrace FsmNode::calcDistinguishingTrace(const shared_ptr<FsmNode>& otherNode,
                                            const vector<shared_ptr<OFSMTable>>& ofsmTblLst,
                                            const int maxInput,
                                            const int maxOutput)
{
    InputTrace itrc = InputTrace(presentationLayer);
    int q1 = this->getId();
    int q2 = otherNode->getId();
    
    /*Now we know that this and otherNode are NOT distinguished by OFSM-Table-0.
     Determine the smallest l >= 1, such that this and otherNode are
     distinguished by OFSM-Table l, but not by OFSM-table (l-1).
     Note that table OFSM-table n is found at ofsmTblLst.get(n).*/
    unsigned int l;
    for (l = 1; l < ofsmTblLst.size(); ++ l)
    {
        /*Two nodes are distinguished by a OFSM-table, if they
         reside in different OFSM-table classes.*/
        shared_ptr<OFSMTable> ot = ofsmTblLst.at(l);
        if (ot->getS2C().at(q1) != ot->getS2C().at(q2))
        {
            break;
        }
    }
    
    for (int k = 1; l - k > 0; ++ k)
    {
        shared_ptr<OFSMTable> ot = ofsmTblLst.at(l - k);
        
        /*Determine IO x/y such that qi.after(x/y) is distinguished
         from qj.after(x/y) in ot*/
        for (int x = 0; x <= maxInput; ++ x)
        {
            for (int y = 0; y <= maxOutput; ++ y)
            {
                int q1Post = ot->get(q1, x, y);
                int q2Post = ot->get(q2, x, y);
                
                if (q1Post < 0 || q2Post < 0)
                {
                    continue;
                }
                
                if (ot->getS2C().at(q1Post) != ot->getS2C().at(q2Post))
                {
                    itrc.add(x);
                    
                    /*Set q1,q2 to their post-states under x/y*/
                    q1 = q1Post;
                    q2 = q2Post;
                    break;
                }
            }
        }
    }
    
    /*Now the case l == k. q1 and q2 must be distinguishable by at least
     one IO in OFSM-Table-0*/
    shared_ptr<OFSMTable> ot0 = ofsmTblLst.front();
    for (int x = 0; x <= maxInput; ++ x)
    {
        for (int y = 0; y <= maxOutput; ++ y)
        {
            if ( (ot0->get(q1, x, y) < 0 && ot0->get(q2, x, y) >= 0) or
                 (ot0->get(q1, x, y) >= 0 && ot0->get(q2, x, y) < 0))
            {
                itrc.add(x);
                return itrc;
            }
        }
    }
    return itrc;
}

bool FsmNode::isObservable() const
{
    
    for ( size_t t = 0; t < transitions.size(); t++ ) {
        
        auto lbl = transitions[t]->getLabel();
        
        for ( size_t other = t + 1; other < transitions.size(); other++ ) {
            auto otherLbl = transitions[other]->getLabel();
            if ( *lbl == *otherLbl )
            {
                VLOG(1) << "Node " << getName() << " is not observable:";
                VLOG(1) << "  " << transitions[t]->str();
                VLOG(1) << "  " << transitions[other]->str();
                return false;
            }
        }
        
    }
    
    return true;
    
}

bool FsmNode::isDeterministic() const
{
    unordered_set<int> inputSet;
    
    /*Check if more than one outgoing transition
     is labelled with the same input value*/
    for (auto tr : transitions)
    {
        int inp = tr->getLabel()->getInput();
        if (!inputSet.insert(inp).second)
        {
            return false;
        }
    }
    return true;
}

ostream & operator<<(ostream & out, const FsmNode & node)
{
    for (auto tr : node.transitions)
    {
        out << *tr << endl;
    }
    return out;
}

bool operator==(FsmNode const & node1, FsmNode const & node2)
{
    if (node1.id == node2.id)
    {
        return true;
    }
    return false;
}





void FsmNode::accept(FsmVisitor& v) {
    
    v.visit(*this);
    
}

void FsmNode::accept(FsmVisitor& v,
                     std::deque< std::shared_ptr<FsmNode> >& bfsq){
    
    setVisited();
    v.visit(*this);
    
    for ( auto t : transitions ) {
        t->accept(v);
        t->getTarget()->accept(v);
        if ( not t->getTarget()->hasBeenVisited() ) {
            bfsq.push_back(t->getTarget());
        }
    }
    
}
