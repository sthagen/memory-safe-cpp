// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>


struct TestObj {};
struct TestObj2 {};


template<class T>
class TemplateClass {
public:
    template<class U>
    class TemplateInner {
    public:
        
        auto aMethod(U u) {
            return u;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( u );            
        }

        template<class V>
        auto templMethod(V v) {
            return v;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( v );
        }
    };
};


int main() {

    TestObj to;
    TestObj2 to2;

    TemplateClass<TestObj&>::TemplateInner<TestObj2&> tc12;
    TemplateClass<TestObj2&>::TemplateInner<TestObj&> tc21;
    TemplateClass<TestObj2&>::TemplateInner<TestObj2&> tc22;

    tc12.templMethod<TestObj2&>(to2);
    tc12.templMethod<TestObj&>(to);
    tc21.templMethod<TestObj&>(to);
    tc22.templMethod<TestObj2&>(to2);
    tc22.templMethod<TestObj&>(to);

    tc12.aMethod(to2);
    tc21.aMethod(to);
    tc22.aMethod(to2);
}