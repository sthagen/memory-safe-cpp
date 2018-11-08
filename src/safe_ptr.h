#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <assert.h>

#if defined __GNUC__
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

//#define NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST

enum class MemorySafety {none, partial, full};

//#define NODECPP_MEMORYSAFETY_NONE
#define NODECPP_MEMORYSAFETY_EARLY_DETECTION

#ifdef NODECPP_MEMORYSAFETY_NONE
#define NODECPP_ISSAFE_MODE MemorySafety::none
#define NODECPP_ISSAFE_DEFAULT false
#elif defined NODECPP_MEMORYSAFETY_PARTIAL
#define NODECPP_ISSAFE_MODE MemorySafety::partial
#define NODECPP_ISSAFE_DEFAULT true
#elif defined NODECPP_MEMORYSAFETY_FULL
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#else
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#endif

#ifndef NODECPP_MEMORYSAFETY_NONE
#ifdef NODECPP_MEMORYSAFETY_EARLY_DETECTION
//constexpr void* invalid_ptr = (void*)(1);
#endif
#endif


struct Ptr2PtrWishFlags {
private:
	uintptr_t ptr;
public:
	void set( void* ptr_ ) { ptr = (uintptr_t)ptr_; assert( !isUsed() ); }// reasonable default
	void* getPtr() { return (void*)( ptr & ~((uintptr_t)3) ); }
	void setUsed() { ptr |= 1; }
	void setUnused() { ptr &= ~((uintptr_t)1); }
	bool isUsed() { return ptr & 1; }
	void set1stBlock() { ptr |= 2; }
	void set2ndBlock() { ptr &= ~((uintptr_t)2); }
	bool is1stBlock() { return (ptr & 2)>>1; }
	static bool is1stBlock( uintptr_t ptr ) { return (ptr & 2)>>1; }
};
static_assert( sizeof(Ptr2PtrWishFlags) == 8 );

struct Ptr2PtrWishData {
//private:
	uintptr_t ptr;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t upperDataMask_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	static constexpr uintptr_t upperDataMaskInData_ = 0x7FFF8ULL;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static constexpr size_t upperDataShift_ = 45;
	static_assert ( (upperDataMaskInData_ << upperDataShift_ ) == upperDataMask_ );
	static_assert ( (ptrMask_ & upperDataMask_) == 0 );
	static_assert ( (ptrMask_ >> (upperDataShift_ + lowerDataSize_)) == 0 );
	static_assert ( (ptrMask_ & lowerDataMask_) == 0 );
	static_assert ( (upperDataMask_ & lowerDataMask_) == 0 );
	static_assert ( (ptrMask_ | upperDataMask_ | lowerDataMask_) == 0xFFFFFFFFFFFFFFFFULL );
public:
	void init( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = (uintptr_t)ptr_; 
	}
	void* getPtr() const { return (void*)( ptr & ptrMask_ ); }
	size_t getData() const { return ( (ptr & upperDataMask_) >> 45 ) | (ptr & lowerDataMask_); }
	void updatePtr( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr &= ~ptrMask_; 
		ptr |= (uintptr_t)ptr_; 
	}
	void updateData( size_t data ) { 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr &= ptrMask_; 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
	void swap( Ptr2PtrWishData& other ) { uintptr_t tmp = ptr; ptr = other.ptr; other.ptr = tmp; }
};
static_assert( sizeof(Ptr2PtrWishData) == 8 );

static_assert( sizeof(void*) == 8 );
struct FirstControlBlock // not reallocatable
{
	static constexpr size_t maxSlots = 5;
	static constexpr size_t secondBlockStartSize = 8;	
	Ptr2PtrWishFlags* firstFree;
	size_t otherAllockedCnt = 0; // TODO: try to rely on our allocator on deriving this value
	Ptr2PtrWishFlags* otherAllockedSlots = nullptr;
	Ptr2PtrWishFlags slots[maxSlots];

