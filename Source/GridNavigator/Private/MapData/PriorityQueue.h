// based on:
// https://gist.github.com/SomethingWithComputers/48e32b281d1bf8e662412e4113df43dc#file-priorityqueue-cpp-L25

#pragma once

template <typename NodeT>
struct TPriorityQueueNode
{
	NodeT Element;
	float Priority;

	TPriorityQueueNode() = default;
	TPriorityQueueNode(NodeT InElement, float InPriority) : Element(InElement), Priority(InPriority) {}

	bool operator<(const TPriorityQueueNode<NodeT>& Other) const
	{
		return this->Priority < Other.Priority;
	}
};

template <typename InType>
class TPriorityQueue
{
public:
	TPriorityQueue() {}

	InType Pop()
	{
		TPriorityQueueNode<InType> Node;
		this->Data.HeapPop(Node);
		return Node.Element;
	}
	
	void Push(InType Element, float Priority)
	{
		this->Data.HeapPush(TPriorityQueueNode<InType>(Element, Priority));
	}

	bool IsEmpty()
	{
		return this->Data.IsEmpty();
	}

private:
	TArray<TPriorityQueueNode<InType>> Data;
};
