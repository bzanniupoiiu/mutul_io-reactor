#include <iostream>
#include <memory>
using namespace std;

void test(shared_ptr<int> sp)
{
    cout<<"test func sp.use_count()"<<sp.use_count()<<endl;

}

int main()
{
    auto sp1 = make_shared<int>(100);
    shared_ptr<int> sp2(new int(100));
    int* p = new int(1);

    std::shared_ptr<int> p1;
    p1.reset(new int(1));
    shared_ptr<int> p2 = p1;
    cout<<p1.use_count();
    
}