#pragma once
#include <initializer_list>

template< class T > struct remove_ref      {typedef T type;};
template< class T > struct remove_ref<T&>  {typedef T type;};
template< class T > struct remove_ref<T&&> {typedef T type;};
template< class T > using RefRemoved = typename remove_ref<T>::type;

template<typename T> constexpr
RefRemoved<T>&& move2(T&& x)
{
	return static_cast< RefRemoved<T>&& > (x);
}

typedef unsigned int uint;
typedef unsigned long long uint64;


template<typename T, typename SizeType = uint> 
class Array
{
    SizeType _size = 0;
	SizeType _capacity = 0;
    T* _data = nullptr;

public:
	~Array() { clear(); }
    Array() {}
    Array(SizeType size)					{ resize(size); }
    Array(SizeType size, const T& value)	{ resize(size, value); }
	Array(std::initializer_list<T> list)
	{
		reserve( (SizeType) list.size() );
		for (auto ele : list)
		{
			push_back(ele);
		}
	}
	Array(const Array& arr) : _size(arr._size), _capacity(arr._capacity)
	{
		_data = static_cast<T*>(::operator new(sizeof(T) * _capacity));
		for (SizeType i = 0; i < _size; ++i)
		{
			new (_data + i) T(arr._data[i]);
		}
	}
    Array(Array&& arr) : _size(arr._size), _capacity(arr._capacity)
	{
		_data = arr._data;
		arr._data = nullptr;
		arr._size = 0;
		arr._capacity = 0;
	}

	void swap(Array& arr)
	{
		SizeType tempSize = _size;
		SizeType tempCapacity = _capacity;
		T* tempData = _data;

		_size = arr._size;
		_capacity = arr._capacity;
		_data = arr._data;

		arr._size = tempSize;
		arr._capacity = tempCapacity;
		arr._data = tempData;
	}

	void reserve(SizeType capacity)
	{
		if (capacity <= _capacity)
			return;

		T* newData = static_cast<T*>(::operator new(sizeof(T) * capacity));
		for (SizeType i = 0; i < _size; ++i)
		{
			new (newData + i) T(_data[i]);
			_data[i].~T();
		}

		if(_data)
			::operator delete(_data);

		_data = newData;
		_capacity = capacity;
	}

	void resize(SizeType size)
	{
		if (size <= _size)
		{
			for (SizeType i = size; i < _size; ++i)
			{
				_data[i].~T();
			}
		}
		else
		{
			if (size > _capacity)
				reserve(size);
			
			for (SizeType i = _size; i < size; ++i)
			{
				new (_data + i) T;
			}
		}

		_size = size;
	}

	void resize(SizeType size, const T& value)
	{
		if (size <= _size)
		{
			for (SizeType i = size; i < _size; ++i)
			{
				_data[i].~T();
			}
		}
		else
		{
			if (size > _capacity)
				reserve(size);
			
			for (SizeType i = _size; i < size; ++i)
			{
				new (_data + i) T(value);
			}
		}

		_size = size;
	}

	void push_back(const T& value)
	{
		if (_size == _capacity)
		{
			SizeType newCapacity = _capacity >= 4 ? (SizeType)((float)_capacity * 1.5f) : 4;
			reserve(newCapacity);
		}
		
		new (_data + _size) T(value);
		++_size;
	}

	void push_back(T&& value)
	{
		if (_size == _capacity)
		{
			SizeType newCapacity = _capacity >= 4 ? (SizeType)((float)_capacity * 1.5f) : 4;
			reserve(newCapacity);
		}
		
		new (_data + _size) T(move2(value));
		++_size;
	}

    void clear()
    {
        for (SizeType i = 0; i < _size; ++i)
		{
			_data[i].~T();
		}

		if(_data)
			::operator delete(_data);
		_data = nullptr;
		_capacity = 0;
		_size = 0;
    }

	SizeType size() const					{return _size;}
	T* data()								{return _data;}
    T* begin()								{return _data;}
    T* end()								{return _data + _size;}
    T& operator[](SizeType idx)				{return _data[idx];}
	const T* data() const					{return _data;}
	const T* begin() const					{return _data;}
    const T* end() const					{return _data + _size;}
    const T& operator[](SizeType idx) const	{return _data[idx];}
};