	void dbgCheckFreeList() {
		Ptr2PtrWishFlags* start = firstFree;
		while( start ) {
			assert( !start->isUsed() );
			assert( ( start->getPtr() == 0 || start->is1stBlock() && (size_t)(start - slots) < maxSlots ) || ( (!start->is1stBlock()) && (size_t)(start - otherAllockedSlots) < otherAllockedCnt ) );
			start = ((Ptr2PtrWishFlags*)(start->getPtr()));
		}
	}

	void init() {
		/*firstFree = slots;
		for ( size_t i=0; i<maxSlots-1; ++i ) {
			slots[i].set(slots + i + 1);
			slots[i].set1stBlock();
		}
		slots[maxSlots-1].set(nullptr);*/
		firstFree = nullptr;
		addToRfeeList( slots, maxSlots );
		otherAllockedCnt = 0;
		otherAllockedSlots = nullptr;
		assert( !firstFree->isUsed() );
		dbgCheckFreeList();
	}
	void deinit() {
		if ( otherAllockedSlots != nullptr ) {
			assert( otherAllockedCnt != 0 );
			delete [] otherAllockedSlots;
			otherAllockedCnt = 0;
		}
		else {
			assert( otherAllockedCnt == 0 );
		}
	}
	void addToRfeeList( Ptr2PtrWishFlags* begin, size_t count ) {
		assert( firstFree == nullptr );
		firstFree = begin;
		for ( size_t i=0; i<count-1; ++i ) {
			begin[i].set(begin + i + 1);
			begin[i].set1stBlock();
		}
		begin[count-1].set(nullptr);
		dbgCheckFreeList();
	}
	void enlargeSecondBlock() {
		if ( otherAllockedSlots != nullptr ) {
			assert( otherAllockedCnt != 0 );
			size_t newSize = otherAllockedCnt << 1;
			Ptr2PtrWishFlags* newOtherAllockedSlots = new Ptr2PtrWishFlags[newSize];
			memcpy( newOtherAllockedSlots, otherAllockedSlots, sizeof(Ptr2PtrWishFlags) * otherAllockedCnt );
			delete [] otherAllockedSlots;
			otherAllockedSlots = newOtherAllockedSlots;
			addToRfeeList( otherAllockedSlots + otherAllockedCnt, otherAllockedCnt );
			otherAllockedCnt = newSize;
		}
		else {
			assert( otherAllockedCnt == 0 );
			otherAllockedCnt = secondBlockStartSize;
			otherAllockedSlots = new Ptr2PtrWishFlags[otherAllockedCnt];
			addToRfeeList( otherAllockedSlots, otherAllockedCnt );
		}
	}
	size_t insert( void* ptr ) {
		assert( firstFree == nullptr || !firstFree->isUsed() );
		if ( firstFree != nullptr ) {
			Ptr2PtrWishFlags* tmp = ((Ptr2PtrWishFlags*)(firstFree->getPtr()));
			assert( !firstFree->isUsed() );
			size_t idx;
			if ( firstFree->is1stBlock() )
				idx = firstFree - slots;
			else
				idx = maxSlots + otherAllockedSlots - firstFree;
			firstFree->set(ptr);
			firstFree->setUsed();
			firstFree = tmp;
			assert( firstFree == nullptr || !firstFree->isUsed() );
			dbgCheckFreeList();
			return idx;
		}
		else {
			// TODO: reallocate
			assert(false); // TODO+++++
			return -1;
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		if ( idx < maxSlots ) {
			slots[idx].set( newPtr );
			slots[idx].setUsed();
			slots[idx].set1stBlock();
		}
		else {
			assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots[idx].set( newPtr );
			otherAllockedSlots[idx].setUsed();
			otherAllockedSlots[idx].set1stBlock();
		}
		dbgCheckFreeList();
		assert( firstFree == nullptr || !firstFree->isUsed() );
	}
	void remove( size_t idx ) {
		assert( firstFree == nullptr || !firstFree->isUsed() );
		if ( idx < maxSlots ) {
			slots[idx].set( firstFree );
			firstFree = slots + idx;
			firstFree->setUnused();
			firstFree->set1stBlock();
		}
		else {
			assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots[idx].set( firstFree );
			firstFree = otherAllockedSlots + idx;
			firstFree->setUnused();
			firstFree->set2ndBlock();
		}
		assert( firstFree == nullptr || !firstFree->isUsed() );
		dbgCheckFreeList();
	}
	void clear() {
	}
};
static_assert( sizeof(FirstControlBlock) == 64 );

template<class T, bool isSafe> class soft_ptr; // forward declaration

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class owning_ptr
{
	T* t;
	FirstControlBlock* getControlBlock() { return reinterpret_cast<FirstControlBlock*>(t) - 1; }

	void updatePtrForListItems( T* t_ )
	{
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
				reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->setPtr_( t_ );
		for ( size_t i=0; i<cb->otherAllockedCnt; ++i )
			if ( cb->otherAllockedSlots[i].isUsed() )
				reinterpret_cast<soft_ptr<T, isSafe>*>(cb->otherAllockedSlots[i].getPtr())->setPtr_( t_ );
	}

	void updatePtrForListItemsWithInvalidPtr() { updatePtrForListItems( nullptr ); }

public:
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( T* t_ )
	{
		t = t_;
		getControlBlock()->init();
	}
	owning_ptr( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr( owning_ptr<T, isSafe>&& other ) = default;
	owning_ptr& operator = ( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other ) = default;
	~owning_ptr()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			updatePtrForListItemsWithInvalidPtr();
			delete [] getControlBlock();
#ifdef NODECPP_MEMORYSAFETY_EARLY_DETECTION
			t = nullptr;
#endif
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			updatePtrForListItemsWithInvalidPtr();
			delete [] getControlBlock();
			t = nullptr;
		}
	}

