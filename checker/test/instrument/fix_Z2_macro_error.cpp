// RUN: %check_nodecpp_instrument --fix-only --no-silent-mode %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_ptr.h>
#include <dezombiefy.h>
#include <safe_types.h>

#define MACRO_FUNC(X, Y) (safeFunction((X), (Y)))
#define MACRO_DO(X) (X)

using namespace nodecpp::safememory;

struct UnsafeType {

	int call(int i) { return i; }
	UnsafeType& operator<<(int) {
		return *this;
	}

};


struct Bad {

	SafeType StMember;
	owning_ptr<SafeType> StPtr;
	owning_ptr<UnsafeType> UPtr;


	Bad() { 
		StPtr = make_owning<SafeType>(); 
		UPtr = make_owning<UnsafeType>();
	}

	int release() {
		StPtr.reset();
		UPtr.reset();
		return 0;
	}

	void otherMethod(int i) {}
	SafeType& getSt() { return *StPtr; }
	UnsafeType& getU() { return *UPtr; }

	void verifyZombieMacro() {

		MACRO_FUNC(getSt(), getSt());
//CHECK-MESSAGES: :[[@LINE-1]]:3: error: Unwrap couldn't complete because of MACRO

		safeFunction(MACRO_DO(getSt()), getSt());
//CHECK-MESSAGES: :[[@LINE-1]]:25: error: Unwrap couldn't complete because of MACRO
	}
};
