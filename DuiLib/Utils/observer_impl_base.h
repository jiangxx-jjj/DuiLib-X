#ifndef OBSERVER_IMPL_BASE_HPP
#define OBSERVER_IMPL_BASE_HPP

#include <map>
#include <vector>

template <typename ReturnT, typename ParamT>
class ReceiverImplBase;

template <typename ReturnT, typename ParamT>
class ObserverImplBase
{
public:
	virtual void AddReceiver(ReceiverImplBase<ReturnT, ParamT>* receiver) = 0;
	virtual void RemoveReceiver(ReceiverImplBase<ReturnT, ParamT>* receiver) = 0;
	virtual ReturnT Broadcast(ParamT param) = 0;
	virtual ReturnT Notify(ParamT param) = 0;
};

template <typename ReturnT, typename ParamT>
class ReceiverImplBase
{
public:
	virtual void AddObserver(ObserverImplBase<ReturnT, ParamT>* observer) = 0;
	virtual void RemoveObserver() = 0;
	virtual ReturnT Receive(ParamT param) = 0;
	virtual ReturnT Respond(ParamT param, ObserverImplBase<ReturnT, ParamT>* observer) = 0;
};

template <typename ReturnT, typename ParamT>
class ReceiverImpl;

template <typename ReturnT, typename ParamT>
class ObserverImpl : public ObserverImplBase<ReturnT, ParamT>
{
public:
	ObserverImpl()
		: count_(0)
	{}

	virtual ~ObserverImpl()	{}

	virtual void AddReceiver(ReceiverImplBase<ReturnT, ParamT>* receiver)
	{
		if (receiver == NULL)
			return;

		receivers_[count_] = receiver;
		receiver->AddObserver(this);
		count_++;
	}

	virtual void RemoveReceiver(ReceiverImplBase<ReturnT, ParamT>* receiver)
	{
		if (receiver == NULL)
			return;

		typename ReceiversMap::iterator it = receivers_.begin();
		while (it != receivers_.end())
		{
			if (it->second == receiver)
			{
				it = receivers_.erase(it);   // erase 返回下一个迭代器，避免迭代器失效
				count_--;
			}
			else
			{
				++it;
			}
		}
	}

	virtual ReturnT Broadcast(ParamT param)
	{
		// 拷贝快照再遍历：Receive 内部可能注销自身导致迭代器失效
		std::vector<ReceiverImplBase<ReturnT, ParamT>*> snapshot;
		{
			typename ReceiversMap::iterator it = receivers_.begin();
			for (; it != receivers_.end(); ++it)
				snapshot.push_back(it->second);
		}
		typename std::vector<ReceiverImplBase<ReturnT, ParamT>*>::const_iterator it = snapshot.begin();
		for (; it != snapshot.end(); ++it)
		{
			(*it)->Receive(param);
		}

		return ReturnT();
	}

	virtual ReturnT Notify(ParamT param)
	{
		// 拷贝快照再遍历：Respond 内部可能注销自身导致迭代器失效
		std::vector<ReceiverImplBase<ReturnT, ParamT>*> snapshot;
		{
			typename ReceiversMap::iterator it = receivers_.begin();
			for (; it != receivers_.end(); ++it)
				snapshot.push_back(it->second);
		}
		typename std::vector<ReceiverImplBase<ReturnT, ParamT>*>::const_iterator it = snapshot.begin();
		for (; it != snapshot.end(); ++it)
		{
			(*it)->Respond(param, this);
		}

		return ReturnT();
	}

protected:
	typedef std::map<int, ReceiverImplBase<ReturnT, ParamT>*> ReceiversMap;
	ReceiversMap receivers_;
	int count_;
};


template <typename ReturnT, typename ParamT>
class ReceiverImpl : public ReceiverImplBase<ReturnT, ParamT>
{
public:
	ReceiverImpl()
		: count_(0)
	{}

	virtual ~ReceiverImpl()	{}

	virtual void AddObserver(ObserverImplBase<ReturnT, ParamT>* observer)
	{
		if (observer == NULL)
			return;
		observers_[count_] = observer;
		count_++;
	}

	virtual void RemoveObserver()
	{
		typename ObserversMap::iterator it = observers_.begin();
		while (it != observers_.end())
		{
			ObserverImplBase<ReturnT, ParamT>* pObserver = it->second;
			// 先移除自身记录再通知对方，避免重入与悬空指针
			it = observers_.erase(it);
			count_--;
			pObserver->RemoveReceiver(this);
		}
	}

	virtual ReturnT Receive(ParamT param)
	{
		return ReturnT();
	}

	virtual ReturnT Respond(ParamT param, ObserverImplBase<ReturnT, ParamT>* observer)
	{
		return ReturnT();
	}

protected:
	typedef std::map<int, ObserverImplBase<ReturnT, ParamT>*> ObserversMap;
	ObserversMap observers_;
	int count_;
};

#endif // OBSERVER_IMPL_BASE_HPP