	void reset( T* t_ ) // Q: what happens to safe ptrs?
	{
		T* tmp = t;
		t = t_;
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			delete [] getControlBlock();
			if ( NODECPP_LIKELY(t != t_) )
			{
				t = t_;
				updatePtrForListItems( t_ );
			}
			else
			{
				t = nullptr;
				updatePtrForListItemsWithInvalidPtr();
			}
		}
		else
		{
			t = t_;
			updatePtrForListItems( t_ );
		}
	}

	void swap( owning_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		// if zero guard page... if constexpr( sizeof(T)<4096); then add signal handler
		//todo: pair<ptr,sz> realloc(ptr,minsise,desiredsize); also: size_t try_inplace_realloc(ptr,minsise,desiredsize)
		//<bool isSafe=NODECPP_ISSAFE()>
		assert( t != nullptr );
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};

#if 0
template<class T>
class owning_ptr<T>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class soft_ptr<T>;
	T* t;

public:
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( T* t_ )
	{
		t = t_;
	}
	owning_ptr( owning_ptr<T, false>& other ) = delete;
	owning_ptr( owning_ptr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	~owning_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: being placed at the level of class definition, the codition may be checked whether or not this specialization is instantiated (see, for instance, https://stackoverflow.com/questions/5246049/c11-static-assert-and-template-instantiation)
		if ( NODECPP_LIKELY(t) )
		{
			delete t;
		}
	}

	owning_ptr& operator = ( owning_ptr<T, false>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
		return *this;
	}

	void reset( T* t_ = t )
	{
		T* tmp = t;
		t = t_;
		// if ( NODECPP_LIKELY(tmp) ) : we do not need this check
		delete tmp;
	}

	void swap( owning_ptr<T, false>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};
#endif // 0

template<bool _Test,
	class _Ty = void>
	using enable_if_t = typename std::enable_if<_Test, _Ty>::type;

template<class _Ty>
	_INLINE_VAR constexpr bool is_array_v = std::is_array<_Ty>::value;

template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int> = 0>
	_NODISCARD inline owning_ptr<_Ty> make_owning(_Types&&... _Args)
	{	// make a unique_ptr
	uint8_t* data = new uint8_t[ sizeof(FirstControlBlock) + sizeof(_Ty) ];
	_Ty* objPtr = new ( data + sizeof(FirstControlBlock) ) _Ty(_STD forward<_Types>(_Args)...);
	return owning_ptr<_Ty>(objPtr);
//	return (owning_ptr<_Ty>(new _Ty(_STD forward<_Types>(_Args)...)));
	}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr<T, isSafe>;

#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
	T* t;
	size_t idx;
	T* getPtr_() const { return t; }
	void setPtr_( T* t_ ) { t = t_; }
	size_t getIdx_() const { return idx; }
#else
	Ptr2PtrWishData td;
	T* getPtr_() const { return static_cast<T*>(td.getPtr()); }
	void setPtr_( T* t_ ) { td.updatePtr( t_ ); }
	size_t getIdx_() const { return td.getData(); }
#endif
	FirstControlBlock* getControlBlock() { return reinterpret_cast<FirstControlBlock*>(getPtr_()) - 1; }

public:
	soft_ptr()
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = nullptr;
		idx = (size_t)(-1);
#else
		td.init(nullptr);
#endif
	}
	soft_ptr( owning_ptr<T, isSafe>& owner )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = owner.get();
		idx = getControlBlock()->insert(this);
#else
		td.init(owner.get());
		td.updateData(getControlBlock()->insert(this));
#endif
	}
	soft_ptr( soft_ptr<T, isSafe>& other )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = other.t;
		idx = getControlBlock()->insert(this);
