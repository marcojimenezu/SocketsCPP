//
// Queue.h: Interface for the Queue class.
//

#ifndef QUEUE_H_
#define QUEUE_H_

#include "mutex.h"

//-----------------------------------------------------------------------------
// QueueEntry template class definition.
//-----------------------------------------------------------------------------
template <class T>

class QueueEntry
{
public:
	T value;
	QueueEntry* next;

	QueueEntry(const T& item_value);
};

template <class T>
QueueEntry<T> ::QueueEntry(const T& item_value) {
	value = item_value;
	next = (QueueEntry<T>*)0;
} // QueueEntry()


//-----------------------------------------------------------------------------
// Queue template class definition.
//-----------------------------------------------------------------------------
template <class T>

class Queue : private Semaphore, Mutex
{
private:
	QueueEntry<T>* head;
	QueueEntry<T>* tail;
	int m_nSize;

public:
	Queue()
	{
		m_nSize = 0;
		head = tail = (QueueEntry<T>*)0;
	} // Queue()

	void Add(const T& item_value)
	{
		Lock();
		if (tail == (QueueEntry<T>*)0)
		{
			head = tail = new QueueEntry<T>(item_value);
		}
		else
		{
			tail->next = new QueueEntry<T>(item_value);
			tail = tail->next;
		}
		++m_nSize;
		Unlock();
		Post(); // wake up any waiting threads
	} // Add()

	T Wait()
	{
		Semaphore::Wait(); // wait for something to show up

		Lock();
		T value = head->value;
		QueueEntry<T>* old = head;
		head = head->next;
		delete old;

		if (head == (QueueEntry<T>*)0)
		{
			tail = (QueueEntry<T>*)0;
		}

		--m_nSize;
		Unlock();
		return value;
	} // Wait()

	inline QueueEntry<T>* begin(){ return head; }
	inline QueueEntry<T>* end(){ return tail; }

	inline int size() const { return m_nSize; }
};

#endif // QUEUE_H_
