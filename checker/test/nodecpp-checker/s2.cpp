// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers | FileCheck %s -implicit-check-not="{{warning|error}}:"



class Bad {
	mutable int i;
// CHECK: :[[@LINE-1]]:14: warning: (S2.2)
};

void bad() { 

	const int i = 0;

	int& j = const_cast<int&>(i);
// CHECK: :[[@LINE-1]]:11: warning: (S2.1)

	auto l = []() mutable {};
// CHECK: :[[@LINE-1]]:11: warning: (S2)

}