#else
		td = other.td;
		td.updateData(getControlBlock()->insert(this));
#endif
	}
	soft_ptr( soft_ptr<T, isSafe>&& other )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = other.t;
		other.t = nullptr;
		idx = other.idx;
		other.idx = (size_t)(-1);
#else
		td = other.td;
		other.td.init(nullptr);
#endif
		getControlBlock()->resetPtr(getIdx_(), this);
	}

	soft_ptr& operator = ( soft_ptr<T, isSafe>& other )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = other.t;
		idx = getControlBlock()->insert(this);
#else
		td = other.td;
		td.setData(getControlBlock()->insert(this));
#endif
		return *this;
	}
	soft_ptr& operator = ( soft_ptr<T, isSafe>&& other )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		t = other.t;
		other.t = nullptr;
		idx = other.idx;
		other.idx = (size_t)(-1);
#else
		td = other.td;
		other.td.init(nullptr);
#endif
		getControlBlock()->resetPtr(getIdx_(), this);
		return *this;
	}

	void swap( soft_ptr<T, isSafe>& other )
	{
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
		T* tmp = t;
		t = other.t;
		other.t = tmp;
		size_t idx = idx;
		idx = other.idx;
		other.idx = idx;
		if ( t )
			getControlBlock()->resetPtr(idx, this);
		if ( other.t )
			other.getControlBlock()->resetPtr(other.idx, &other);
#else
		td.swap( other.td );
#endif
		if ( getPtr_() )
			getControlBlock()->resetPtr(getIdx_(), this);
		if ( other.getPtr_() )
			other.getControlBlock()->resetPtr(other.getIdx_(), &other);
	}

	T* get() const
	{
		assert( getPtr_() != nullptr );
		return getPtr_();
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return getPtr_() != nullptr;
	}

	~soft_ptr()
	{
		if( getPtr_() != nullptr ) {
#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
			assert( idx != (size_t)(-1) );
			getControlBlock()->remove(getIdx_());
			t = nullptr;
#else
			//assert( idx != (size_t)(-1) );
			getControlBlock()->remove(getIdx_());
			td.init(nullptr);
#endif
		}
	}
};

#if 0
template<class T>
class soft_ptr<T,false>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class owning_ptr<T,false>;
	T* t;

public:
	soft_ptr()
	{
		this->t = nullptr;
	}
	soft_ptr( owning_ptr<T,false>& owner )
	{
		this->t = owner.t;
	}
	soft_ptr( soft_ptr<T,false>& other )
	{
		this->t = other.t;
	}
	soft_ptr( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
	}

	soft_ptr& operator = ( soft_ptr<T,false>& other )
	{
		this->t = other.t;
		return *this;
	}
	soft_ptr& operator = ( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		return *this;
	}

	void swap( soft_ptr<T, false>& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return this->t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	~soft_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial );
	}
};
#endif // 0

#endif
