// RUN: %check_nodecpp_instrument --fix-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_ptr.h>
#include <dezombiefy.h>
#include <safe_types.h>

using namespace nodecpp::safememory;

int release() {	return 0; }


struct Bad {

	owning_ptr<SafeType> StPtr;



	Bad() {
		StPtr = make_owning<SafeType>(); 
	}

	// int release() {
	// 	StPtr.reset();
	// 	return 0;
	// }


	void verifyZombie(SafeType& StRef) {

		//safeFunction may be eating a zombie
		while(safeFunction(*StPtr) + release() != 0);
// CHECK-FIXES: while(nodecpp::safememory::ne(nodecpp::safememory::add(safeFunction(*StPtr) , release()) , 0));
	}
};
