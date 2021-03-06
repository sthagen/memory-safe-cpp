// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>

void func(int* ip, int& ir) {

    int* ip2 = ip;
// CHECK-FIXES: int* ip2 = nodecpp::safememory::dezombiefy( ip );

    int& ir2 = ir;
// CHECK-FIXES: int& ir2 = nodecpp::safememory::dezombiefy( ir );

    int i = *ip2;// no dz needed here

    i = ir2;// no dz needed here
}

class Class {

    int attribute = 0;

    void method1() {
        int i = attribute;
// CHECK-FIXES: int i = nodecpp::safememory::dezombiefy( this )->attribute;
        
        i = this->attribute; //no dz needed here
    }

    void method2() {
        int i = this->attribute;
// CHECK-FIXES: int i = nodecpp::safememory::dezombiefy( this )->attribute;
    }

    void method3() {
        method1();
// CHECK-FIXES: nodecpp::safememory::dezombiefy( this )->method1();
    }

    void method4() {
        this->method1();
// CHECK-FIXES: nodecpp::safememory::dezombiefy( this )->method1();
    }
};