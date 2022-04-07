/* time example */
#include <iostream>
#include <ctime>
#include <atomic>
#include <vector>
#include <algorithm>    // std::sort
// #include <boost/thread.hpp>
using namespace std;

// void increment(boost::barrier &bar, atomic<int> &count) {
// 	bar.wait();
// 	// std::cout << "In thread " <<  boost::this_thread::get_id() << std::endl;
// 	cout << count.fetch_add(1, memory_order_relaxed) << endl;
// }

// struct A{
//     char a;
//     int b;
//     short c;
// };

// int main ()
// {
// 	// atomic<int> counter(0);
// 	// boost::barrier bar(10);
	
// 	// vector<boost::thread> threads;
// 	// for (int i = 0; i < 10; ++i) {
// 	// 	boost::thread thr(boost::bind(&increment, boost::ref(bar), boost::ref(counter)));
// 	// 	threads.push_back(move(thr));
// 	// }

// 	// for (boost::thread &thr : threads) {
// 	// 	thr.join();
// 	// }

// 	// cout << counter << endl;

//     struct A a;
//     printf("A: %ld\n", sizeof(a));


// 	return 0;
// }

class A {
	virtual int getSize() {
		cout << "Inside A:\n";
		return 0;
	}
};

class B {
public:
	int size;	
	B() : size(getSize()) {}

	virtual int getSize() {
		cout << "Inside B:\n";
		return 1;
	}
};

int main(int argc, char const *argv[]) {
	B b;
	cout << b.size << endl;
	return 0;
}
