/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#include "fsm/InputTrace.h"
#include "fsm/FsmLabel.h"

InputTrace::InputTrace(const std::shared_ptr<FsmPresentationLayer>& presentationLayer)
	: Trace(presentationLayer)
{

}

InputTrace::InputTrace(const std::vector<int>& trace, const std::shared_ptr<FsmPresentationLayer>& presentationLayer)
	: Trace(trace, presentationLayer)
{

}

InputTrace::InputTrace(int x,
           const std::shared_ptr<FsmPresentationLayer>& presentationLayer)
    :Trace(std::vector<int>({x}), presentationLayer)
{

}

InputTrace::InputTrace(const InputTrace& other, size_t n):
    Trace(other.getPresentationLayer())
{
    std::vector<int> otherTrace = other.get();
    if (n > otherTrace.size() - 1)
    {
        n = otherTrace.size() - 1;
    }

    if (n > 0)
    {
        trace = std::vector<int>(otherTrace.begin() + static_cast<std::vector<int>::difference_type>(n), otherTrace.end());
    }
}

InputTrace::InputTrace(const InputTrace& other):
    Trace(other)
{

}

InputTrace::InputTrace(const Trace& other):
    Trace(other)
{

}

bool InputTrace::contains(const std::vector<std::shared_ptr<InputTrace>>& list, const InputTrace& trace)
{
    for (std::shared_ptr<InputTrace> t : list)
    {
        if (*t == trace)
        {
            return true;
        }
    }
    return false;
}

bool InputTrace::isEmptyTrace() const
{
    return trace.size() == 1 && trace.at(0) == FsmLabel::EPSILON;
}

std::ostream & operator<<(std::ostream & out, const InputTrace & trace)
{
	for (auto it = trace.cbegin(); it != trace.cend(); ++ it)
	{
		if (it != trace.cbegin())
		{
			out << ".";
		}
        if (*it == -1)
        {
            out << "ε";
        }
        else
        {
            out << trace.presentationLayer->getInId(*it);
        }
	}
	return out;
}

InputTrace& InputTrace::operator=(InputTrace&& other)
{
    if (this != &other)
    {
        Trace::operator=(std::move(other));
    }
    return *this;
}
